#pragma once

#include <stdint.h>

#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYLEFT 0x02

// flags for display and cursor control
#define LCD_BLINKON 0x01
#define LCD_CURSORON 0x02
#define LCD_DISPLAYON 0x04

// flags for display and cursor shift
#define LCD_MOVERIGHT 0x04
#define LCD_DISPLAYMOVE 0x08

// flags for function set
#define LCD_5x10DOTS 0x04
#define LCD_2LINE 0x08
#define LCD_8BITMODE 0x10

// flag for backlight control
#define LCD_BACKLIGHT 0x08

#define LCD_ENABLE_BIT 0x04

// By default these LCD display drivers are on bus address 0x27
#define LCD_ADDR 0x27

// Modes for lcd_send_byte
#define LCD_CHARACTER  1
#define LCD_COMMAND    0

#define MAX_LINES      2
#define MAX_CHARS      16

#define LCD_DELAY_US 600


/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val); 

void lcd_toggle_enable(uint8_t val); 

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode); 

void lcd_init(); 

void lcd_clear(void); 

// go to location on LCD
void lcd_set_cursor(int line, int position); 

void lcd_char(char val); 

void lcd_string(const char *s);