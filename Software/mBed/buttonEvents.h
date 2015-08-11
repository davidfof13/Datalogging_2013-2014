#include "mbed.h"
#include "rtos.h"
#include <stdarg.h>
#include "TextLCD.h"

#define LCD_TIMEOUT 5000

extern TextLCD lcd;
extern DigitalOut lcdBacklightEnable;
extern DigitalOut lcdTextEnable;
extern Mutex stdio_mutex;

extern InterruptIn b1;
extern InterruptIn b2;
extern InterruptIn b3;
extern InterruptIn b4;

extern DigitalOut d2;
extern DigitalOut d3;
extern DigitalOut d4;
extern DigitalOut d5;
//extern Serial pc;
extern RtosTimer * lcdTimer;
extern Mutex lcd_mutex;

void disable_lcd(void const * );
void LCD(const char *fmt, ...);

void flip1();
void flip2();
void flip3();
void flip4();

