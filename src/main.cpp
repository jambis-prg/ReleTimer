#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"


#define SW 29
#define DT 28
#define CLK 27

#define RELE 2

#define SW_DELAY_CURSOR 25
#define SW_DELAY_RUN 1000

enum CursorType : uint8_t 
{
     Seconds, 
     Minutes, 
     Hours,
     Count
};

bool running = false;
uint8_t hours, minutes, seconds;
uint32_t old_time;

uint32_t next_value = GPIO_IRQ_EDGE_FALL;

bool old_state = true;
uint32_t tDown = 0, tUp = 0;
CursorType cursor = Seconds;

bool update_lcd = false;

const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16

uint32_t millis()
{
    return to_ms_since_boot(get_absolute_time());
}

void encoder_callback(uint gpio, uint32_t events)
{
    if (gpio == SW && events != (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))
    {
        bool state = events == GPIO_IRQ_EDGE_FALL;

        if (state && !old_state) tDown = millis();
        else if (!state && old_state) tUp = millis();

        uint32_t delta = tUp - tDown;
        if (!state && old_state && delta > SW_DELAY_CURSOR)
        {
            if (delta > SW_DELAY_RUN)
            {
                if (running)
                {
                    seconds = old_time;
                    minutes = old_time >> 8;
                    hours = old_time >> 16;
                }
                else
                    old_time = seconds + (minutes << 8) + (hours << 16);

                running = !running;
                update_lcd = true;
                gpio_put(RELE, running);
            }
            else if (!running)
            {
                cursor = (CursorType)((cursor + 1) % CursorType::Count);
                update_lcd = true;
            }
        }

        old_state = state;
    }
    else if (!running && events != (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))
    {
        if (gpio == CLK && events == next_value)
        {
            next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

            if (cursor == Seconds)
            {
                seconds++;
                if (seconds >= 60)
                {
                    seconds = 0;
                    minutes++;

                    if (minutes >= 60)
                    {
                        hours = (hours + 1) % 24;
                        minutes = 0;
                    }
                }
            }
            else if (cursor == Minutes)
            {
                minutes++;

                if (minutes >= 60)
                {
                    hours = (hours + 1) % 24;
                    minutes = 0;
                }
            }
            else hours = (hours + 1) % 24;

            printf("%d:%d:%d\n", hours, minutes, seconds);
            update_lcd = true;
        }
        else if (gpio == DT && events == next_value)
        {
            next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

            if (cursor == Seconds)
            {
                seconds--;

                if (seconds >= 60)
                {
                    if (hours != 0 || minutes != 0)
                    {
                        seconds = 59;
                        minutes--;

                        if (minutes >= 60)
                        {
                            hours = hours > 0 ? hours - 1 : 0;
                            minutes = 59;
                        }
                    }
                    else
                        seconds = 0;
                }
            }
            else if (cursor == Minutes)
            {
                minutes--;

                if (minutes >= 60)
                {
                    if (hours != 0)
                    {
                        hours = hours > 0 ? hours - 1 : 0;
                        minutes = 59;
                    }
                    else
                        minutes = 0;
                }
            }
            else
                hours = hours > 0 ? hours - 1 : 0;
            update_lcd = true;

            printf("%d:%d:%d\n", hours, minutes, seconds);
        }
    }
}

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val) {
#ifdef i2c_default
    i2c_write_blocking(i2c0, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val) {
    // Toggle enable pin on LCD display
    // We cannot do this too quickly or things don't work
#define DELAY_US 600
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position) {
    int val = (line == 0) ? 0x80 + position : 0xC0 + position;
    lcd_send_byte(val, LCD_COMMAND);
}

static inline void lcd_char(char val) {
    lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s) {
    while (*s)
        lcd_char(*s++);
}

void lcd_init() {
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);

    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON, LCD_COMMAND);
    lcd_clear();
}

int main() {
    stdio_init_all(); // Inicializa a comunicação USB

    gpio_init(RELE);
    gpio_set_dir(RELE, GPIO_OUT);
    gpio_put(RELE, false);

    gpio_init(SW);
    gpio_init(DT);
    gpio_init(CLK);

    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);

    gpio_set_dir(DT, GPIO_IN);
    gpio_pull_up(DT);

    gpio_set_dir(CLK, GPIO_IN);
    gpio_pull_up(CLK);

    gpio_set_irq_enabled_with_callback(SW, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, encoder_callback);
    gpio_set_irq_enabled_with_callback(DT, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, encoder_callback);
    gpio_set_irq_enabled_with_callback(CLK, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, encoder_callback);

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);
    gpio_pull_up(0);
    gpio_pull_up(1);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(0, 1, GPIO_FUNC_I2C));

    lcd_init();

    lcd_clear();
    lcd_string("00:00:00");
    lcd_set_cursor(0, 7);

    uint32_t start = millis();
    uint16_t accum = 0;
    char buffer[10];
    while (true) 
    {
        uint32_t end = millis();
        uint32_t delta = end - start;
        start = end;

        if (running)
        {
            accum += delta;


            if (accum >= 1000)
            {
                update_lcd = true;
                accum -= 1000;
                seconds -= 1;
                if (seconds >= 60)
                {
                    if ((hours != 0 || minutes != 0))
                    {
                        seconds = 59;
                        minutes--;

                        if (minutes >= 60)
                        {
                            hours = hours > 0 ? hours - 1 : 0;
                            minutes = 59;
                        }
                    }
                    else
                    {
                        seconds = old_time;
                        minutes = old_time >> 8;
                        hours = old_time >> 16;
                        running = false;
                        gpio_put(RELE, false);
                    }
                }

                printf("%d:%d:%d\n", hours, minutes, seconds);
            }
        }
        
        if (update_lcd)
        {
            lcd_clear();
            sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
            lcd_string(buffer);
            switch (cursor)
            {
            case Seconds:
                lcd_set_cursor(0, 7);
                break;
            case Minutes:
                lcd_set_cursor(0, 4);
                break;
            case Hours:
                lcd_set_cursor(0, 1);
                break;
            default:
                break;
            }
            update_lcd = false;
        }
    }

    return 0;
}