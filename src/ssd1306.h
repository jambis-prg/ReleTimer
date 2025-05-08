#pragma once

#include <stdint.h>
#include "hardware/i2c.h"

#define INVERT_SECONDS 0x01
#define INVERT_MINUTES 0x02
#define INVERT_HOURS 0x04

void ssd1306_init();
void ssd1306_power(bool on);
void ssd1306_update_timer(uint32_t timer, uint8_t invert);