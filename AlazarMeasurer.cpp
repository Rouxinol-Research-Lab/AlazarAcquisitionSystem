
#include <conio.h>

#include "AlazarMeasurer.hpp"


#include "AlazarError.h"
#include "AlazarApi.h"
#include "AlazarCmd.h"


// samples exported function
int GetIQ(double* RawSin, double* RawCos, double* Data, int Headpointer, int Tailpointer, double* I, double* Q)
{

    double sumI = 0, sumQ = 0;
    for (int i = Headpointer; i < Tailpointer; i++)
    {
        sumI += RawSin[i] * Data[i];
        sumQ += RawCos[i] * Data[i]; 
    }
    *I = sumI / (Tailpointer - Headpointer + 1) * 2; // I = As Aref cos(Phase_s - Phase_ref)
    *Q = sumQ / (Tailpointer - Headpointer + 1) * 2; // Q = As Aref sin(Phase_s - Phase_ref)
    return 0;
}

int GetCosFromSin(double* rawSin, double Period, double* rawCos, int ChopHead, int ChopTail, int* Headpointer, int* Tailpointer)
{
    int i;

    int Head = 0;
    int Tail = ChopTail; // pointer for head and tail of array wanted;
    double ShiftDist;

    for (i = ChopHead; i < ChopTail; i++)//find the Head, move Tail to the last one
    {
        if ((rawSin[i] < 0) && (rawSin[i + 1] >= 0))
        {
            Head = i + 1;
            break;
        }
    }

    for (i = ChopTail; i > ChopHead; i--)//move Tail one period back
    {
        if ((rawSin[i] < 0) && (rawSin[i + 1] >= 0))
        {
            Tail = i;
            break;
        }
    };


    ShiftDist = Period / 4;
    for (i = Head; i < Tail; i++)
    {
        int position = int(ShiftDist);
        rawCos[i] = rawSin[i + position] + (rawSin[i + position + 1] - rawSin[i + position]) * (ShiftDist - position);
    }

    *Headpointer = Head;
    *Tailpointer = Tail;
    return 0;
}


/*
//Select the number of pre-trigger samples
U32 preTriggerSamples = 0;

//Select the number of post-trigger samples per record
U32 postTriggerSamples = 5120;

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
*/



