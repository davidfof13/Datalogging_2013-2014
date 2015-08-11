#include "systemConfiguration.h"
#include <ctype.h>
#include "PowerControl/PowerControl.h"
#include "PowerControl/EthernetPowerControl.h"


void setDefaultParameters(){
    
    upload_rate = DEFAULT_UPLOAD_RATE;
    download_rate = DEFAULT_DOWNLOAD_RATE;
    upload_timeout = upload_rate;
    download_timeout = download_rate;
    samplingFreq = DEFAULT_SAMPLING_FREQ;
    modem_attempts = 0;
    
    sampleRate = DEFAULT_SAMPLE_RATE;
    rtc = 0;
    upload = false;
    remote = false;
    lines = 0; 
        
    // check if the rtc has been set
    if (!RTCisSet()){
        SYSLOG("rtc not set");
        rtc = 1;
    }
    
    // set status leds
    d5 = RTCisSet();

    // get the current time
    time_t seconds = time(NULL);
    
    // initialise them to the same date
    dayStart = *localtime(&seconds);
    dayEnd = *localtime(&seconds);
    
    // set the default time boundaries
    setTimeBoundaries(&dayStart, DEFAULT_DAY_START);
    setTimeBoundaries(&dayEnd, DEFAULT_DAY_END);
    
    SYSLOG("dayStart: %02d:%02d ", dayStart.tm_hour, dayStart.tm_min);
    SYSLOG("dayEnd %02d:%02d ", dayEnd.tm_hour, dayEnd.tm_min);

}


/*Check flags inside configuration file */
void readParameters(){
    
    FILE *fp = fopen(CONFIG_FILE_NAME ,"r");
    char line[50];

    if (fp != NULL){

        int i = 0;
        while(fgets(line, 100, fp) != NULL){

            char * tmp = (char *)malloc(strlen(line)*sizeof(char));
            SYSLOG("line %d : %s", i, line );

            // Omit empty lines and comments
            if(strchr(line, '#') == NULL && !is_empty(line)){
                
                strcpy(tmp, line);

                // split line into words
                char ** tokens = tokenize(tmp, " ");
                char * var = *tokens;
                char * value = *(tokens + 2);
                
                if(!strcmp(var, "rtc")){   
                    setRTC(value);   
                }
                
                else if (!strcmp(var, "uploadRate")){
                    setUploadRate(value);   
                }
                
                else if (!strcmp(var, "downloadRate")){
                    setRemoteRate(value);   
                }
                
                else if (!strcmp(var, "uploadTimeout")){
                    setUploadTimeout(value);   
                }
                
                else if (!strcmp(var, "downloadTimeout")){
                    setRemoteTimeout(value);   
                }
                
                else if (!strcmp(var, "upload")) {
                    setUpload(value);
                }
                
                else if (!strcmp(var, "remote")){   
                    setRemote(value);   
                }
            
                else if (!strcmp(var, "sampleRate")){
                    setSampleRate(value);   
                }
                
                else if(!strcmp(var, "lines")){
                    setLineCount(value);
                }
                
                else if(!strcmp(var, "dayStart")){
                    setDayStart(value);
                }
                
                else if(!strcmp(var, "dayEnd")){
                    setDayEnd(value);
                }
                 else if(!strcmp(var, "modemAttempts")){
                    setModemAttempts(value);
                }
                else if(!strcmp(var, "samplingFreq")){
                    setSamplingFreq(value);   
                }
                
                // delete words
                char** it;
                for(it=tokens; it && *it; ++it)
                {
                    free(*it);
                }
                
                free(tokens);
                i++;
            }

            free(tmp);
        } // end while



        fclose(fp);

    } else {
        SYSLOG("Couldn't open file");
    }
    
    // ensure that they don't happen at the same time
    if(upload_timeout == download_timeout)
        download_timeout += 2;
        
    // if we're going to connect to the network,
    // we need to update the configuration file now
    // as the watchdog timer may reset the system
    // and the file wouldn't be updated
    if(remote || upload){
        
        clear_file();
        
        // count the number of times modem is used
        modem_attempts++;
        
        if(modem_attempts > MAX_modem_attempts){
            modem_attempts = 0;
            rtc = 0;
            upload = 0;
            remote = 0;
            if(upload)
                lines = 0;
        }
        
        // add all settings
        saveSettings();
           
    }
        
        
}


/*Checks if a string is empty
link: http://stackoverflow.com/questions/3981510/getline-check-if-line-is-whitespace */
int is_empty(const char *s) {
  while (*s != '\0') {
    if (!isspace((unsigned char)*s))
      return 0;
    s++;
  }
  return 1;
}

