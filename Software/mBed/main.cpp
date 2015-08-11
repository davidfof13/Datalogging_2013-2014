#include "mbed.h"
#include "rtos.h"
#include "modemAPI.h"
#include "sensorAPI.h"
#include "SDFileSystem.h"
#include "buttonEvents.h"
#include "systemConfiguration.h"
#include "PowerControl.h"
#include "IAP.h"

// uncomment this to display debbugging information from the main thread
//#define __MAIN_LOG__

#ifdef __MAIN_LOG__
#define MLOG(...) do {stdio_mutex.lock(); \
                      printf("\n\r"); \
                      printf(__VA_ARGS__); \
                      stdio_mutex.unlock(); \
                 } while(0)
#else
#define MLOG(...) ((void)0) //do nothing
#endif

// diodes
DigitalOut d2(p21); // indicates if system is in rest mode
DigitalOut d3(p22); 
DigitalOut d4(p23);
DigitalOut d5(p24); // indicates if rtc has been set

// switches
InterruptIn b1(p10);
InterruptIn b2(p11);
InterruptIn b3(p12);
InterruptIn b4(p13);

//SD enable
DigitalOut SDEnable(p9);
DigitalOut myLED(LED1);
DigitalOut myled2(LED2);

//LCD
DigitalOut lcdBacklightEnable(p14);
DigitalOut lcdTextEnable(p25);


// modem 
osThreadId modemId;
DigitalOut modemEnable(p30); // modem enable pin (active high)

// queue of modem requests
Queue<ModemRequest, 1> modem_queue;

// LDC object
TextLCD lcd(p15, p16, p17, p18, p19, p20);

Mutex queue_mutex;
Mutex stdio_mutex;
Mutex sd_mutex; // SD card mutex
Mutex lcd_mutex;

// settings stored in configuration file
bool rtc; // indicate if we have to set the rtc
unsigned int modem_thread_stack_size;

bool upload; // indicates if we have to upload data
bool remote; // indicates if we have to fetch remote settings
bool SDFileSystemCreated;

uint8_t modem_attempts; // maximum use of the modem
int download_timeout;  // time remaining before download occurs
int upload_timeout; // time remaining before upload occurs
unsigned int download_rate; // rate at which we get remote settings 
unsigned int upload_rate; // rate at which we upload data in(secs)
unsigned int sampleRate; // rate at which we read samples (in mins)
unsigned int samplingFreq; // sampling frequency for AC signals
unsigned int lines; // number of instances of sensor readings that have been uploaded
// for the current file

RtosTimer * uploadTimer;
RtosTimer * remoteSettingsTimer;
RtosTimer * lcdTimer;
RtosTimer * modemTimer; // times the usage of the modem in general

// times at which our activites start
struct tm dayStart;
struct tm dayEnd;

Watchdog wtd; // watchdog timer


int main() {
    
    MLOG("\x1B[2J");
    MLOG("\n\rStart");

    powerDownInterfaces();
    lcdBacklightEnable = 1;
    SDFileSystemCreated = false;
    
    // every hour
    wtd.kick(3600.0);
    
    // define LCD timer
    lcdTimer = new RtosTimer(disable_lcd, osTimerOnce, (void *)NULL);
    //LCD("Start");
    
    // set button interrupts
    b3.rise(&flip3);
    b4.rise(&flip4);
    
    // set default parameters
    MLOG("set default parameters");
    setDefaultParameters();
    
    // Open local file system to read settings
    LocalFileSystem local("local");

    MLOG("readings system parameters...\n");
    readParameters();
    MLOG("\nfinished setting flags");
    
    MLOG("-----------------------------");
        
    // choose the amount of memory to allocate to the modem thread stack
    // depending on whether we just want to set the RTC
    // or we want to transfer sensor values or fetch remote settings
     modem_thread_stack_size = (upload)? 1200*4:(remote)? 1150*4: 950*4;
    
    // if there's a modem request, create the thread
    if(upload || remote || rtc){
        Thread modemThread(dataTransfer, NULL, osPriorityNormal, modem_thread_stack_size);
        MLOG("Modem thread : %d bytes", modem_thread_stack_size);
        MLOG("Modem attempts: %d\n\r", modem_attempts);
        
        ModemRequest request = IDLE;
        
        // upload data
        if(upload){
            request = UPLOAD;   
            LCD( "Uploading data  to server");
        }
            
        // fetch remote settings
        else if(remote){ // we don't set the RTC when we want to upload data
            request = REMOTE_SETTINGS;
            LCD("Fetching remote settings");
        }
        
        // set rtc
        if(rtc){ // is there a request to set the rtc?
            request = NTP_TIME;
            LCD("Getting NTP time");
        }
            
        // send request to modem thread
        if(request != IDLE){
            queue_mutex.lock();
            modem_queue.put(&request);
            queue_mutex.unlock();
        }
        
        // wait for thread to finish executing
        while(modemThread.get_state() > 0);
        
        modemThread.terminate();
        MLOG("modem thread has been terminated\n");
        reset_system();        
    
    } else { // or else get sensor values 
        
            // system should only operate within a certain
            // time period.
            // Note: The flash will always contain the latest data
            // taken from the sensors when the upload timeout occurs

            int r;
            r = restPeriod();
            if(r > 0){ // Put system in rest mode
                wtd.kick(r);
                
                d2 = 1;
                LCD("DeepSleep");
                wait(LCD_TIMEOUT);
                
                // Power level in DeepSleep around 135mW (27MA at 5VDC)
                DeepSleep();   
            }

            int uploadDelay ,downloadDelay = 0;
            
            // set upload and download timers
            uploadTimer = new RtosTimer(setDataUpload,osTimerOnce, (void *)(&uploadDelay));
            remoteSettingsTimer = new RtosTimer(setDataDownload,osTimerOnce, (void *)(&downloadDelay));
            
            // now start timers
            setTimers(&uploadDelay, &downloadDelay);
            
            MLOG("creating SD file system");
            
            // create SD file system
            SDFileSystem sd(p5, p6, p7, p8, "sd");
            SDFileSystemCreated = 1;
            
            MLOG("before creating directories");
            
            // create directories
            createSDdirectories();
            
            LCD("Sampling sensor values");
            
            // collect sensor values
            collectSensorData((void *)NULL);
    } // end else 
    
 
}
