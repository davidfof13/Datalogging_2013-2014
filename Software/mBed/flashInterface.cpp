#include "flashInterface.h"
//#include "sensorAPI.h"


void memdump( char *base, int n ) {
    
    unsigned int * p;

    //printf( "memdump from memory location 0x%08X for %d bytes", (unsigned long)base, n );
    
    p   = (unsigned int *)((unsigned int)base & ~(unsigned int)0x3);

    for(int i = 0; i < (n >> 2); i++, p++ ) {
       // if (!(i % 4))
            //printf( "\r\n  0x%08X :", (unsigned int)p );

        //printf( " 0x%08X", *p );
    }
    //printf( "\r\n" );
}

/* Reads a string from the flash */
void memCharDisplay( char *base, int n ) {
    
    char * p = base;
    char c;

    for(int i = 0; i < n; i++) {

        c = *p;            
        //printf( "%c", c);
        if(c == '\n')
            //printf("\n\rnew line detected");
        p++;
    }
    //printf( "\r\n" );
}

/*Erases the flash sector if necessary*/
void initializeFlashArea(){
    
    IAP iap;
    int r;
    //  blank check: The mbed will erase all flash contents after downloading new executable
    r   = iap.blank_check( TARGET_SECTOR, TARGET_SECTOR );
    //printf( "\r\nblank check result = 0x%08X\r\n", r );

    //  erase sector, if required 
    if ( r == SECTOR_NOT_BLANK ) {
        iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
        r   = iap.erase( TARGET_SECTOR, TARGET_SECTOR );
        //printf( "erase result  = 0x%08X\r\n", r );
    }
    
}

void writeToFlash(char data[], int offset){
    
    IAP iap;
    int r;
    // copy RAM to Flash
    iap.prepare( TARGET_SECTOR, TARGET_SECTOR );
    r   = iap.write( data, sector_start_adress[TARGET_SECTOR] + offset, JSON_BLOCK_SIZE );
    //printf( "\n\rcopied: SRAM(0x%08X)->Flash(0x%08X) for %d bytes. (result=0x%08X)", data, sector_start_adress[ TARGET_SECTOR ], JSON_BLOCK_SIZE, r );


    // compare
    r   = iap.compare( data, sector_start_adress[TARGET_SECTOR] + offset, JSON_BLOCK_SIZE );
    //printf( "\n\rcompare result = \"%s\"\r\n", r ? "FAILED" : "OK" );
    
    
    //printf( "\n\rshowing the flash contents of the buffer...\n\r");
    memCharDisplay(sector_start_adress[TARGET_SECTOR] + offset, JSON_BLOCK_SIZE);
    
    //wait(5.0);
    
    
}

void readDataFromFlash(char * dest , int offset){
    

    // get the start address of the next json string
   // char * startAddr = sector_start_adress[TARGET_SECTOR] + offset;

    char * tmp = sector_start_adress[TARGET_SECTOR] + offset;
    char c; // used to read one byte at a time from flash
    int i = 0;

    //printf("\r\nstarting to read at address 0x%08X", tmp);


    // read bytes into buffer until new line is found
     while( i <JSON_BLOCK_SIZE){  
        c  = *tmp;
        if (c == '\n'){
            ////printf("\n\rnew line detected");
            break;
        }
               
        dest[i] = c; 
        tmp++;
        i++;
     }
     //printf("\r\nfinished read");
}
