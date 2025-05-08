#include <stdio.h>
#include "pico/stdlib.h"
#include "ssd1306.h"

#define SW 29
#define DT 28
#define CLK 27

#define RELE 2
#define BUZZER 13

#define SW_DELAY_CURSOR 25
#define SW_DELAY_RUN 1000

#define DAY 86400
#define HOUR 3600
#define MINUTE 60
#define SECOND 1

bool running = false;
uint32_t next_value = GPIO_IRQ_EDGE_FALL;

bool old_state = true;
uint32_t tDown = 0, tUp = 0;

bool gate_a = true;
bool gate_b = true;

bool update_lcd = false;

uint32_t timer_uint = 0, old_timer_uint = 0;
uint16_t timer_incrementer = 1;
uint8_t selected = INVERT_SECONDS;

bool reset = false;

uint32_t millis()
{
    return to_ms_since_boot(get_absolute_time());
}

void sw_handler(bool state)
{
    if (state && !old_state) tDown = millis();
    else if (!state && old_state) tUp = millis();

    uint32_t delta = tUp - tDown;
    if (!state && old_state && delta > SW_DELAY_CURSOR)
    {
        if (delta > SW_DELAY_RUN && timer_uint != 0)
        {
            if (running) timer_uint = old_timer_uint;
            else old_timer_uint = timer_uint;

            running = !running;
            update_lcd = true;
            gpio_put(RELE, running);
            gpio_put(BUZZER, false);
        }
        else if (running)
        {
            timer_uint = old_timer_uint;
            update_lcd = true;
            reset = true;
        }
        else
        {
            timer_incrementer = timer_incrementer == 3600 ? 1 : timer_incrementer * 60;
            update_lcd = true;
            selected = selected >= INVERT_HOURS ? INVERT_SECONDS : selected << 1;
        }
    }

    old_state = state;
}



void rot_handler(uint gpio, uint32_t events)
{
    if (gpio == CLK && events == next_value)
    {
        gate_a = (next_value != GPIO_IRQ_EDGE_FALL);

        next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

        if (gate_a != gate_b)
        {
            timer_uint = timer_uint + timer_incrementer;
            if (timer_uint >= DAY)
                timer_uint = DAY - 1;
            update_lcd = true;
        }
    }
    else if (gpio == DT && events == next_value)
    {
        gate_b = (next_value != GPIO_IRQ_EDGE_FALL);
        
        next_value = next_value ^ (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);

        if (gate_a != gate_b)
        {
            timer_uint -= timer_incrementer;
            // Overflow
            if (timer_uint >= DAY)
                timer_uint = 0;
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

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, false);

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

    ssd1306_init();
    ssd1306_update_timer(0, selected);

    uint32_t start = millis();
    uint16_t accum = 0, beep_accum = 0;
    bool beeping = false;
    while (true) 
    {
        uint32_t end = millis();
        uint32_t delta = end - start;
        start = end;

        if (running)
        {
            if (timer_uint >= DAY)
            {
                timer_uint = old_timer_uint;
                running = false;
                gpio_put(RELE, false);
                gpio_put(BUZZER, false);
                accum = beep_accum = 0;
                beeping = false;
                update_lcd = true;
            }
            else
            {
                accum += delta;
                if (accum >= 1000)
                {
                    update_lcd = true;
                    accum -= 1000;
                    timer_uint--;
                }

                if (reset)
                {
                    gpio_put(BUZZER, false);
                    accum = beep_accum = 0;
                    beeping = false;
                    reset = false;
                }

                if (timer_uint < MINUTE)
                {
                    beep_accum += delta;

                    if (!beeping && beep_accum >= 5000)
                    {
                        gpio_put(BUZZER, true);
                        beeping = true;
                        beep_accum -= 5000;
                    }
                    else if (beeping && beep_accum >= 500)
                    {
                        gpio_put(BUZZER, false);
                        beeping = false;
                        beep_accum -= 500;
                    }
                }
            }
        }
        else
        {
            accum = beep_accum = 0;
            beeping = false;
        }
        
        if (update_lcd)
        {
            ssd1306_update_timer(timer_uint, selected);
            update_lcd = false;
        }
    }

    return 0;
}