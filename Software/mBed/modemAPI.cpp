#include "modemAPI.h"
#include "debug.h"
#include "systemConfiguration.h"
#include "sensorAPI.h"
#include "flashInterface.h"
#include "MbedJSONValue.h"


/* function that uploads or downloads data from/to the server at
   regular time intervals. The modem will be disabled after 
   any request in order to save power
*/ 
void dataTransfer(void const *){

    // Define modem object and the pin that enable/disables it
    VodafoneUSBModem modem(p30, 1);

    // Get next modem request from queue
    osEvent evt = modem_queue.get(QUEUE_TIMEOUT);

    // Check if the message was well received
    if (evt.status == osEventMessage){

        // determine what action must be taken
        ModemRequest * request = (ModemRequest *)evt.value.p;
         
        // number of attempts for trying to connect
        int att = MAX_CONNECT; 
        
        // indicates when we should stop trying to connect
        int stop = 0;

        // Try to establish a modem connection MAX_CONNECT times
        do{

            CLOG("\n\renabling modem...(%d)", MAX_CONNECT - att);

            // power the modem
            modem.power(1);
            wait(4.0);

            CLOG("\n\rTrying to connect...");

            wtd.kick(MODEM_CONNECT_TIMEOUT);
                        
            // connect to the mobile network
            int ret = modem.connect(APN);
            wtd.kick();

            // Check if connection was successful
            if(!ret){ 
                switch(*request){
                    case UPLOAD: CLOG("uploading\n");
                                 sendData();
                                 sendLog(&modem);
                                 upload = 0;
                                 lines = 0;
                    break;
        
                    case REMOTE_SETTINGS: CLOG("downloading\n");
                                getRemoteSettings();
                                remote = 0;
                    break;
        
                    case NTP_TIME: getNTPtime();
                                    rtc = 0;
                    break;
        
                    default: CLOG("\n\rNo modem request");
                    break;
                }
    
                stop = 1;
                modem.disconnect();
            }else{
                CLOG("\n\rCould not connect");
                att--;
            }

            // Turn modem off
            modem.power(0);
            modem_attempts = 0; // reset number of attempts
    
        } while(att > 0 && !stop); 

    } // end if
}

static void getRemoteSettings(){
    
    HTTPClient http;
    char url[100];
    char result[150];
    sprintf(url,"http://eqdatalogger2.appspot.com/Settings?dev=mBed&kiosk=%s", KIOSK);
    int ret;
    MbedJSONValue v; // container of json object

    //GET data
    CLOG("\n\rTrying to fetch page...\n");
            
    ret = http.get(url, result, 128);
    if (!ret){
            
        CLOG("\n\rPage fetched successfully - read %d characters\n", strlen(result));
        CLOG("\n\rResult: %s\n", result);

        const char * json = result;
           
        CLOG("\n\rNow going to parse json %s\n", json);
        
        // parse json
        parse(v, json);        
        CLOG("Finished parsing json.");         
        CLOG("Displaying json fields");
            
        CLOG("\n\rday start = %s", v["dayStart"].get<std::string>());
        CLOG("\n\rday end = %s", v["dayEnd"].get<std::string>());
        CLOG("\n\rupload rate = %d", v["uploadRate"].get<int>());
        CLOG("\n\rsampling Freq = %d", v["samplingFreq"].get<int>());
        CLOG("\n\rsample Rate = %d", v["sampleRate"].get<int>());
    
        // save json fields
        setDayStart((v["dayStart"].get<std::string>()).c_str());
        setDayEnd((v["dayEnd"].get<std::string>()).c_str());
        upload_rate = v["uploadRate"].get<int>();
        sampleRate = v["sampleRate"].get<int>();
        samplingFreq = v["samplingFreq"].get<int>();
             
    }

    CLOG("\n\rHTTP return code = %d\n",  http.getHTTPResponseCode());
        
}

/** Synchronize RTC with remote server */
static void getNTPtime(){
    NTPClient ntp;
    
    CLOG("\n\rTrying to update time...");
    
    // Tanzania ntp server
    if (ntp.setTime("tz.pool.ntp.org") == 0)
    {
      CLOG("\n\rSet time successfully");
      time_t ctTime;
      ctTime = time(NULL);
      CLOG("\n\rTime is set to (UTC): %s", ctime(&ctTime));
    }
    else
    {
      printf("\n\rError");
    }     
}


static void sendData(){
    
    int i = 0;
    int offset = 0;
    
    HTTPClient client; // http client
    char url[] = "http://eqdatalogger2.appspot.com/postSensorValues";    
    char str[70]; // server response
    HTTPText reply(str, 70); // container of server response;
    char data[256]; // data to upload
    
    CLOG("No of lines to upload: %d\n", lines);
    // read one instance of sensor values at a time from
    // the flash and upload it to the server
    while(i < lines){
        
        // HTTP payload
        HTTPMap message; 

        CLOG("instance %d", i);
        
        // clear the buffer first
        memset(data, 0, JSON_BLOCK_SIZE);
        
        // read from the flash
        readDataFromFlash(data , offset);
        
        CLOG("%s\r\n",data );
        
        message.put("dev", "mBed"); // specify device
        message.put("kiosk", KIOSK); // specify kiosk
        message.put("data", data); // add data
    
        // post data
        CLOG("Posting...\n");
        int ret = client.post(url, message, &reply);
            
        if (!ret)
            CLOG("Executed POST successfully - read %d characters\n", strlen(str));   
        else
            CLOG("Error : ret = %d\n", ret);
        
        CLOG("Result: %s", str);
        CLOG("HTTP return code = %d\n", client.getHTTPResponseCode());
        CLOG("--------------------------------------\n");
        
        offset += JSON_BLOCK_SIZE;
        i++;
        // reset watchdog to prevent activity from stopping
        wtd.kick();
    
    }
    //lines = 0; // rest number of lines
}


