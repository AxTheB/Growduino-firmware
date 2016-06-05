#include "GrowduinoFirmware.h"
//#include <LiquidCrystal.h>

#ifdef DISPLAY_2004

#include <LiquidCrystal_I2C.h>
extern LiquidCrystal_I2C lcd;

#else

#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
extern Adafruit_RGBLCDShield lcd;

#endif


char lcd_lines[LCD_BUFFER_LINES][17];
int lcd_last_printed_line, inserted_lines;
long lastrun;

void lcd_setup(){
#ifdef DISPLAY_2004
    lcd.init();
    lcd.backlight();
#else 
    lcd.begin(16,2);
    lcd.setBacklight(0x07);
#endif

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

void lcd_publish(const char * text, const char * format, int data) {
    lcd_publish(text, format, data, 0);
}

void lcd_publish(const char * text, const char * format, int data, int divisor) {
    char lcd_msg[18];
    if (data == MINVALUE) {
        snprintf(lcd_msg, 17, "%s read error", text);
    } else {
        if (divisor == 0) {
            snprintf(lcd_msg, 17, format, text, data);
        } else {
            snprintf(lcd_msg, 17, format, text, data / divisor, abs(data % divisor));
        }
    }
    lcd_publish(lcd_msg);
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
    long currrun = millis() / (500 * LCD_DISPLAY_LINES);
    if (currrun != lastrun) {

        lastrun = currrun;
        lcd.clear();

        int lines_to_print = min(LCD_DISPLAY_LINES, inserted_lines);
#ifdef DEBUG_LCD
        Serial.print(F("Lines to print: "));
        Serial.print(lines_to_print);
#endif

        for (int i=0; i < lines_to_print; i++) {

            lcd.setCursor(0, i);
            lcd.print(lcd_lines[lcd_last_printed_line]);
            lcd_last_printed_line += 1;
            if (lcd_last_printed_line >= inserted_lines)
                lcd_last_printed_line = 0;
        }
    }
}