int alazarCapture( int preTriggerSamples,
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
    double* returnI, double* returnQ) {
    RETURN_CODE retCode;
    int returnCodeAlazarCapture = 0;

    ViSession RM;
    ViSession viDelayGenerator;
    ViStatus err;

    snprintf(errorMessage, 80, "NO ERROR");

    err = viOpenDefaultRM(&RM);
    if (err != VI_SUCCESS) {
        return -1;
    }


    err = viOpen(RM, "TCPIP0::169.254.101.106::inst0::INSTR", VI_NULL, VI_NULL, &viDelayGenerator);
    if (err != VI_SUCCESS) {
        error_handler(viDelayGenerator, err, errorMessage);
        return -2;
    }

                                                    /*********************/
                                                    /* CONFIGURING BOARD */
                                                    /*********************/

    /***********************************************************************************************************************/
    /* Alazar allows multiples boards in the same computer. Also one may group boards together in different groups.        */
    /* Each group is called a board system and receives a number as an ID. Also each board in that board system receives   */
    /* a board ID. When there is only one board, there is only one system. Therefore, systemId is 1 and boardId is also 1. */
    /***********************************************************************************************************************/
    // Select a board
    U32 systemId = 1;
    U32 boardId = 1;

    /**********************************************************************************************/
    /* Several ATS - SDK functions require a �system handle�.A system handle is the handle of the */
    /*    master board in a board system. If a board is not connected to other boards using a     */
    /* SyncBoard, then its board handle is the system handle.                                     */
    /**********************************************************************************************/

    // Get a handle to the board
    HANDLE boardHandle = AlazarGetBoardBySystemID(systemId, boardId);
    if (boardHandle == NULL)
    {
        snprintf(errorMessage, 80, "Error: Unable to open board system Id %u board Id %u\n", systemId, boardId);
        return -3;
    }


    /********************************************************************************************/
    /* ATS - SDK includes a number of functions that return information about a board specified */
    /*    by its handle.                                                                        */
    /********************************************************************************************/

    BoardTypes kind = AlazarGetBoardKind(boardHandle);
    //U32 memorySize;
    //U8 bitsPerSample;
    //AlazarGetChannelInfo(boardHandle, &memorySize, &bitsPerSample);

    U8 CPLDMajor;
    U8 CPLDMinor;
    AlazarGetCPLDVersion(boardHandle, &CPLDMajor, &CPLDMinor);

    U8 driverMajor, driverMinor, driverRevision;
    AlazarGetDriverVersion(&driverMajor, &driverMinor, &driverRevision);

    U32 returnValue;
    AlazarQueryCapability(boardHandle, GET_SERIAL_NUMBER, 0, &returnValue);

    /******************************************************************************************/
    /* Before acquiring data from a board system, an application must configure the timebase, */
    /* analog inputs, and trigger system settings of each board in the board system.          */
    /******************************************************************************************/

    // Set 250 MSamples/s Sampling rate using a 10MHz external clock.
    // sample rate identifier value must be 1000000000. 
    retCode = AlazarSetCaptureClock(
        boardHandle, // HANDLE - board handle
        EXTERNAL_CLOCK_10MHZ_REF, // U32 - clock source Id
        1000000000, // U32 - sample rate Id or value
        CLOCK_EDGE_RISING, // U32 - clock edge Id
        decimation_value // U32 - decimation value
    );

    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarSetCaptureClock failed -- %s\n", AlazarErrorToText(retCode));
        return -4;
    }

    /**********************************************************************************************/
    /* AlazarTech digitizers have analog amplifier sections that process the signals input to its */
    /* analog input connectors before they are sampled by its ADC converters.The gain, coupling,  */
    /* and termination of the amplifier sections should be configured to match the properties of  */
    /* the input signals.                                                                         */
    /**********************************************************************************************/
    retCode = AlazarInputControl(
        boardHandle, // HANDLE -- board handle
        CHANNEL_A, // U8 -- input channel
        AC_COUPLING, // U32 -- input coupling id
        INPUT_RANGE_PM_100_MV, // U32 -- input range id
        IMPEDANCE_50_OHM // U32 -- input impedance id
    );

    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
        return -5;
    }

    retCode = AlazarSetBWLimit(boardHandle,
        CHANNEL_A,
        0);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode));
        return -6;
    }

    retCode = AlazarInputControl(
        boardHandle, // HANDLE -- board handle
        CHANNEL_B, // U8 -- input channel
        AC_COUPLING, // U32 -- input coupling id
        INPUT_RANGE_PM_100_MV, // U32 -- input range id
        IMPEDANCE_50_OHM // U32 -- input impedance id
    );

    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarInputControl failed -- %s\n", AlazarErrorToText(retCode));
        return -7;
    }

    retCode = AlazarSetBWLimit(boardHandle,
        CHANNEL_B,
        0);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarSetBWLimit failed -- %s\n", AlazarErrorToText(retCode));
        return -8;
    }

    /*************************************************************************************************/
    /*Use the AlazarSetTriggerOperation() API function to configure each of the two trigger engines, */
    /*and to specify how they should be used to make the board trigger.                              */
    /*AlazarTech digitizer boards can trigger on a signal connected to its TRIG IN connector.        */
    /*************************************************************************************************/


    U32 triggerLevel_code =
        (U32)(128 + 127 * triggerLevel_volts / triggerRange_volts);

    // Configure trigger engine J to generate a trigger event
    // on the falling edge of an external trigger signal.
    retCode = AlazarSetTriggerOperation(
        boardHandle, // HANDLE -- board handle
        TRIG_ENGINE_OP_J, // U32 -- trigger operation
        TRIG_ENGINE_J, // U32 -- trigger engine id
        TRIG_EXTERNAL, // U32 -- trigger source id
        TRIGGER_SLOPE_POSITIVE, // U32 -- trigger slope id
        triggerLevel_code, // U32 -- trigger level (0 to 255)
        TRIG_ENGINE_K, // U32 -- trigger engine id
        TRIG_DISABLE, // U32 -- trigger source id for engine K
        TRIGGER_SLOPE_POSITIVE, // U32 -- trigger slope id
        128 // U32 -- trigger level (0 to 255)
    );

    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarSetTriggerOperation failed -- %s\n", AlazarErrorToText(retCode));
        return -9;
    }


    if (TTL == true) {
        // Configure the external trigger input to +/-5V range,
        // with DC coupling
            retCode = AlazarSetExternalTrigger(
                boardHandle, // HANDLE -- board handle
                DC_COUPLING, // U32 -- coupling id
                ETR_TTL // U32 -- external range id
            );
    }
    else {
        // Configure the external trigger input to +/-5V range,
        // with DC coupling
        retCode = AlazarSetExternalTrigger(
            boardHandle, // HANDLE -- board handle
            DC_COUPLING, // U32 -- coupling id
            ETR_5V // U32 -- external range id
        );
    }
    


    // NOTE:
    //
    // The board will wait for a for this amount of time for a trigger event. If a trigger event
    // does not arrive, then the board will automatically trigger. Set the trigger timeout value to
    // 0 to force the board to wait forever for a trigger event.
    //
    // IMPORTANT:
    //
    // The trigger timeout value should be set to zero after appropriate trigger parameters have
    // been determined, otherwise the board may trigger if the timeout interval expires before a
    // hardware trigger event arrives.

    retCode = AlazarSetTriggerTimeOut(boardHandle, 0);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarSetTriggerTimeOut failed -- %s\n", AlazarErrorToText(retCode));
        return -10;
    }

    retCode = AlazarConfigureAuxIO(boardHandle, AUX_OUT_TRIGGER, 0);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarConfigureAuxIO failed -- %s\n", AlazarErrorToText(retCode));
        return -11;
    }

                                                    /***************/
                                                    /* ACQUISITION */
                                                    /***************/



    U32 averagesDone = buffersPerAcquisition * recordsPerBuffer;

    //Select which channels to capture (A, B, or both)
    U32 channelMask = CHANNEL_A | CHANNEL_B;

    int channelCount = 2;

    // Get the sample size in bits, and the on-board memory size in samples per channel
    U8 bitsPerSample;
    U32 maxSamplesPerChannel;
    retCode = AlazarGetChannelInfo(boardHandle, &maxSamplesPerChannel, &bitsPerSample);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarGetChannelInfo failed -- %s\n", AlazarErrorToText(retCode));
        return -12;
    }

    // Calculate the size of each DMA buffer in bytes
    float bytesPerSample = (float)((bitsPerSample + 7) / 8);
    U32 samplesPerRecord = preTriggerSamples + postTriggerSamples;
    U32 bytesPerRecord = (U32)(bytesPerSample * samplesPerRecord +
        0.5); // 0.5 compensates for double to integer conversion 
    U32 bytesPerBuffer = bytesPerRecord * recordsPerBuffer * channelCount;
    
    
    // Create a data file if required
    FILE* fpData = NULL;


    if (save) {


        fpData = fopen("data.bin", "wb");
        if (fpData == NULL)
        {
            snprintf(errorMessage, 80, "Error: Unable to create data file -- %u\n", GetLastError());
            return -13;
        }
    }




    // Allocate memory for DMA buffers
    BOOL success = TRUE;

