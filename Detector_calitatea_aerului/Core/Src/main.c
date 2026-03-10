/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd_i2c.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
//variabile pentru periferice
ADC_HandleTypeDef hadc1;
I2C_HandleTypeDef hi2c1;   // LCD
I2C_HandleTypeDef hi2c2;   // BMP280
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//definirea adreselor
#define LCD_ADDR     (0x27 << 1)
#define BMP280_ADDR  (0x76 << 1)

//variabile pentru calibrarea BMP280
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t t_fine;

//formatarea mesajelor
char uart_buf[100];
//senzor praf
uint32_t counterLow = 0;
uint32_t counterTotal = 0;
uint8_t page = 0;


/* USER CODE END PV */

//prototipuri de funcții( init ceas, senzorii)
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_ADC1_Init(void);

//funcții dedicate senzori
HAL_StatusTypeDef BMP280_Init(void);
HAL_StatusTypeDef BMP280_Read_CalibData(void);
void BMP280_ReadValues(float *temperature, float *pressure);
float BMP280_Compensate_Temperature(int32_t adc_T);
float BMP280_Compensate_Pressure(int32_t adc_P);
static void scan_I2C(void);


int main(void)
{// inițializare sistem
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();     // LCD
    MX_I2C2_Init();     // BMP280
    MX_USART2_UART_Init();
    MX_ADC1_Init();
//inițializare lcd
    LCD_Init(LCD_ADDR);
    LCD_Clear(LCD_ADDR);
    HAL_Delay(100);

//mesaj in monitor
    sprintf(uart_buf, "\r\n--- BMP280 test pe I2C2 ---\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
//scanare magistrala I2C
    scan_I2C();

    float temp = 0, pres = 0;
//initializarea senzor
    if (BMP280_Init() == HAL_OK)
    {
        sprintf(uart_buf, "BMP280 OK!\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
    }
    else
    {
        sprintf(uart_buf, "Eroare BMP280!\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
        //blocare aplicatie
        while(1)
        {
            HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
            HAL_Delay(300);
        }
    }
// bucla mare
    while (1)
    {
        /* ================= BMP280 ================= */
    	//citire senzor
        BMP280_ReadValues(&temp, &pres);

        sprintf(uart_buf, "T=%.2f C | P=%.2f hPa\r\n", temp, pres);
        HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
        //formatare text
        char line1[17];
        char line2[17];

        snprintf(line1, sizeof(line1), "T: %.1f C", temp);
        snprintf(line2, sizeof(line2), "P: %.1f hPa", pres);


        /* ================= SENZOR PRAF ================= */
        /* ================= SENZOR PRAF ================= */
        counterLow = 0;
        counterTotal =2000;

        for (int i = 0; i < counterTotal; i++)
        {
            if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET)
                counterLow++;

            HAL_Delay(1); // 1 ms
        }

        float ratio = counterLow / (float)counterTotal;

        char msg_air[17];
        // clasificare valori
        if(ratio < 0.1)
            strcpy(msg_air, "Aer curat!");
        else if(ratio < 0.5)
            strcpy(msg_air, "Aer moderat");
        else
            strcpy(msg_air, "Aer murdar!!!");



        /* ================= SENZOR GAZ (ADC) ================= */
        uint32_t valoare_gaz;
        char msg_gaz[17];

        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);

        valoare_gaz = HAL_ADC_GetValue(&hadc1);
        //clasificare calitate aer
        if(valoare_gaz < 500)
            strcpy(msg_gaz, "Aer curat");
        else if(valoare_gaz >= 500 && valoare_gaz <= 1000)
            strcpy(msg_gaz, "Aer moderat");
        else if(valoare_gaz >= 1000 && valoare_gaz <= 1500)
            strcpy(msg_gaz, "Posibil spirt!");
        else if(valoare_gaz > 2000)
            strcpy(msg_gaz, " Posibil GAZ !!!");
        else
            strcpy(msg_gaz, "Aer murdar");


        /* ================= PAGINILE LCD ================= */

        page++;
        if(page > 2) page = 0;   // avem 3 pagini acum!

        LCD_Clear(LCD_ADDR);

        if(page == 0)
        {
            // PAGINA 0 = Temperatura + Presiune
            LCD_SetCursor(LCD_ADDR, 0, 0);
            LCD_SendString(LCD_ADDR, line1);

            LCD_SetCursor(LCD_ADDR, 1, 0);
            LCD_SendString(LCD_ADDR, line2);
        }
        else if(page == 1)
        {
            // PAGINA 1 = SENZOR PRAF
            LCD_SetCursor(LCD_ADDR, 0, 0);
            LCD_SendString(LCD_ADDR, "Calitate aer:");

            LCD_SetCursor(LCD_ADDR, 1, 0);
            LCD_SendString(LCD_ADDR, msg_air);
        }
        else if(page == 2)
        {
            // PAGINA 2 = SENZOR GAZ
            LCD_SetCursor(LCD_ADDR, 0, 0);
            LCD_SendString(LCD_ADDR, "Senzor GAZ:");

            LCD_SetCursor(LCD_ADDR, 1, 0);
            LCD_SendString(LCD_ADDR, msg_gaz);
        }

        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        HAL_Delay(200);
    }


}

/* ========================================================================== */
/* ======================  I2C, UART, GPIO, CLOCK  ========================== */
/* ========================================================================== */
//configurare ceas sistem
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    //setare nivel tensiune
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    //configurare oscilator  intern
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();
    //configurare ceas
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}
//initializare i2c1
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00503D58;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
//initializare interfata
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
        Error_Handler();
}