char** tokenize(const char* input, const char * separator)
{
    char* str = strdup(input);
    int count = 0;
    int capacity = strlen(input);
    char** result = (char **)malloc(capacity*sizeof(*result));

    char* tok=strtok(str,separator); 

    while(1)
    {
        if (count >= capacity)
            result = (char **)realloc(result, (capacity*=2)*sizeof(*result));

        result[count++] = tok? strdup(tok) : tok;

        if (!tok) break;

        tok=strtok(NULL,separator);
    } 

    free(str);
    return result;
}

char *strdup (const char *s)
{
    char *p = (char *)malloc (strlen (s) + 1);   // allocate memory
    if (p != NULL)
        strcpy (p,s);                    // copy string
    return p;                            // return the memory
}

static void setSamplingFreq(char * str){
    
    samplingFreq = atoi(str);   

}
/*Checks the the RTC setting on the configuration file */
static void setRTC(char * str){
    
    rtc = atoi(str);
    
    // check again if the rtc is set
    if(!RTCisSet())
        rtc = 1;
        
    if(rtc){
        remote = false;
        upload = false;
    }
 
}

static void setUploadTimeout(char * str){
    
    upload_timeout = atoi(str);
}
static void setRemoteTimeout(char * str){
    download_timeout = atoi(str);
}

static void setUploadRate(char * str){
    upload_rate = atoi(str);
    // set the upload timeout in case it's not provided
    // If provided, upload_timeout will be overwritten
    upload_timeout = upload_rate;
}
static void setRemoteRate(char * str){
    download_rate = atoi(str);
    
    // set the download timeout in case it's not provided
    download_timeout = download_rate;
}

static void setRemote(char * str){
    remote = atoi(str);
    
    if( remote){
        rtc = false;
        upload = false;
    }
}

static void setModemAttempts(char * str){
    
    modem_attempts = atoi(str);   
}
static void setSampleRate(char * str){
    sampleRate = atoi(str);
    
}
static void setUpload(char * str){
    
    upload = atoi(str);
    if(upload){
        rtc = false;
        remote = false;
    }
}

static void setLineCount(char * str){
    lines = atoi(str);   
}



/*empties a file */
static void clear_file(){
    
    FILE * fp = fopen(CONFIG_FILE_NAME, "w");
    
    if(fp != NULL);
     fclose(fp);
      
}

void reset_system(){
    

  // empty file
  clear_file();
 

  // add all settings
  saveSettings();
  
  //SYSLOG("delete objects");
  delete uploadTimer;
  delete remoteSettingsTimer;
  delete lcdTimer;
 
  // reset   
  mbed_reset();
}

/* add a new setting to the configuration file */
static void addSetting(char * setting, char * value){
    
    // write setting to file
    FILE *fp = fopen(CONFIG_FILE_NAME ,"a");

    if (fp != NULL){
        fprintf(fp, "%s = %s\n", setting, value);  
        fclose(fp);   
    }else{
        SYSLOG("Couldn't open file"); 
    }
    
    return;   
}

static void saveSettings(){
    
    char buf[15];
    sprintf(buf, "%d", upload);
    addSetting("upload" , buf);
    
    sprintf(buf, "%d", remote);
    addSetting("remote" , buf);
    
    sprintf(buf, "%d", upload_rate);
    addSetting("uploadRate", buf);
    
    sprintf(buf, "%d", download_rate);
    addSetting("downloadRate", buf);
    
    sprintf(buf, "%d", upload_timeout);
    addSetting("uploadTimeout", buf);
    
    sprintf(buf, "%d", download_timeout);
    addSetting("downloadTimeout", buf);
    
    sprintf(buf, "%d", sampleRate);
    addSetting("sampleRate", buf);
    
    sprintf(buf, "%d", modem_attempts);
    addSetting("modemAttempts", buf);
    
    // check if rtc is set
    rtc = !RTCisSet();
    
    sprintf(buf, "%d", rtc);
    addSetting("rtc", buf);
    
    sprintf(buf, "%d", lines);
    addSetting("lines", buf);
    
    sprintf(buf, "%02d:%02d", dayStart.tm_hour, dayStart.tm_min);
    addSetting("dayStart", buf);
    
    sprintf(buf, "%02d:%02d", dayEnd.tm_hour, dayEnd.tm_min);
    addSetting("dayEnd", buf);
    
    sprintf(buf, "%d", samplingFreq);
    addSetting("samplingFreq", buf);
    
}