/** Sends extra information to the server */
static void sendLog(VodafoneUSBModem * modem){
 
    HTTPClient client; // http client
    HTTPMap message;
    char url[] = "http://eqdatalogger2.appspot.com/postClientLog";    
    char str[20]; // server response
    HTTPText reply(str, 20); // container of server response;
    char log[200]; // data to upload
    
    CLOG("LOG");
    char msgBuffer[80];
    
    // get the SIM card credit balance
    int ret = sendUSSDCommand(modem, USSD_COMMAND_BYTES, msgBuffer, 78);
    
     // Should only display message if one has been received.
    CLOG("Result of command: %s\n", msgBuffer);
    
    if(!ret){
       sprintf(log, "%s\n\n," ,msgBuffer);
    }
    
    int  Rssi; // signal strength
    LinkMonitor::REGISTRATION_STATE RegistrationState; // registration state
    LinkMonitor::BEARER Bearer; // connection type
    
    modem->getLinkState(&Rssi, &RegistrationState, &Bearer);
    CLOG("Link state Rssi: %d Registration state %x Bearer %x\n ", Rssi, RegistrationState, Bearer);
    
    strcat(log, "connection type: ");
    switch(Bearer) {
    
        case 0 : strcat(log, "Unknown\n");
        break;
        
        case 1: strcat(log, "GSM (2G)\n");
        break;
        
        case 2: strcat(log, "EDGE (2.5G)\n");
        break;
        
        case 3 : strcat(log,"UMTS (3G)\n");
        break;
        
        case 4 : strcat(log,"HSPA (3G+)\n");
        break;
    
    }
    
    strcat(log, ",\n ");
    
    strcat(log, "registration state :");
    switch(RegistrationState) {
        case 0: strcat(log, "Unknown\n");
        break;
        
        case 1: strcat(log, "Registering\n");
        break;
        
        case 2: strcat(log, "Denied\n");
        break;
        
        case 3: strcat(log,"No signal\n");
        break;
        
        case 4: strcat(log, "Registered on Home Network\n");
        break;
        
        case 5: strcat(log,"Registered on Roaming Network unknown\n");
        break;

    }
    strcat(log, ", \n");
    
    strcat(log, "Rssi: ");
    if(!Rssi)
        strcat(log,"Unknown\n");
        
    else{
        char tmp[5];
        sprintf(tmp ,"%ddbm\n", Rssi); 
        strcat(log, tmp);
    }
    CLOG("log : %s", log);
    
    message.put("dev", "mBed"); // specify device
    message.put("kiosk", KIOSK); // specify kiosk
    message.put("content", log); // add data
    
    // post data
    CLOG("Posting...\n");
    ret = client.post(url, message, &reply);
            
    if (!ret)
        CLOG("Executed POST successfully - read %d characters\n", strlen(str));   
    else
        CLOG("Error : ret = %d\n", ret);
        
    CLOG("Result: %s", str);
    
}

/*Send USSD command by SMS */
static int sendUSSDCommand(VodafoneUSBModem * modem, char *ussdCommand, char * msgBuffer, int maxLen)
{
   
    CLOG("Sending %s on USSD channel\n", ussdCommand);

    int ret = modem->sendUSSD(ussdCommand, msgBuffer, maxLen);

    CLOG("Send USSD command returned %d\n", ret);

    return ret;
    
}

void setDataUpload(void const * arg){
    
    // determine the remaining time
    static int timeRemaining = upload_timeout;
    
     // get the delay
    int * delay = (int *)arg;
    
     // compute the remaining time
    timeRemaining -= *delay;
    
    if(timeRemaining > 0){
        
        // restart the timer with the new delay 
        int newDelay = (timeRemaining < 60) ? timeRemaining: 60;
        uploadTimer->start(1000*newDelay);
        return;
    }
    
    CLOG("Upload timeout!\n");
    if(SDFileSystemCreated){
         
        // lock sd card resources to prevent other threads from
        // using it
         sd_mutex.lock();
         upload = 1;  
         //download_timeout -= *delay; // keep track of the other timer aswell
         download_timeout -= upload_timeout; // keep track of the other timer aswell
         upload_timeout = upload_rate; // reset timeout value
         reset_system();
            
    }
}


void setDataDownload(void const * arg){
    
    // determine the remaining time
    static int remainingTime = download_timeout;
    
     // get the delay
    int * delay = (int *)arg;
    
     // compute the remaining time
    remainingTime -= *delay;
    
    if(remainingTime > 0){
        
        // restart the timer with the new delay 
        // maximise the delay to 1 min
        int newDelay = (remainingTime < 60) ? remainingTime: 60;
        remoteSettingsTimer->start(1000*newDelay);
        
        return;
    }
    
    CLOG("Download timeout!\n");

    // lock sd card resources to prevent other threads from
    // using it
         sd_mutex.lock();
         remote = 1;  
         //upload_timeout -= *delay; // keep track of the other timer as well
         upload_timeout -= download_timeout;
         download_timeout = download_rate; // reset timeout value
         reset_system();
            
 
}