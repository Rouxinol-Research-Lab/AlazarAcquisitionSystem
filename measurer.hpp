
#ifndef __MEASURER_H__
#define __MEASURER_H__

#include "visa.h"
#include <string.h> /* For strcpy(), strcat(). */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>



/*  To use this exported function of dll, include this header
 *  in your project.
 */

int do_query_ieeeblock_words(ViSession vi, ViPBuf ieeeblock_data, int data_length, char* query, int* returnedDataLength, char* errorMessage); /* Query for word block. */
void check_instrument_errors(ViSession vi);
void error_handler(ViSession vi, ViStatus err, char* error);
int do_command(ViSession vi, char* command, char* error); /* Send command. */
int do_query_number(ViSession vi, char* query, double* result, char* error);

#ifdef MEASURER_EXPORTS
#define MEASURER_API __declspec(dllexport)
#else
#define MEASURER_API __declspec(dllimport)
#endif

extern "C" int MEASURER_API open_resources(int ieeeblock_size,
                    int cos_raw_size,
                    int edges_size,
                    int trigger_normalizer_treshold,
                    int trigger_max,
                    int trigger_treshold,
                    int trigger_pulse_min,
                    double period,
                    int waveformHeadCut,
                    int waveformTailCut,
                    char* address,
                    char* errorMessage);

extern "C" int MEASURER_API doMeasure(double* returnI, double* returnQ, double* returnV, int* averagesDone, char* errorMessage);

extern "C" int MEASURER_API close_resources();

/*
extern "C" int MEASURER_API measure(double* returnI,
    double* returnQ,
    double* returnV,
    char* osc_address,
    int block_size,
    int number_of_averages,
    int* averagesDone,
    char trigger_treshold,
    int pulse_treshold,
    char trigger_normalizer_treshold,
    char* errorMessage);

    */
#endif // __MEASURER_H__
