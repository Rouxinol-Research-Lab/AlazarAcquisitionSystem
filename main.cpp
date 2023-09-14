#include <stdio.h>
#include <chrono>

#include "measurer.hpp"
#include "AlazarMeasurer.hpp"
#include <iostream>




int main() {
    
    //Select the number of pre-trigger samples
    U32 preTriggerSamples = 0;

    //Select the number of post-trigger samples per record
    U32 postTriggerSamples = 3840;

    //Specify the number of records per DMA buffer
    U32 recordsPerBuffer = 200;

    //Specify the total number of buffers to capture
    U32 buffersPerAcquisition = 25;

    // Calculate the trigger level code from the level and range
    double triggerLevel_volts = 0.5; // trigger level
    double triggerRange_volts = 3.; // input range

    int waveformHeadCut = 100;
    int period = 14;
    double returnI = 0, returnQ = 0;
    int retCode = 99 ;

    int timeout_ms = 5000; //time to wait for acquiring all records in buffer
    int decimation_value = 1; // divides the clock 1 GHz/ decimation_value. It is 1,2 or 4, or multiples of 10

    char errorMessage[81];
    
    retCode = alazarCapture(preTriggerSamples,
        postTriggerSamples,
        recordsPerBuffer,
        buffersPerAcquisition,
        triggerLevel_volts,
        triggerRange_volts,
        waveformHeadCut,
        period,
        timeout_ms, //time to wait for acquiring all records in buffer
        decimation_value, // divides the clock 1 GHz/ decimation_value. It is 1,2 or 4, or multiples of 10
        true,
        errorMessage,
        &returnI, &returnQ);

    printf("I = %f \n Q = %f\n", returnI, returnQ);


    printf(errorMessage);


    return 0;
}


/*
int main () {

    double I, Q, V;

    int averagesDone = 0;

    char* oscAddress = "TCPIP0::169.254.101.102::inst0::INSTR";
    int block_size = 1000050;

    char errorMessage[1024] = { 0 };

    double meanV = 0;

#define TRIGGER_RISING_THRESHOLD  30
#define TRIGGER_NORMALIZER_TRESHOLD  30
#define TRIGGER_NORMALIZER_MAX  100
#define MIN_PULSE_SIZE_THRESHOLD 3000
    
    int err = open_resources(block_size,//int ieeeblock_size,
        5100,//int cos_raw_size,
        1100,//int edges_size,
        TRIGGER_NORMALIZER_TRESHOLD,//,int trigger_normalizer_treshold,
        TRIGGER_NORMALIZER_MAX,//int trigger_max,
        TRIGGER_RISING_THRESHOLD,//int trigger_treshold,
        MIN_PULSE_SIZE_THRESHOLD,//int trigger_pulse_min,
        10,//double period,
        100,//int waveformHeadCut,
        0, //int waveformTailCut,
        oscAddress,//char* address,
        errorMessage);// char* errorMessage);

    if (err < 0) {
        close_resources();
        printf("error: %d message: %s", err, errorMessage);
        exit(1);
    }
    
    
    // Start measuring time
    auto begin = std::chrono::high_resolution_clock::now();

    doMeasure(&I, &Q, &V, &averagesDone, errorMessage);
    printf("Averages done: %d", averagesDone);

    meanV = sqrt(pow(I, 2) + pow(Q, 2));

    printf("\n\n meanI: %0.15lf\n", I);
    printf("meanQ: %0.15lf\n", Q);
    printf("meanV: %0.15lf\n", meanV);

    
    // Stop measuring time and calculate the elapsed time
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
    printf("Time measured: %.3f seconds.\n", elapsed.count() * 1e-9);
    
    
    close_resources();


    return 0;
}
*/
