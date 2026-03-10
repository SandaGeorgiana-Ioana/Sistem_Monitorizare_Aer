#include "lcd_i2c.h"

extern I2C_HandleTypeDef hi2c1;

HAL_StatusTypeDef LCD_SendInternal(uint8_t lcd_addr, uint8_t data, uint8_t flags)
{
    HAL_StatusTypeDef res;

    while(1) {
        res = HAL_I2C_IsDeviceReady(&hi2c1, lcd_addr, 1, HAL_MAX_DELAY);
        if(res == HAL_OK) break;
    }

    uint8_t up = data & 0xF0;
    uint8_t lo = (data << 4) & 0xF0;

    uint8_t data_arr[4];
    data_arr[0] = up | flags | BACKLIGHT | PIN_EN;
    data_arr[1] = up | flags | BACKLIGHT;
    data_arr[2] = lo | flags | BACKLIGHT | PIN_EN;
    data_arr[3] = lo | flags | BACKLIGHT;

    res = HAL_I2C_Master_Transmit(&hi2c1, lcd_addr, data_arr, 4, HAL_MAX_DELAY);
    HAL_Delay(LCD_DELAY_MS);

    return res;
}

void LCD_SendCommand(uint8_t lcd_addr, uint8_t cmd)
{
    LCD_SendInternal(lcd_addr, cmd, 0);
}

void LCD_SendData(uint8_t lcd_addr, uint8_t data)
{
    LCD_SendInternal(lcd_addr, data, PIN_RS);
}

void LCD_Init(uint8_t lcd_addr)
{
    HAL_Delay(50);

    LCD_SendCommand(lcd_addr, 0b00110000);
    LCD_SendCommand(lcd_addr, 0b00000010);
    LCD_SendCommand(lcd_addr, 0b00001100);
    LCD_SendCommand(lcd_addr, 0b00000001);
    HAL_Delay(5);
}

void LCD_SendString(uint8_t lcd_addr, char *str)
{
    while(*str) {
        LCD_SendData(lcd_addr, (uint8_t)*str);
        str++;
    }
}
void LCD_Clear(uint8_t addr)
{
    LCD_SendCommand(addr, 0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t addr, uint8_t row, uint8_t col)
{
    static uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    LCD_SendCommand(addr, 0x80 | (col + row_offsets[row]));
}

