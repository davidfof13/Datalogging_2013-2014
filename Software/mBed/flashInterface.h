#ifndef FLASH_INTERFACE_H
#define FLASH_INTERFACE_H

#include "IAP.h"

void memdump(char *base, int n);
void memCharDisplay( char *base, int n );
void initializeFlashArea();
void writeToFlash(char data[], int offset);
void readDataFromFlash(char * dest , int offset);



#endif