#define BUFFER_COUNT 4

    // Globals variables
    U8* BufferArray[BUFFER_COUNT] = { NULL };
    double samplesPerSec = 0.0;

    U32 bufferIndex;
    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
        // Allocate page aligned memory
        BufferArray[bufferIndex] =
            (U8*)AlazarAllocBufferU8(boardHandle, bytesPerBuffer);
        if (BufferArray[bufferIndex] == NULL)
        {
            snprintf(errorMessage, 80, "Error: Alloc %u bytes failed\n", bytesPerBuffer);
            success = FALSE;
            returnCodeAlazarCapture = -14;
        }
    }

    // Configure the record size
    if (success)
    {
        retCode = AlazarSetRecordSize(boardHandle, preTriggerSamples, postTriggerSamples);
        if (retCode != ApiSuccess)
        {
            snprintf(errorMessage, 80, "Error: AlazarSetRecordSize failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
            returnCodeAlazarCapture = -15;
        }
    }

    // Configure the board to make an Traditional AutoDMA acquisition
    if (success)
    {
        U32 recordsPerAcquisition = recordsPerBuffer * buffersPerAcquisition;

        U32 admaFlags = ADMA_EXTERNAL_STARTCAPTURE | ADMA_TRADITIONAL_MODE;

        retCode = AlazarBeforeAsyncRead(boardHandle, channelMask, -(long)preTriggerSamples,
            samplesPerRecord, recordsPerBuffer, recordsPerAcquisition,
            admaFlags);
        if (retCode != ApiSuccess)
        {
            snprintf(errorMessage, 80, "Error: AlazarBeforeAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
            returnCodeAlazarCapture = -16;
        }
    }

    // Add the buffers to a list of buffers available to be filled by the board

    for (bufferIndex = 0; (bufferIndex < BUFFER_COUNT) && success; bufferIndex++)
    {
        U8* pBuffer = BufferArray[bufferIndex];
        retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
        if (retCode != ApiSuccess)
        {
            snprintf(errorMessage, 80, "Error: AlazarPostAsyncBuffer %u failed -- %s\n", bufferIndex,
                AlazarErrorToText(retCode));
            success = FALSE;
            returnCodeAlazarCapture = -17;
        }
    }

    // Arm the board system to wait for a trigger event to begin the acquisition
    if (success)
    {
        retCode = AlazarStartCapture(boardHandle);
        if (retCode != ApiSuccess)
        {
            snprintf(errorMessage, 80, "Error: AlazarStartCapture failed -- %s\n", AlazarErrorToText(retCode));
            success = FALSE;
            returnCodeAlazarCapture = -18;
        }
    }

    double* rawCos = (double*)calloc(samplesPerRecord, sizeof(double));
    double* rawData = (double*)calloc(bytesPerBuffer, sizeof(double));


    int Headpointer = 0;
    int Tailpointer = 0;
    double I, Q;
    *returnI = 0;
    *returnQ = 0;

    do_command(viDelayGenerator, "*TRG", errorMessage);

    // Wait for each buffer to be filled, process the buffer, and re-post it to
    // the board.
    if (success)
    {
        //printf("Capturing %d buffers ... press any key to abort\n", buffersPerAcquisition);

        U32 startTickCount = GetTickCount();
        U32 buffersCompleted = 0;
        INT64 bytesTransferred = 0;
        while (buffersCompleted < buffersPerAcquisition)
        {


            // Wait for the buffer at the head of the list of available buffers
            // to be filled by the board.
            bufferIndex = buffersCompleted % BUFFER_COUNT;
            U8* pBuffer = BufferArray[bufferIndex];

            retCode = AlazarWaitAsyncBufferComplete(boardHandle, pBuffer, timeout_ms);
            if (retCode != ApiSuccess)
            {
                snprintf(errorMessage, 80, "Error: AlazarWaitAsyncBufferComplete failed -- %s\n",
                    AlazarErrorToText(retCode));
                success = FALSE;
                returnCodeAlazarCapture = -19;
            }



            if (success)
            {
                // The buffer is full and has been removed from the list
                // of buffers available for the board

                buffersCompleted++;
                bytesTransferred += bytesPerBuffer;

                // TODO: POSSIBLES PROBLEMS
                for (int i = 0; i < bytesPerBuffer; i++) {
                    rawData[i] = .1 * (double)pBuffer[i] / 128. - .1;
                }

                // NOTE:
                //
                // While you are processing this buffer, the board is already filling the next
                // available buffer(s).
                //
                // You MUST finish processing this buffer and post it back to the board before
                // the board fills all of its available DMA buffers and on-board memory.
                //
                // Records are arranged in the buffer as follows: R0A, R0B, ..., R1A, R1B, ...
                // with RXY the record number X of channel Y.
                //
                // Sample code are stored as 8-bit values. 
                // Sample codes are unsigned by default. As a result:
                // - a sample code of 0x00 represents a negative full scale input signal.
                // - a sample code of 0x80 represents a ~0V signal.
                // - a sample code of 0xFF represents a positive full scale input signal.  
                // Write record to file
                if(save) {
                    size_t bytesWritten = fwrite(pBuffer, sizeof(BYTE), bytesPerBuffer, fpData);
                    if (bytesWritten != bytesPerBuffer)
                    {
                        snprintf(errorMessage, 80, "Error: Write buffer %u failed -- %u\n", buffersCompleted,
                            GetLastError());
                        success = FALSE;
                        returnCodeAlazarCapture = -20;
                    }
                }
            }

            // Add the buffer to the end of the list of available buffers.
            if (success)
            {
                retCode = AlazarPostAsyncBuffer(boardHandle, pBuffer, bytesPerBuffer);
                if (retCode != ApiSuccess)
                {
                    snprintf(errorMessage, 80, "Error: AlazarPostAsyncBuffer failed -- %s\n",
                        AlazarErrorToText(retCode));
                    success = FALSE;
                    returnCodeAlazarCapture = -21;
                }
            }

            if (success) {

                // TODO: POSSIBLES PROBLEMS
                // TODO: Process sample data in this buffer
                for (int recordIndex = 1; recordIndex < recordsPerBuffer; recordIndex++) {

                    // TODO: POSSIBLES PROBLEMS MAINLY THIS
                    double* rawSin = rawData + 2 * recordIndex * samplesPerRecord;
                    double* signal = rawData + (2 * recordIndex + 1) * samplesPerRecord;

                    // I have inverted cos with sin because my reference is the Q instead of I. 
                    GetCosFromSin(rawSin, period, rawCos, waveformHeadCut, samplesPerRecord, &Headpointer, &Tailpointer);

                    GetIQ(rawSin, rawCos, signal, Headpointer, Tailpointer, &I, &Q);


                    *returnI += I / averagesDone;
                    *returnQ += Q / averagesDone;
                }

            }

            // If the acquisition failed, exit the acquisition loop
            if (!success)
                break;

            // Display progress
            //printf("Completed %u buffers\r", buffersCompleted);
        }

        // Display results
        double transferTime_sec = (GetTickCount() - startTickCount) / 1000.;
        //printf("Capture completed in %.2lf sec\n", transferTime_sec);

        double buffersPerSec;
        double bytesPerSec;
        double recordsPerSec;
        U32 recordsTransferred = recordsPerBuffer * buffersCompleted;

        if (transferTime_sec > 0.)
        {
            buffersPerSec = buffersCompleted / transferTime_sec;
            bytesPerSec = bytesTransferred / transferTime_sec;
            recordsPerSec = recordsTransferred / transferTime_sec;
        }
        else
        {
            buffersPerSec = 0.;
            bytesPerSec = 0.;
            recordsPerSec = 0.;
        }

        //printf("Captured %u buffers (%.4g buffers per sec)\n", buffersCompleted, buffersPerSec);
        //printf("Captured %u records (%.4g records per sec)\n", recordsTransferred, recordsPerSec);
        //printf("Transferred %I64d bytes (%.4g bytes per sec)\n", bytesTransferred, bytesPerSec);
    }

    // Abort the acquisition
    retCode = AlazarAbortAsyncRead(boardHandle);
    if (retCode != ApiSuccess)
    {
        snprintf(errorMessage, 80, "Error: AlazarAbortAsyncRead failed -- %s\n", AlazarErrorToText(retCode));
        success = FALSE;
        returnCodeAlazarCapture = -21;
    }

    // Free all memory allocated
    for (bufferIndex = 0; bufferIndex < BUFFER_COUNT; bufferIndex++)
    {
        if (BufferArray[bufferIndex] != NULL)
        {
            AlazarFreeBufferU8Ex(boardHandle, BufferArray[bufferIndex]);

        }
    }

    // Close the data file

    if (fpData != NULL)
        fclose(fpData);

    viClose(viDelayGenerator);
    viClose(RM);

    free(rawCos);
    free(rawData);

    return returnCodeAlazarCapture;
}

/* Handle VISA errors.
* --------------------------------------------------------------- */
void error_handler(ViSession vi, ViStatus err, char* error)
{
    char err_msg[1024] = {0};

    viStatusDesc(vi, err, err_msg);

    strcpy(error, err_msg);
}

int do_command(ViSession vi, char* command, char* errorMessage)
{
    char message[80];
    strcpy(message, command);
    strcat(message, "\n");

    ViStatus err = viPrintf(vi, message);
    if (err != VI_SUCCESS) {
        error_handler(vi, err, errorMessage);
        return -4;
    }

    return 0;
}

void check_instrument_errors(ViSession vi)
{
    char str_err_val[256] = { 0 };
    char str_out[800] = "";
    char err_msg[1024] = { 0 };

    ViStatus err = viQueryf(vi, ":SYSTem:ERRor? STRing\n", "%t", str_err_val);
    if (err != VI_SUCCESS) {
        viStatusDesc(vi, err, err_msg);
        printf("VISA Error: %s\n", err_msg);
    };
    while (strncmp(str_err_val, "0,", 2) != 0)
    {
        strcat(str_out, ", ");
        strcat(str_out, str_err_val);
        err = viQueryf(vi, ":SYSTem:ERRor? STRing\n", "%t", str_err_val);
        if (err != VI_SUCCESS) {
            viStatusDesc(vi, err, err_msg);
            printf("VISA Error: %s\n", err_msg);
        };
    }
    if (strcmp(str_out, "") != 0)
    {
        printf("INST Error%s\n", str_out);
        err = viFlush(vi, VI_READ_BUF);

        if (err != VI_SUCCESS) {
            viStatusDesc(vi, err, err_msg);
            printf("VISA Error: %s\n", err_msg);
        };
        err = viFlush(vi, VI_WRITE_BUF);
        if (err != VI_SUCCESS) {
            viStatusDesc(vi, err, err_msg);
            printf("VISA Error: %s\n", err_msg);
        };
    }
}