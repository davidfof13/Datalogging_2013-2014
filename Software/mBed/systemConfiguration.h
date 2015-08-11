#ifndef _config_header
#define _config_header

#include "mbed.h"
#include "modemAPI.h"
#include "sensorAPI.h"


// Rwanda Timezone : UTC + 2
#define TIMEZONE_OFFSET 2
//#define __SYSLOG__

#ifdef __SYSLOG__
#define SYSLOG(...) do {stdio_mutex.lock(); \
                      printf("\n\r"); \
                      printf(__VA_ARGS__); \ 
                      stdio_mutex.unlock(); \
                  } while(0) // sensor log
                              
#else
#define SYSLOG(...) ((void)0)
#endif

// default period of time during which daily tasks 
// should be performed
// Note: these times correspond to the 
// timezone provided
#define DEFAULT_DAY_START "05:30"
#define DEFAULT_DAY_END "18:00"
#define MAX_modem_attempts 3 // maximum consecutive uses of the modem
#define USR_POWERDOWN (0x104)

extern struct tm dayStart;
extern struct tm dayEnd;

extern DigitalOut d5;

extern "C" void mbed_reset();

//extern Serial pc;
extern bool rtc;

// to keep track of the number of lines in the file
extern unsigned int lines;

// rates
extern unsigned int download_rate;
extern unsigned int upload_rate;
extern int download_timeout;
extern int upload_timeout;
extern unsigned int sampleRate;
extern unsigned int samplingFreq;

extern uint8_t modem_attempts;

// flags
extern bool upload;
extern bool remote;
extern bool rtc_set;

// timers
extern RtosTimer * remoteSettingsTimer;
extern RtosTimer * uploadTimer;
extern RtosTimer * lcdTimer;

// configuration file path
static const char * CONFIG_FILE_NAME = "/local/config.txt";
static const char * UPLOAD_FILE_NAME = "/local/upload.txt";



// parsing parameters from file
void readParameters();
int is_empty(const char *s);
char** tokenize(const char* input, const char * separator);
char *strdup (const char *s);

// setting the parameters
void setDefaultParameters();

static void setUploadRate(char *);
static void setRemoteRate(char *);
static void setUploadTimeout(char *);
static void setRemoteTimeout(char *);
static void setModemAttempts(char *);
static void setSamplingFreq(char *);

static void setSampleRate(char *);
static void setUpload(char *);
static void setRemote(char * str);
static void setLineCount(char * str);
static void setRTC(char *);

// reset related actions
void reset_system();
static void clear_file();
static void addSetting(char * setting, char * value);
static void saveSettings();

// time limits
int restPeriod();
static void setTimeBoundaries(struct tm * tlimit, const char * t);
void setDayStart(const char *);
void setDayEnd(const char *);
static bool RTCisSet();

// timer interrupt functions
void uploadData(void const * arg);
void setTimers(int *, int * );

// Power control
void powerDownInterfaces();
int semihost_powerdown();


#endif