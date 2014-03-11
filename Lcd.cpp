#include "GrowduinoFirmware.h"
#include <LiquidCrystal.h>
extern LiquidCrystal lcd;

char lcd_lines[LCD_BUFFER_LINES][17];
int lcd_last_printed_line, lcd_buffer_line;
long lastrun;

void lcd_setup(){
    
    lcd.begin(16,2);
    lcd_flush();
    lcd.print(F("Grow! "));
    lastrun = -1;
}
void lcd_publish(char * msg) {
    strlcpy((char * ) lcd_lines[lcd_buffer_line], msg, 17);
    lcd_buffer_line += 1;
    if (lcd_buffer_line == LCD_BUFFER_LINES)
        lcd_buffer_line = 0;
}
void lcd_flush() {
    lcd_last_printed_line = 0;
    lcd_last_printed_line=0;
    lcd.setCursor(0, 0);
    for (int i =0; i < LCD_BUFFER_LINES; i++) {
        lcd_lines[i][0] = '\0';
    }
    lcd_buffer_line = 0;
}
void lcd_tick() {
    long currrun = millis() / 5000;
    if (currrun != lastrun) {
        lastrun = currrun;
        lcd.clear();
        lcd.print(lcd_lines[lcd_last_printed_line]);

        lcd_last_printed_line += 1;
        if (lcd_last_printed_line >= lcd_buffer_line)
            lcd_last_printed_line = 0;
        lcd.setCursor(0, 1);
        lcd.print(lcd_lines[lcd_last_printed_line]);
    }
}
