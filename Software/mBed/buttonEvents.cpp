#include "buttonEvents.h"

void flip1() {
    d2 = !d2;
}

void flip2() {
    d3 = !d3;
}

void flip3() {
    d3 = !d3;
}

void flip4() {
    d4 = !d4;
}

void disable_lcd(void const * args){
    
    
    lcdBacklightEnable = 1;
    lcdTextEnable = 0;
    
}

void LCD(const char *fmt, ...){
    
     lcd_mutex.lock();
    
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    // enable lcd
    lcdBacklightEnable = 0;
    lcdTextEnable = 1;
    
    lcd.cls(); // clear the screen
    lcd.printf("%s\n", fmt); // display message
    

    // Start timeout
    lcdTimer->start(LCD_TIMEOUT);
    
    lcd_mutex.unlock();

    
}
