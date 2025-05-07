#include "ssd1306.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

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

static i2c_inst_t *i2c_inst;

static void ssd1306_write_cmd(uint8_t cmd)
{
    uint8_t val[2] = { 0x80, cmd };
    i2c_write_blocking(i2c_inst, ADDR, val, sizeof(val), false);
}

void ssd1306_init(uint8_t width, uint8_t height, uint8_t scl, uint8_t sda, i2c_inst_t *i2c)
{
    i2c_init(i2c0, 400000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(sda, scl, GPIO_FUNC_I2C));

    i2c_inst = i2c;

    ssd1306_write_cmd(SET_DISP);
    ssd1306_write_cmd(SET_MEM_ADDR);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(SET_DISP_START_LINE);
    ssd1306_write_cmd(SET_SEG_REMAP | 0x01);
    ssd1306_write_cmd(SET_MUX_RATIO);
    ssd1306_write_cmd(height - 1);
    ssd1306_write_cmd(SET_COM_OUT_DIR | 0x08);
    ssd1306_write_cmd(SET_DISP_OFFSET);
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd((width > 2 * height) ? 0x02 : 0x12);
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