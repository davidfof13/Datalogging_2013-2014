#include "mbed.h"

#define SAMPLE_RATE 0.001  // in seconds
#define READINGS // number of reads
#define MAX_SAMPLES 50 //

LocalFileSystem local("local");
Serial pc(USBTX, USBRX);


// configure unused ADC pins as DigitalOut
// to reduce noisy spikes
DigitalOut led(LED1);
DigitalOut led2(LED2);
DigitalOut ai15(p15);
DigitalOut ai16(p16);
DigitalOut ai17(p17);
DigitalOut ai18(p18);
DigitalOut ai19(p19);


AnalogIn Vin(p20); // voltage input in percentage
float samples[100]; // readings

float Vmax;
float Vmin;
void check_wave(); // initial wave check
float ADC_read();

int main()
{
     int ADC_count = 0; // number of ADC samples
     mbed_interface_disconnect(); // turn off debug
     
     check_wave();
     pc.printf("initial values\n\r");
     pc.printf("Vmax = %.4f\n\r", Vmax);
     pc.printf("Vmin = %.4f\n\r", Vmin);
     
     led2 = 1;
    

     while(ADC_count != MAX_SAMPLES ){         
     
        // read RMS voltage
        float tmp = ADC_read();
        samples[ADC_count] = tmp;
        
        
        wait(0.1); 
        ADC_count++;
     }
  
     FILE *fp = fopen("/local/samples.txt", "w");  // Open "samples.txt" on the local file system for writing
     
     int i;
     fprintf (fp,"total number of samples : %d\n\n", MAX_SAMPLES);
     
     for(i=0; i < MAX_SAMPLES; i++){
        fprintf(fp,"%.4f\n" , samples[i]);   
     }
     fclose(fp);
     
     pc.printf("\n\rFinished\n\r");
     return 0;
     
}

// Just one period of the sinewave is read to initialise 
// our variables
void check_wave(){

    int i;
    // choose the number of samples to read
    const int N = 200;

    // we assume a mains input frequency of 50Hz for the AC signal
    // => f = 50Hz <=> T = 20ms
    // choose Ts such that only a single period of the sinewave is covered
    float Ts = (0.02)/N;
    
    // initialize extremum values to first sample
    Vmax = 3.3*Vin;
    Vmin = Vmax;
     

    // read the remaining N-1 samples of the input with a time spacing
    // of Ts seconds between each
    for(i=0; i < N-1; i++){
        
        float tmp = 3.3*Vin;
        if (tmp < Vmin) 
            Vmin = tmp;
           
        else if (tmp > Vmax)
            Vmax = tmp;
            
        wait(Ts);   
    }
}

// reads the sinewave for several periods over which
// the DC offset Vdc and the peak voltage Vmax are calculated
// The RMS voltage is then calculated and returned
// ref: http://masteringelectronicsdesign.com/how-to-derive-the-rms-value-of-a-sine-wave-with-a-dc-offset/
float ADC_read(){
    
    float Vrms;
    float Vavg = 0.0; //  DC offset. Need to reset to 0 as it's a cumulative average
    
    // we assume a mains input frequency of 50Hz for the AC signal
    // f = 50Hz <=> T = 20ms
    float T = 0.02;
    
    // total amount of time we want to read the sinewave.
    float total_sample_period = 3.0*T;
    
    // number of samples that will be read
    int total_samples = ceil(T/SAMPLE_RATE);

    // reset sample counter
    int n = 0;
    
    
    // continously calculate average voltages
    while (n < total_samples){
        
        // read analogue input
        float tmp = Vin*3.3;
        
        if (tmp > Vmax)
            Vmax = tmp;
            
        // cumulative average
        Vavg += tmp/total_samples;
        
        // wait before reading next sample
        wait(SAMPLE_RATE);
        n++;
    }

    pc.printf("Vavg = %.4f and Vmax = %.4f \n\r", Vavg, Vmax);
    // calculate RMS voltage
    Vrms = sqrt(pow(Vavg,2) + pow(Vmax,2)/2.0);
    
    return Vrms;
}