#pragma once

#include <stdint.h>

void ssd1306_init(uint8_t width, uint8_t height, uint8_t scl, uint8_t sda, uint8_t I2C_PORT);
void ssd1306_power(bool on);
void ssd1306_contrast(uint8_t contrast);
void ssd1306_invert(uint8_t invert);
void ssd1306_rotate(uint8_t rotate);
void ssd1306_clear();
void ssd1306_show();