void setTimers(int * uploadDelay, int * remoteDelay){
    
   
    // the timers should run only when the modem isn't being used.
    if(!upload && !remote && !rtc){
        
        // set the timers for upload and download.
        // Note: maximese the delays to 1minute, if the timeouts
        // are longer the timers will be automatically reset by 
        // the intterupts
          *uploadDelay = upload_timeout;
          *remoteDelay = download_timeout;
       
        *uploadDelay = (*uploadDelay < 60) ? *uploadDelay: 60;
        uploadTimer->start((*uploadDelay)*1000);
        
        *remoteDelay = (*remoteDelay < 60) ? *remoteDelay: 60;
        remoteSettingsTimer->start((*remoteDelay)*1000);
           
    } 
}
static bool RTCisSet(){
 
    // get the current time
    time_t seconds = time(NULL);
    
    // time info object encapsulating 
    // all the components of the time
    struct tm * timeInfo = localtime(&seconds);
    
    // Get the number of years since 1970
    // the rtc's default year is 1970 when it
    // hasn't been set.
    int year = timeInfo->tm_year;
    return (year > 70);
         
}


/*Initialzes the hour and minute parameters of the dayStart
and dayEnd time objects*/
static void setTimeBoundaries(struct tm * tlimit, const char * t){
                
        // now only overwrite the time of these bjects
         
        // split the hour and minute fields of the time
        char ** tokens = tokenize(t, ":");
        char * h  = *(tokens);
        char * m = *(tokens+1);


        // set the parameters of the structs;
        tlimit->tm_hour = atoi(h);
        tlimit->tm_min = atoi(m);
            
        char** it;
        for(it=tokens; it && *it; ++it)
        {
                free(*it);
        }
                        
        free(tokens);   
}

/** Check if the system should be resting. We need to 
 check if the current time falls in the resting period
 return the amount of time (in seconds) that the system
 should be resting for
*/ 
int restPeriod(){
    
    SYSLOG("check if we're in the period of rest");
    
    // get the current time
    time_t seconds = time(NULL);   
    
    SYSLOG("Time as a basic string = %s", ctime(&seconds));
    
    // determine how much time is left before start
    time_t tstart = mktime(&dayStart);
    time_t tend = mktime(&dayEnd);
    
    if(tend <  tstart)
        tend += 24*3600;
    
    if(seconds >= tstart && seconds <= tend) {
        SYSLOG("time is in the accepted range. Can start the daily activites\n");
        return 0;  
        
    } 
        
        
    else {
        
        int diff;
        // The range has passed
        if( seconds > tstart && seconds > tend){
            // update the time boundary for the next day
            tstart += 24*3600;
        } 
        // compare the times    
        diff = tstart  - seconds;

        SYSLOG("time  isn't in the range. System has to rest %d seconds more\n", diff);
        return diff;   
    }
   
}

// overwrite the dayStart variable 
// with the value given in the file
void setDayStart(const char * str){
    
    setTimeBoundaries(&dayStart, str);
    SYSLOG("dayStart is now %02d:%02d\n", dayStart.tm_hour, dayStart.tm_min);
}

// overwrite the dayStart variable 
// with the value given in the file
void setDayEnd(const char * str){
  
    setTimeBoundaries(&dayEnd, str);
    SYSLOG("dayEnd is now %02d:%02d\n", dayEnd.tm_hour, dayEnd.tm_min);
 
}

/*Power down usb interface */
int semihost_powerdown() {
    uint32_t arg;
    return __semihost(USR_POWERDOWN, &arg);
}

void powerDownInterfaces(){
    
    // Power down Ethernet interface - saves around 175mW
    // Also need to unplug network cable - just a cable sucks power
    PHY_PowerDown();
    
 
    // Power down magic USB interface chip - saves around 150mW
    // Needs new firmware (URL below) and USB cable not connected
    // http://mbed.org/users/simon/notebook/interface-powerdown/
    // Supply power to mbed using Vin pin
    semihost_powerdown();
    
    // http://developer.mbed.org/forum/bugs-suggestions/topic/434/
    // http://developer.mbed.org/handbook/Beta
    int result = LPC_SC->RSID;
    if ((result & 0x03 )!=0){
        LPC_SC->RSID = 0x0F;
        
        // Must supply power to mbed using Vin pin for powerdown
        // Also exits debug mode - must not be in debug mode
        // for deep power down modes
        mbed_interface_powerdown();
        // Now can do a reset to free mbed of debug mode
        // NXP manual says must exit debug mode and reset for DeepSleep or lower power levels to wakeup
        wait(1.0);
        NVIC_SystemReset();
    
    } else{
        SYSLOG("Debug Mode: OFF");
    }   

    //Peripheral_PowerDown(0xFFFF7FFF);
    //Peripheral_PowerDown(0x7D9B3879);
    
}