static void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x00503D58;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
        Error_Handler();
}
// initializare uart
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;

    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;   // foarte important!
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* ========================================================================== */
/* ============================ BMP280 DRIVER =============================== */
/* ========================================================================== */

HAL_StatusTypeDef BMP280_Read_CalibData(void)
{
    uint8_t data[24];
//citire coeficienti de calibrare
    if (HAL_I2C_Mem_Read(&hi2c2, BMP280_ADDR, 0x88, I2C_MEMADD_SIZE_8BIT, data, 24, HAL_MAX_DELAY) != HAL_OK)
        return HAL_ERROR;
//coeficienti temperatura
    dig_T1 = (data[1] << 8) | data[0];
    dig_T2 = (data[3] << 8) | data[2];
    dig_T3 = (data[5] << 8) | data[4];
//coeficienti presiune
    dig_P1 = (data[7] << 8) | data[6];
    dig_P2 = (data[9] << 8) | data[8];
    dig_P3 = (data[11] << 8) | data[10];
    dig_P4 = (data[13] << 8) | data[12];
    dig_P5 = (data[15] << 8) | data[14];
    dig_P6 = (data[17] << 8) | data[16];
    dig_P7 = (data[19] << 8) | data[18];
    dig_P8 = (data[21] << 8) | data[20];
    dig_P9 = (data[23] << 8) | data[22];

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_Init(void)
{
    uint8_t id = 0;
//citire senzor pentru verificare
    HAL_I2C_Mem_Read(&hi2c2, BMP280_ADDR, 0xD0, I2C_MEMADD_SIZE_8BIT, &id, 1, HAL_MAX_DELAY);
    if (id != 0x58 && id != 0x60) return HAL_ERROR;

    if (BMP280_Read_CalibData() != HAL_OK)
        return HAL_ERROR;

    uint8_t data[2];
//configurare registru
    data[0] = 0xF5;
    data[1] = 0b10100000;
    HAL_I2C_Master_Transmit(&hi2c2, BMP280_ADDR, data, 2, HAL_MAX_DELAY);

    data[0] = 0xF4;
    data[1] = 0b01010111;
    HAL_I2C_Master_Transmit(&hi2c2, BMP280_ADDR, data, 2, HAL_MAX_DELAY);

    HAL_Delay(100);
    return HAL_OK;
}

float BMP280_Compensate_Temperature(int32_t adc_T)
{//calcule conform senzorului
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) *
                    ((int32_t)dig_T2)) >> 11;

    int32_t var2 = (((((adc_T >> 4) - (int32_t)dig_T1) *
                      ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) *
                    (int32_t)dig_T3) >> 14;

    t_fine = var1 + var2;
//returnare grade Celsius
    return ((t_fine * 5 + 128) >> 8) / 100.0f;

}
//CONVERTIRI ANALOGICE NUMERICE

float BMP280_Compensate_Pressure(int32_t adc_P)
{
    int64_t var1, var2, p;
//calculeze pentru obtinerea presiunii
    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 += ((var1 * (int64_t)dig_P5) << 17);
    var2 += ((int64_t)dig_P4 << 35);

    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) +
           ((var1 * (int64_t)dig_P2) << 12);

    var1 = (((((int64_t)1) << 47) + var1) *
            (int64_t)dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)dig_P8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) + ((int64_t)dig_P7 << 4);

    return ((float)p) / 256.0f / 100.0f;
}

void BMP280_ReadValues(float *temperature, float *pressure)
{
    uint8_t data[6];
//CITIRE VALORI BRUTE
    HAL_I2C_Mem_Read(&hi2c2, BMP280_ADDR, 0xF7, I2C_MEMADD_SIZE_8BIT, data, 6, HAL_MAX_DELAY);

    int32_t adc_P = ((uint32_t)data[0] << 12) |
                    ((uint32_t)data[1] << 4) |
                    ((uint32_t)data[2] >> 4);

    int32_t adc_T = ((uint32_t)data[3] << 12) |
                    ((uint32_t)data[4] << 4) |
                    ((uint32_t)data[5] >> 4);
//VALORI COMPESATE, cu mici erori
    *temperature = BMP280_Compensate_Temperature(adc_T);
    *pressure = BMP280_Compensate_Pressure(adc_P);
}
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
//configurare generale
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;

    HAL_ADC_Init(&hadc1);

    sConfig.Channel = ADC_CHANNEL_0;    // <- canalul tău real
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}


static void scan_I2C(void)

{
	//scanari de dispozitive pe anumite adrese
    sprintf(uart_buf, "Scanare I2C2...\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);

    for (uint8_t addr = 1; addr < 128; addr++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c2, addr << 1, 1, 5) == HAL_OK)
        {
            sprintf(uart_buf, "  -> Gasit 0x%02X\r\n", addr);
            HAL_UART_Transmit(&huart2, (uint8_t*)uart_buf, strlen(uart_buf), HAL_MAX_DELAY);
        }
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
        HAL_Delay(200);
    }
}
