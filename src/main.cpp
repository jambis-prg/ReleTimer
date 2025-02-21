#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "lcd.h"

#define SW 29
#define DT 28
#define CLK 27

#define RELE 2

#define SW_DELAY_CURSOR 25
#define SW_DELAY_RUN 1000

typedef struct
{
    uint8_t hours, minutes, seconds;
} Time;

enum CursorType : uint8_t 
{
     Seconds, 
     Minutes, 
     Hours,
     Count
};

bool running = false;
Time timer;
uint32_t old_time;

uint32_t next_value = GPIO_IRQ_EDGE_FALL;

bool old_state = true;
uint32_t tDown = 0, tUp = 0;
CursorType cursor = Seconds;

bool update_lcd = false;

uint32_t millis()
{
    return to_ms_since_boot(get_absolute_time());
}


void inc_timer(Time *timer, CursorType cursor)
{
    switch (cursor)
    {
    case Seconds:
        timer->seconds++;
        if (timer->seconds < 60)
            break;
        timer->seconds = 0;
    case Minutes:
        timer->minutes++;
        if (timer->minutes < 60)
            break;
        timer->minutes = 0;
    case Hours:
        timer->hours = (timer->hours + 1) % 24;
        break;
    default:
        break;
    }
}

void dec_timer(Time *timer, CursorType cursor)
{
    switch (cursor)
    {
    case Seconds:
        timer->seconds--;
        if (timer->seconds < 60)
            break;
        timer->seconds = 59;
    case Minutes:
        timer->minutes--;
        if (timer->minutes < 60)
            break;
        timer->minutes = 59;
    case Hours:
        timer->hours = timer->hours > 0 ? timer->hours - 1 : 23;
        break;
    default:
        break;
    }
}

bool reset = false;

void sw_handler(bool state)
{
    if (state && !old_state) tDown = millis();
    else if (!state && old_state) tUp = millis();

    uint32_t delta = tUp - tDown;
    if (!state && old_state && delta > SW_DELAY_CURSOR)
    {
        if (delta > SW_DELAY_RUN)
        {
            if ((timer.hours + timer.minutes + timer.seconds) != 0)
            {
                if (running)
                {
                    timer.seconds = old_time;
                    timer.minutes = old_time >> 8;
                    timer.hours = old_time >> 16;
                }
                else
                    old_time = timer.seconds + (timer.minutes << 8) + (timer.hours << 16);

                running = !running;
                update_lcd = true;
                gpio_put(RELE, running);
            }
        }
        else if (running)
        {
            timer.seconds = old_time;
            timer.minutes = old_time >> 8;
            timer.hours = old_time >> 16;
            update_lcd = true;
            reset = true;
        }
        else
        {
            cursor = (CursorType)((cursor + 1) % CursorType::Count);
            update_lcd = true;
        }
    }

    old_state = state;
}

int a = 1;
int b = 1;

void rot_handler(uint gpio, uint32_t events)
{
    if (gpio == CLK && events == next_value)
    {
        a = (next_value == GPIO_IRQ_EDGE_FALL) ? 0 : 1;

        next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

        if (a != b)
        {
            inc_timer(&timer, cursor);
            printf("%02d:%02d:%02d\n", timer.hours, timer.minutes, timer.seconds);
            update_lcd = true;
        }

    }
    else if (gpio == DT && events == next_value)
    {
        b = (next_value == GPIO_IRQ_EDGE_FALL) ? 0 : 1;
        
        next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

        if (a != b)
        {
            dec_timer(&timer, cursor);
            printf("%02d:%02d:%02d\n", timer.hours, timer.minutes, timer.seconds);
            update_lcd = true;
        }

    }
}

void encoder_callback(uint gpio, uint32_t events)
{
    if (events != (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))
    {
        if(gpio == SW)
            sw_handler(events == GPIO_IRQ_EDGE_FALL);
        else if (!running)
            rot_handler(gpio, events);
    }
}


void init_gpios()
{
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
}

int main() {
    stdio_init_all(); // Inicializa a comunicação USB

    init_gpios();

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
            if (reset)
            {
                accum = 0;
                reset = false;
            }

            accum += delta;


            if (accum >= 1000)
            {
                update_lcd = true;
                accum -= 1000;

                if (timer.hours == 0 && timer.minutes == 0 && timer.seconds == 1)
                {
                    timer.seconds = old_time;
                    timer.minutes = old_time >> 8;
                    timer.hours = old_time >> 16;
                    running = false;
                    gpio_put(RELE, false);
                }
                else
                    dec_timer(&timer, Seconds);

                printf("%02d:%02d:%02d\n", timer.hours, timer.minutes, timer.seconds);
            }
        }
        
        if (update_lcd)
        {
            lcd_clear();
            sprintf(buffer, "%02d:%02d:%02d", timer.hours, timer.minutes, timer.seconds);
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