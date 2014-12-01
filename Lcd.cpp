#include "GrowduinoFirmware.h"
//#include <LiquidCrystal.h>

#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

//extern LiquidCrystal lcd;
extern Adafruit_RGBLCDShield lcd;

char lcd_lines[LCD_BUFFER_LINES][17];
int lcd_last_printed_line, inserted_lines;
long lastrun;

void lcd_setup(){
    lcd.begin(16,2);
    lcd.setBacklight(0x07);
    lcd_flush();
    lcd_publish(F("Initialising LCD"));
    lcd_tick();
    lastrun = -1;

}
void lcd_publish(char * msg) {
    // Inserts msg into buffer
    Serial.print("lcd_publish: ");
    Serial.println(msg);
    if (inserted_lines < LCD_BUFFER_LINES) {
        strlcpy((char * ) lcd_lines[inserted_lines], msg, 17);
        inserted_lines += 1;
        lastrun = -1;
    } else {
        lcd_print_immediate(F("lcd buf overflow"));
    }
}

void lcd_publish(const __FlashStringHelper * msg) {
    // inserts msg into buffer from flash
    Serial.print("lcd_publish: ");
    Serial.println(msg);
    if (inserted_lines < LCD_BUFFER_LINES) {
        strlcpy_P((char * ) lcd_lines[inserted_lines], (char *) msg, 17);
        inserted_lines += 1;
        lastrun = -1;
    } else {
        lcd_print_immediate(F("lcd buf overflow"));
    }
}

void lcd_print_immediate(const __FlashStringHelper * msg) {
    // injects msg from flash just after buffer start and displays it
    if (inserted_lines > 1) {
        strlcpy((char * ) lcd_lines[0], lcd_lines[inserted_lines - 1],17);
        inserted_lines = 2;
    } else {
        inserted_lines += 1;
    }
    strlcpy_P((char * ) lcd_lines[inserted_lines - 1], (char *) msg, 17);
    Serial.println(lcd_lines[inserted_lines - 1]);
    lastrun = -1;
    lcd_last_printed_line = 0;
    
    lcd_tick();
}

void lcd_flush() {
    // cleans buffer
    lcd_last_printed_line = 0;
    lcd.setCursor(0, 0);
    for (int i =0; i < LCD_BUFFER_LINES; i++) {
        lcd_lines[i][0] = '\0';
    }
    inserted_lines = 0;
    lastrun = -1;
}

void lcd_tick() {
    long currrun = millis() / 2000;
    if (currrun != lastrun) {

        lastrun = currrun;
        lcd.clear();
        lcd.print(lcd_lines[lcd_last_printed_line]);

        if (inserted_lines > 1) {
            lcd_last_printed_line += 1;
            if (lcd_last_printed_line >= inserted_lines)
                lcd_last_printed_line = 0;
            lcd.setCursor(0, 1);
            lcd.print(lcd_lines[lcd_last_printed_line]);
        }
    } else {
    }
}
