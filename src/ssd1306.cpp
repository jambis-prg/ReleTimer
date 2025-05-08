#include "ssd1306.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"
#include <string.h>

#define ADDR 0x3C

// register definitions
#define SET_CONTRAST            0x81
#define SET_ENTIRE_ON           0xA4
#define SET_NORM_INV            0xA6
#define SET_DISP                0xAE
#define SET_MEM_ADDR            0x20
#define SET_COL_ADDR            0x21
#define SET_PAGE_ADDR           0x22
#define SET_DISP_START_LINE     0x40
#define SET_SEG_REMAP           0xA0
#define SET_MUX_RATIO           0xA8
#define SET_IREF_SELECT         0xAD
#define SET_COM_OUT_DIR         0xC0
#define SET_DISP_OFFSET         0xD3
#define SET_COM_PIN_CFG         0xDA
#define SET_DISP_CLK_DIV        0xD5
#define SET_PRECHARGE           0xD9
#define SET_VCOM_DESEL          0xDB
#define SET_CHARGE_PUMP         0x8D

#define WIDTH 128
#define HEIGHT 64

#define SCL 0 
#define SDA 1
#define I2C_INST i2c0

static uint8_t s_page_buffer[8] = {0};

static void ssd1306_write_cmd(uint8_t cmd)
{
    uint8_t val[2] = { 0x80, cmd };
    i2c_write_blocking(I2C_INST, ADDR, val, sizeof(val), false);
}

void ssd1306_init()
{
    i2c_init(I2C_INST, 400000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(SDA, SCL, GPIO_FUNC_I2C));

    ssd1306_write_cmd(SET_DISP);
    ssd1306_write_cmd(SET_MEM_ADDR);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(SET_DISP_START_LINE);
    ssd1306_write_cmd(SET_SEG_REMAP | 0x01);
    ssd1306_write_cmd(SET_MUX_RATIO);
    ssd1306_write_cmd(HEIGHT - 1);
    ssd1306_write_cmd(SET_COM_OUT_DIR | 0x08);
    ssd1306_write_cmd(SET_DISP_OFFSET);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd((WIDTH> 2 * HEIGHT) ? 0x02 : 0x12);
    ssd1306_write_cmd(SET_DISP_CLK_DIV);
    ssd1306_write_cmd(0x80);
    ssd1306_write_cmd(SET_PRECHARGE);
    ssd1306_write_cmd(0xF1); // 0x22 if self.external_vcc else 0xF1, ?
    ssd1306_write_cmd(SET_VCOM_DESEL);
    ssd1306_write_cmd(0x30);
    ssd1306_write_cmd(SET_CONTRAST);
    ssd1306_write_cmd(0xFF);
    ssd1306_write_cmd(SET_ENTIRE_ON);
    ssd1306_write_cmd(SET_NORM_INV);
    ssd1306_write_cmd(SET_IREF_SELECT);
    ssd1306_write_cmd(0x30);
    ssd1306_write_cmd(SET_CHARGE_PUMP);
    ssd1306_write_cmd(0x14); // 0x10 if self.external_vcc else 0x14 ?
    ssd1306_write_cmd(SET_DISP | 0x01);
}

void ssd1306_power(bool on)
{
    ssd1306_write_cmd(SET_DISP | on);
}

static void ssd1306_draw_page(uint8_t collumn, uint8_t number)
{
    ssd1306_write_cmd(SET_COL_ADDR);
    ssd1306_write_cmd(collumn);
    ssd1306_write_cmd(collumn + 7);

    ssd1306_write_cmd(SET_PAGE_ADDR);
    ssd1306_write_cmd(3);
    ssd1306_write_cmd(4);

    switch (number)
    {
    case 0:
        break;
    
    case 1:
        break;

    case 2:
        break;

    case 3:
        break;
        
    case 4:
        break;

    case 5:
        break;

    case 6:
        break;

    case 7:
        break;

    case 8:
        break;

    case 9:
        break;
    }

    i2c_write_blocking(I2C_INST, ADDR, s_page_buffer, sizeof(s_page_buffer), false);
}

void ssd1306_update_timer(uint32_t timer, uint8_t update_flags)
{
    uint8_t desconstructed_timer[3] = { (timer % 60), (timer / 60) % 60, (timer / 3600) };
    uint8_t i = 0;
    uint8_t column = 58;

    while (update_flags != 0)
    {
        if (update_flags & 1)
        {
            ssd1306_draw_page(column - 10, desconstructed_timer[i] / 10); // Dezenas
            ssd1306_draw_page(column, desconstructed_timer[i] % 10); // Unidades
        }

        update_flags >>= 1;
        column -= 10;
        i++;
    }
}
