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
    #define PAGES 8

    #define SCL 11
    #define SDA 10
    #define I2C_INST i2c1

    // Primeiro byte de controle 0x40
    static uint8_t s_page_buffer[9] = {0x40};
    static uint8_t s_subline_buffer[2] = {0x40, 0x00};
    static const uint8_t s_digits_8x8[10][8] = {
        // Dígito 0
        { 0x00, 0x3E, 0x7F, 0x49, 0x45, 0x7F, 0x3E, 0x00 },
        // Dígito 1
        { 0x00, 0x00, 0x42, 0x7F, 0x7F, 0x40, 0x00, 0x00 },
        // Dígito 2
        { 0x00, 0x42, 0x63, 0x71, 0x59, 0x4F, 0x46, 0x00 },
        // Dígito 3
        { 0x00, 0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00 },
        // Dígito 4
        { 0x00, 0x18, 0x1C, 0x16, 0x7F, 0x7F, 0x10, 0x00 },
        // Dígito 5
        { 0x00, 0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00 },
        // Dígito 6
        { 0x00, 0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00 },
        // Dígito 7
        { 0x00, 0x01, 0x71, 0x79, 0x0D, 0x07, 0x03, 0x00 },
        // Dígito 8
        { 0x00, 0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00 },
        // Dígito 9
        { 0x00, 0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00 },
    };

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

        ssd1306_write_cmd(SET_COM_OUT_DIR | (1 << 3));
        ssd1306_write_cmd(SET_SEG_REMAP | 1);

        memset(&s_page_buffer[1], 0x00, sizeof(s_page_buffer) - 1);

        for (uint8_t i = 0; i < WIDTH; i += 8)
        {
            for (uint8_t j = 0; j < PAGES; j++)
            {
                ssd1306_write_cmd(SET_COL_ADDR);
                ssd1306_write_cmd(i);
                ssd1306_write_cmd(i + 7);

                ssd1306_write_cmd(SET_PAGE_ADDR);
                ssd1306_write_cmd(j);
                ssd1306_write_cmd(j);
                
                i2c_write_blocking(I2C_INST, ADDR, s_page_buffer, sizeof(s_page_buffer), false);
            }
        }

        uint8_t collumn = 85;
        uint8_t dots[] = {0x40, 0x24, 0x24};
        for (uint8_t i = 0, collumn = 85; i < 2; i++, collumn -= 20)
        {
            ssd1306_write_cmd(SET_COL_ADDR);
            ssd1306_write_cmd(collumn - 12);
            ssd1306_write_cmd(collumn - 11);

            // Desenha na página 3
            ssd1306_write_cmd(SET_PAGE_ADDR);
            ssd1306_write_cmd(3);
            ssd1306_write_cmd(4);

            i2c_write_blocking(I2C_INST, ADDR, dots, sizeof(dots), false);
        }

    }

    void ssd1306_power(bool on)
    {
        ssd1306_write_cmd(SET_DISP | on);
    }

    static void ssd1306_draw_page(uint8_t collumn, uint8_t number, uint8_t inverted)
    {
        ssd1306_write_cmd(SET_COL_ADDR);
        ssd1306_write_cmd(collumn);
        ssd1306_write_cmd(collumn + 7);

        // Desenha na página 3
        ssd1306_write_cmd(SET_PAGE_ADDR);
        ssd1306_write_cmd(3);
        ssd1306_write_cmd(3);

        memcpy(&s_page_buffer[1], s_digits_8x8[number], sizeof(s_page_buffer) - 1);

        i2c_write_blocking(I2C_INST, ADDR, s_page_buffer, sizeof(s_page_buffer), false);

        for (uint8_t i = 0; i < 8; i++)
            s_page_buffer[i + 1] = inverted;

        ssd1306_write_cmd(SET_COL_ADDR);
        ssd1306_write_cmd(collumn);
        ssd1306_write_cmd(collumn + 7);

        // Desenha na página 4
        ssd1306_write_cmd(SET_PAGE_ADDR);
        ssd1306_write_cmd(4);
        ssd1306_write_cmd(4);

        i2c_write_blocking(I2C_INST, ADDR, s_page_buffer, sizeof(s_page_buffer), false);
    }

    void ssd1306_update_timer(uint32_t timer, uint8_t inverted)
    {
        uint8_t desconstructed_timer[3] = { (timer % 60), (timer / 60) % 60, (timer / 3600) };
        uint8_t column = 85;

        for (uint8_t i = 0; i < 3; i++)
        {
            ssd1306_draw_page(column - 10, desconstructed_timer[i] / 10, inverted & 1); // Dezenas
            ssd1306_draw_page(column, desconstructed_timer[i] % 10, inverted & 1); // Unidades

            column -= 20;
            inverted >>= 1;
        }
    }
