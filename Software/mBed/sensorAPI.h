#ifndef SENSOR_HEADER
#define SENSOR_HEADER

#include "mbed.h"
#include "rtos.h"
#include "Watchdog.h"

// uncomment this to display debbugging information from the i2c thread
//#define __SENSOR_LOG__
#define DEFAULT_SAMPLE_RATE 480 // in mins
#define DEFAULT_SAMPLING_FREQ 10000.0// (in Hz)
#define AC_WAVEFORM_PERIOD 0.02  // we assume a mains input frequency of 50Hz for the AC signal
// => f = 50Hz <=> T = 20ms

#ifdef __SENSOR_LOG__
#define SLOG(...) do {stdio_mutex.lock(); \
                      printf("\n\r"); \
                      printf(__VA_ARGS__); \ 
                      stdio_mutex.unlock(); \
                  } while(0) // sensor log
#else
#define SLOG(...) ((void)0)
#endif

// I2C 8bit read mode addresses for each board.
const int ADC1 = 0x40; // AC board
const int ADC2 = 0x42; //DC voltage board
const int ADC3 = 0x44; //DC current board

const float vref = 5.0; // Voltage reference
const float res = 1024.0; // ADC resolution

//extern Serial pc;
extern Mutex stdio_mutex;
extern DigitalOut SDEnable;
extern Mutex sd_mutex;
extern unsigned int lines; 
extern Watchdog wtd;
extern unsigned int sampleRate;
extern unsigned int samplingFreq;

struct sensorBoard {
    
    // path in the SD card
    char * boardName;
    
    // ADC address
    const int adcAddr;
    
    // Indicates the types of waveforms sensed by
    // the boards (ac or dc)
    char boardType[3];    
};

// Perform sensor output readings and saves them to SD card
void collectSensorData(void const *);
float getSensorValue(const int adc, int input, char * signalType);
void createSDdirectories();
float ADCread(const int addr, int input, float ts=0.001);
void addJsonField(char * json_string, int fieldType, const char * fmt, ...);
void setJson(char[], float[], float[], float[], int);
float toAnalogue(int x);
float acVpp(int );


#endif