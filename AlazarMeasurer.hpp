
#ifndef __ALAZARMEASURER_H__
#define __ALAZARMEASURER_H__

#include <stdio.h>
#include <chrono>

#include "measurer.hpp"
#include <iostream>

#include <conio.h>
#include "visa.h"
#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"

void check_instrument_errors(ViSession vi);
void error_handler(ViSession vi, ViStatus err, char* error);
int do_command(ViSession vi, char* command, char* error); /* Send command. */

#ifdef ALAZAR_MEASURER_EXPORTS
#define ALAZAR_MEASURER_API __declspec(dllexport)
#else
#define ALAZAR_MEASURER_API __declspec(dllimport)
#endif

extern "C" int ALAZAR_MEASURER_API alazarCapture( int preTriggerSamples,
    int postTriggerSamples,
    int recordsPerBuffer,
    int buffersPerAcquisition,
    double triggerLevel_volts,
    double triggerRange_volts,
    int waveformHeadCut,
    int period,
    int timeout_ms, //time to wait for acquiring all records in buffer
    int decimation_value, // divides the clock 1 GHz/ decimation_value. It is 1,2 or 4, or multiples of 10
    bool save,
    char* errorMessage,
    bool TTL,
    double* returnI, double* returnQ);

#endif // __ALAZARMEASURER_H__