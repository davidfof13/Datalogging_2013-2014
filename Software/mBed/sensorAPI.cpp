#include "sensorAPI.h"
#include "systemConfiguration.h"
#include <stdarg.h>
#include <time.h>
#include "IAP.h"
#include "flashInterface.h"

void collectSensorData(void const * args)
{   
    SLOG("inside sensor thread");

    // define all three sensor boads
    struct sensorBoard boards[3] = {  
        {"DCVoltage",ADC2, "DC"},
        {"DCCurrent" ,ADC3, "DC"},
        {"AC",ADC1 ,"AC"}
    };
    
    int i;
    float dcVoltage[4];
    float dcCurrent[4];
    float ac[4];
    char json_string[JSON_BLOCK_SIZE];
    unsigned int bytes_offset = lines*JSON_BLOCK_SIZE;
    FILE * fp;
    
    SLOG("INITIALIZING FLASH SECTOR\n");
    
    if( lines <= 0){
        // empty flash sector
        initializeFlashArea();
    }
    
    SLOG("Collecting sensor data...\n");
     
    while(1){ // record each line of sensor values to a file
        
        // set the watchdog
        wtd.kick(sampleRate + 10);
        
        // protect sdFilesystem's resources from other
        // threads while using it
        sd_mutex.lock();
        SDEnable = 1;
        wait(0.001);
        
        SLOG("instance %d\n", lines);
        
        // get current time (in UTC) before we start taking values
        time_t t = time(NULL);
        SLOG("time: %s", ctime(&t), t);
        SLOG("\rseconds = %d\n",t);

        // record data from one board at a time
        for(i = 0; i < 3; i++){
            
            // destination path of data to store
            char path[50];
            
            SLOG("Board: %s", boards[i].boardName);
            
            // get date in ddmmyy format
            char date[15];
            strftime(date, 15, "%d-%m-%y", localtime(&t));
            
            // create path of file
            sprintf(path, "/sd/%s/%s.csv", boards[i].boardName, date);
            
            SLOG("path: %s", path);
            
            // try to open file
            if(lines > 0)
                fp = fopen(path, "a");
            
            // we're writing to the file for the first time
            else{
                fp = fopen(path, "w");
            }
         
            if (fp == NULL){
                SLOG("error opening file\n");
                   
            }
            
            else{ // if the file is correctly opened...
                int j;
                float r;

                // if no lines have been written to the files yet
                // add column names for each reading
                if(lines == 0){
                    if(!strcmp(boards[i].boardType, "DCVoltage")){
                        fprintf(fp,"left solar panel, right battery, right solar panel, left battery");  
                    } else if(!strcmp(boards[i].boardType, "DCCurrent")){
                        fprintf(fp,"right battery, right solar panel, left solar panel, left battery");
                    } else{
                        fprintf(fp,"right inverter(A), left inverter(A) , left inverter (V), right inverter (V)", r);
                    }
                    fprintf(fp, ", time\n");
                }
                    
                // write sensor values to file
                for(j = 0; j <= 3; j++){
                    
                    SLOG("\n\rConverting Vin%d. ", j+1);
                    r = getSensorValue(boards[i].adcAddr, j, boards[i].boardType);
                    
                    if( r < 0)
                        SLOG("Sensor didn't respond\n");
                        
                    else
                        SLOG("Sensor responded");
                        
                        
                    // write to SD card
                    fprintf(fp,"%.2f", r);
                    fprintf(fp,",");
                    
                    // copy in these buffers as well
                    if(!strcmp(boards[i].boardName, "DCVoltage")){
                        dcVoltage[j] = r;   
                    }
                
                    else if(!strcmp(boards[i].boardName, "DCCurrent")){
                        dcCurrent[j] = r;   
                    }
                
                    else if(!strcmp(boards[i].boardName, "AC")){
                        ac[j] = r;   
                    }    
                }    
    
                // add time as well
                char fmtime[5]; // write formatted time
                strftime (fmtime,4,"%R",localtime(&t));
                fprintf(fp,"%s" , fmtime);
                
                fprintf(fp, "\n");
                
                //close file
                fclose(fp);
                        
                
                SLOG("--------------------------------\n");
            }

            
        } // end for
        
        SLOG("i2c finish");
        
        // disable sd card
        SDEnable = 0;
        
        // release SD file system for other threads to use
        sd_mutex.unlock();
                
        // now create json string and write data to the flash
        setJson(json_string, dcVoltage, dcCurrent, ac, t);
        
        CLOG("Finished constructing json string:");
        CLOG("%s\n", json_string);

        // now write to the flash
        writeToFlash(json_string, bytes_offset);
        bytes_offset += JSON_BLOCK_SIZE;
        
        // count each instance of sensor values recorded
        lines++;
        
        // wait before reading the next samples
        wait((float)sampleRate);
        
        // reset timer
        wtd.kick();        
      
    } // end while(1)    
    
}

float getSensorValue(const int adc, int input, char * signalType){
 
    float result;
    
    if(!strcmp(signalType, "DC")){
        
        // perform a single conversion for a DC signal
        result = ADCread(adc, input);
    }

    // take multiple samples for an AC signal
    else if(!strcmp(signalType, "AC")){
        result = acVpp(input);
    }
    

    return result;
    
}

