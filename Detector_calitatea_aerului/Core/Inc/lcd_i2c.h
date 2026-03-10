#ifndef LCD_I2C_H_
#define LCD_I2C_H_

#include "stm32g0xx_hal.h"

#define LCD_ADDR (0x27 << 1)   // Adresa ta, confirmată prin scanare

#define PIN_RS    (1 << 0)
#define PIN_EN    (1 << 2)
#define BACKLIGHT (1 << 3)

#define LCD_DELAY_MS 5

void LCD_Init(uint8_t lcd_addr);
void LCD_SendCommand(uint8_t lcd_addr, uint8_t cmd);
void LCD_SendData(uint8_t lcd_addr, uint8_t data);
void LCD_SendString(uint8_t lcd_addr, char *str);

#endif
