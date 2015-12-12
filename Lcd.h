#pragma once

void lcd_setup();
void lcd_publish(char * msg);
void lcd_publish(const __FlashStringHelper * msg);
void lcd_publish(const char * text, const char * format, int data);
void lcd_publish(const char * text, const char * format, int data, int divisor);
void lcd_print_immediate(const __FlashStringHelper * msg);
void lcd_flush();
//void lcd_header(char * msg);
void lcd_tick();
