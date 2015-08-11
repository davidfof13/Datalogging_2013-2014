#include "Watchdog.h"
#include "mbed.h"

// Load timeout value in watchdog timer and enable
void Watchdog::kick(float s) {
   //LPC_WDT->WDCLKSEL = 0x1;                // Set CLK src to PCLK
   LPC_WDT->WDCLKSEL = 0x02;               // Set CLK src to RTC for DeepSleep wakeup
   //uint32_t clk = SystemCoreClock / 16;    // WD has a fixed /4 prescaler, PCLK default is /4
  // LPC_WDT->WDTC = s * (float)clk;
   LPC_WDT->WDTC = (s/4.0)*32768;
   LPC_WDT->WDMOD = 0x3;                   // Enabled and Reset
   kick();
}
    
// "kick" or "feed" the dog - reset the watchdog timer by writing this required bit pattern
void Watchdog::kick() {
   LPC_WDT->WDFEED = 0xAA;
   LPC_WDT->WDFEED = 0x55;
}