float ADCread(const int addr, int input, float ts){
    
    I2C i2c(p28,p27); // define I2C bus
    char data[2]; //buffer used to send and read data from ADC
    int i = 1; // sensor No
    int r; // result
    
    i2c.start();
    
    // First check if we can communicate with ADC
    r = i2c.write(addr); 
   

    // select input pin that we want to convert
    switch(input) {
            
        case 0 : data[0] = 0x10;
        break;
                
        case 1: data[0] = 0x20;
        break; 
                
        case 2: data[0] = 0x40;
        break;
                
        case 3: data[0] = 0x80;
        break;
        
        default:return 0.0;
        //break;
    }
    
    // Send the command to convert on correct pin and set the address pointer to the conversion result register
    r = i2c.write(addr, data, 1);
                  
    if(r) { // ADC doesn't respond to address
        return -1.0;   
    }
                
    //read ADC output. The converted value is stored in the result register
    // and will be returned as a 16 bit number
    r = i2c.read(addr , data, 2); 
    if (!r)  {
       
        int x = data[0] & 0xF; //top byte of data - and with 0xF as only bottom 4 bits are useful
        int y = data[1]; //bottom byte of data 
                    
        int z = (x<<8)|y; //shift top byte of data to the top part of the int and or it with bottom part so they are concatenated
        z = z>>2;  
        
        //wait(0.001);
        i2c.stop(); //stop process
        
        wait(ts); // wait sampling period
        return toAnalogue(z);
    }
    
    else {
        i2c.stop(); //stop process
        return -1.0;
    }   
}

void createSDdirectories(){
    
    SDEnable = 1;
    wait(0.001);
    mkdir("/sd/DCCurrent",0777);
    mkdir("/sd/DCVoltage",0777);
    mkdir("/sd/AC", 0777);
    SDEnable = 0;
  
}



/*@param comma indicates if the element to be added is the 
last element of the string. If it's the case then no 
comma would be added */
void addJsonField(char * json_string, int fieldType, const char * fmt, ...){
    
    char tmp[100];

    // get the list of arguments and write the formatted
    // string to the new buffer
    va_list args;
    va_start(args, fmt);
    vsnprintf(tmp, 100, fmt, args);
    va_end(args);

    switch(fieldType){
        case 0: //strcpy(json_string, tmp);
        strcat(json_string, tmp);
        strcat(json_string,",");
        break;
        
        case 1: strcat(json_string, tmp);
        strcat(json_string,",");
        break;
        
        case 2: strcat(json_string, tmp);
        break;
        
    }
}

/*Convert decimal reading to its analogue
equivalent */
float toAnalogue(int x){
    
        float r = (5.0/1024.0) * ((float)x);
        return r;
           
}

/* Calculate and return peak to peak voltage for an ac signal.
 Note: This will be also used for the DC signals of the AC board*/
float acVpp(int input){
    
    int i;
    float Vmin, Vmax, Ts, Fs, T;
    
    // set the sampling rate
    Fs = samplingFreq;
    
    int N; // number of samples to read
    
    T = AC_WAVEFORM_PERIOD;
    
    // set the sampling period
    Ts = 1.0/Fs;
    
    // determine the number of samples to read
    N = T/Ts;    

    SLOG("sensor %d", input);
    SLOG("sample every %.4f", Ts);
    
    // initialize extremum values with the first
    // sample read
    Vmax = ADCread(ADC1, input);
    Vmin = Vmax;
    
    //SLOG("starting to sample..");
    
    // read the remaining N-1 samples of the input with a time spacing
    // of Ts seconds between each
    for(i=0; i < N-1; i++){
  
        float tmp = ADCread(ADC1, input);
        if (tmp < Vmin) 
            Vmin = tmp;
           
        else if (tmp > Vmax)
            Vmax = tmp;
            
        wait(Ts);   
    }
    
    SLOG("Vmax = %.4f", Vmax);
    SLOG("Vmin = %.4f", Vmin);
    SLOG("Vpp = %.4f\n\n\r", (Vmax - Vmin));
    
    return (Vmax - Vmin);
    
}

void setJson(char json_string[], float dcVoltage[], float dcCurrent[], float ac[], int t){
    
    
    // clear buffer by setting all its bytes to 0s 
    memset(json_string, 0 ,JSON_BLOCK_SIZE);

    // create json instance
    strcpy(json_string, "{");

    addJsonField(json_string, 0, "\"ls\": {\"c\": %.3f, \"v\": %.3f}", dcCurrent[2], dcVoltage[0]);
    addJsonField(json_string, 1, "\"rs\": {\"c\": %.3f, \"v\": %.3f}", dcCurrent[1], dcVoltage[2]);
    addJsonField(json_string, 1, "\"lb\": {\"c\": %.3f, \"v\": %.3f}", dcCurrent[3], dcVoltage[3]);
    addJsonField(json_string, 1, "\"rb\": {\"c\": %.3f, \"v\": %.3f}", dcCurrent[0], dcVoltage[1]);
    addJsonField(json_string, 1, "\"li\": {\"c\": %.3f, \"v\": %.3f}", ac[1], ac[2]);
    addJsonField(json_string, 1, "\"ri\": {\"c\": %.3f, \"v\": %.3f}", ac[0], ac[3]);
    addJsonField(json_string, 2, "\"time\": %d", t);
    strcat(json_string, "}\n"); // add new line as a marker for the end of the json
      
}


