#ifndef MODEM_HEADER
#define MODEM_HEADER

#include "mbed.h"
#include "rtos.h"
#include "VodafoneUSBModem.h"
#include "HTTPClient.h"
#include "NTPClient.h"
#include "Watchdog.h"


#define MAX_CONNECT 3 // max number of connection tries
#define QUEUE_TIMEOUT 5000
#define DEFAULT_UPLOAD_RATE 3600 // in secs
#define DEFAULT_DOWNLOAD_RATE 4800 
#define MODEM_CONNECT_TIMEOUT 60.0
//#define APN "pp.vodafone.co.uk"
#define APN "internet.mtn"

//#define USSD_COMMAND_BALANCE "*#134#" 
//#define USSD_COMMAND_BYTES "*#135#" 
#define USSD_COMMAND_BYTES "*110#"

extern RtosTimer * uploadTimer;
extern RtosTimer * remoteSettingsTimer;
static const char * KIOSK = "Batima";

// uncomment this to display debbugging information from the modem thread
//#define __MODEM_LOG__

#ifdef __MODEM_LOG__

#define CLOG(...) do {stdio_mutex.lock(); \
                      printf("\n\r"); \
                      printf(__VA_ARGS__); \
                      stdio_mutex.unlock(); \
                 } while(0)
#else
#define CLOG(...) ((void)0)
#endif

extern uint8_t modem_attempts; // maximum use of the modem

//List of modem actions
typedef enum ModemRequest {IDLE, UPLOAD, NTP_TIME, REMOTE_SETTINGS} ModemRequest;

extern Watchdog wtd;
extern Queue<ModemRequest, 1> modem_queue;

extern DigitalOut modemEnable;

extern Mutex queue_mutex;
extern Mutex stdio_mutex;
extern Mutex sd_mutex;

extern bool SDFileSystemCreated;
extern bool upload;

extern int upload_timeout;
extern unsigned int upload_rate;
extern int download_timeout;
extern unsigned int download_rate;

void dataTransfer(void const *);
static void getRemoteSettings();
static void sendData();
static void sendLog(VodafoneUSBModem *);
static int sendUSSDCommand(VodafoneUSBModem *, char * , char *, int);
static void getNTPtime();

void setDataUpload(void const * arg);
void setDataDownload(void const * arg);


#endif