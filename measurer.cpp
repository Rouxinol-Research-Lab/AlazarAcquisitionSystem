#include "measurer.hpp"
#include <chrono>
#include <thread>



// samples exported function
int GetIQ(double* RawSin, double* RawCos, double* Data, int Headpointer, int Tailpointer, double* I, double* Q )
{

    double sumI=0,sumQ=0;
    for(int i=Headpointer; i<Tailpointer; i++)
    {
        sumI+=RawSin[i]*Data[i];
        sumQ+=RawCos[i]*Data[i];
    }
    *I=sumI/(Tailpointer-Headpointer+1)*2;
    *Q=sumQ/(Tailpointer-Headpointer+1)*2;
    return 0;
}

int GetCosFromSin(double* rawSin, double Period , double* rawCos, int ChopHead, int ChopTail,int* Headpointer, int* Tailpointer)
{
    int i;

    int Head = 0;
    int Tail=ChopTail; // pointer for head and tail of array wanted;
    double ShiftDist;

    for(i=ChopHead; i<ChopTail; i++)//find the Head, move Tail to the last one
    {
        if(( rawSin[i] < 0 ) && ( rawSin[i+1]>=0) )
        {
                Head=i+1;
                break;
        }
    }

    for(i=ChopTail; i>ChopHead; i--)//move Tail one period back
    {
        if((rawSin[i]<0)&&(rawSin[i+1]>=0))
        {
            Tail=i;
            break;
        }
    };


    ShiftDist=Period/4;
    for(i= Head; i< Tail; i++)
    {
        int position=int(ShiftDist);
        rawCos[i]=rawSin[i+position]+(rawSin[i+position+1]-rawSin[i+position])*(ShiftDist-position);
    }

    *Headpointer = Head;
    *Tailpointer = Tail;
    return 0;
}

bool gCloseResourceManager = false; // Question mark.
ViSession gResourceManager = 0;
bool gCloseOscilloscopeSession = false; // Question mark.
ViSession gOscilloscopeSession = 0;
bool gCloseIeeeblock = false;
ViPBuf gIeeeblock = 0;
int gIeeeblock_size = 0;
bool gCloseReferenceBlock = false;
double* gReferenceBlock = 0;
bool gCloseSignalBlock = false;
double* gSignalBlock = 0;
bool gCloseDerivateBlock = false;
signed char* gDerivateBlock = 0;
bool gCloseFallingEdges = false;
int* gFallingEdges = 0;
bool gCloseRisingEdges = false;
int* gRisingEdges = 0;
bool gCloseCosRaw = false;
double* gCosRaw = 0;

int gTrigger_normalizer_treshold = 30; // The level I think that means high signal. I used that to normalize the trigger signal.
int gTrigger_max_value = 100; // Max value set to trigger signal.
int gTrigger_treshold = 40; // That is the true treshold that I used to detect the trigger.
int gTrigger_pulse_min = 3000; // Minimal pulse size between rising signals ( or falling signals).

int gRisingSize = 0;
int gFallingSize = 0;
int gCosRawSize = 0;

double gPeriod = 10.0;
int gWaveformHeadCut = 0;
int gWaveformTailCut = 0;


int open_resources(int ieeeblock_size, int cos_raw_size, int edges_size, int trigger_normalizer_treshold, int trigger_max, int trigger_treshold, int trigger_pulse_min, double period, int waveformHeadCut, int waveformTailCut, char* address, char* errorMessage) {
    ViStatus err;
    int data_length;
    int header_length;
    int i,j;
    char message[80] = { 0 };

    gTrigger_normalizer_treshold = trigger_normalizer_treshold;
    gTrigger_max_value = trigger_max;
    gTrigger_treshold = trigger_treshold;
    gTrigger_pulse_min = trigger_pulse_min;
    gPeriod = period;
    gWaveformHeadCut = waveformHeadCut;
    gWaveformTailCut = waveformTailCut;
    gRisingSize = edges_size;
    gFallingSize = edges_size;
    gCosRawSize = cos_raw_size;

    if (gCloseResourceManager) {
        strcat(message, "Resource Manager is already opened.");
        strcpy(errorMessage, message);
        return -14;
    }

    if (gCloseOscilloscopeSession) {
        strcat(message, "Oscilloscope session is already opened.");
        strcpy(errorMessage, message);
        return -15;
    }

    if (gCloseIeeeblock) {
        strcat(message, "ieeeblock is already allocated.");
        strcpy(errorMessage, message);
        return -16;
    }

    if (gCloseReferenceBlock) {
        strcat(message, "Reference block is already allocated.");
        strcpy(errorMessage, message);
        return -18;
    }

    if (gCloseSignalBlock) {
        strcat(message, "Signal block is already allocated.");
        strcpy(errorMessage, message);
        return -19;
    }

    if (gCloseDerivateBlock) {
        strcat(message, "Derivate block is already allocated.");
        strcpy(errorMessage, message);
        return -20;
    }

    if (gCloseFallingEdges) {
        strcat(message, "Falling block is already allocated.");
        strcpy(errorMessage, message);
        return -21;
    }

    if (gCloseRisingEdges) {
        strcat(message, "Rising block is already allocated.");
        strcpy(errorMessage, message);
        return -22;
    }

    if (gCloseCosRaw) {
        strcat(message, "Cos Raw block is already allocated.");
        strcpy(errorMessage, message);
        return -23;
    }

    err = viOpenDefaultRM(&gResourceManager);
    if (err != VI_SUCCESS) {
        strcat(message, "Could not open Resource Manager.");
        strcpy(errorMessage, message);
        return -12;
    }
    gCloseResourceManager = true;

    err = viOpen(gResourceManager, address, VI_NULL, VI_NULL, &gOscilloscopeSession);
    if (err != VI_SUCCESS) {
        error_handler(gOscilloscopeSession, err, errorMessage);
        return -13;
    }
    gCloseOscilloscopeSession = true;

    int returnValue = do_command(gOscilloscopeSession, ":WAV:FORMAT BYTE", errorMessage);
    if (returnValue < 0) {
        strcat(message, "Could change format.");
        strcpy(errorMessage, message);
        return -21;
    }

    returnValue = do_command(gOscilloscopeSession, ":WAV:SOUR CHAN3", errorMessage);
    if (returnValue < 0) {
        strcat(message, "Could change to source 3.");
        strcpy(errorMessage, message);
        return -21;
    }

    gIeeeblock_size = ieeeblock_size;
    gIeeeblock = (ViPBuf) new ViBuf[gIeeeblock_size];
    memset(gIeeeblock, 0, ieeeblock_size);
    gCloseIeeeblock = true;

    returnValue = do_query_ieeeblock_words(gOscilloscopeSession, gIeeeblock, gIeeeblock_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) {
        strcat(message, "Could request data from oscilloscope");
        strcpy(errorMessage, message);
        return -17;
    }


    header_length = (char)gIeeeblock[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.

    gReferenceBlock = new double[data_length];
    memset(gReferenceBlock, 0, data_length);
    gCloseReferenceBlock = true;

    gSignalBlock = new double[data_length];
    memset(gSignalBlock, 0, data_length);
    gCloseSignalBlock = true;

    gDerivateBlock = new signed char[data_length - 1];
    gCloseDerivateBlock = true;

    gRisingEdges = (int*) new int[gRisingSize];
    memset(gRisingEdges, 0, gRisingSize);
    gCloseRisingEdges = true;

    gFallingEdges = (int*) new int[gFallingSize];
    memset(gFallingEdges, 0, gFallingSize);
    gCloseFallingEdges = true;

    gCosRaw = (double*) new double[gCosRawSize];
    memset(gCosRaw, 0, gCosRawSize);
    gCloseCosRaw = true;

    return 0;
}

int doMeasure(double* returnI, double* returnQ, double* returnV, int* averagesDone, char* errorMessage) {
    int returnValue = 0;
    int data_length = 0;
    int header_length = 0;
    int i, j;


    if (!(gCloseOscilloscopeSession &&
        gCloseResourceManager &&
        gCloseIeeeblock &&
        gCloseReferenceBlock &&
        gCloseSignalBlock &&
        gCloseDerivateBlock &&
        gCloseFallingEdges &&
        gCloseRisingEdges &&
        gCloseCosRaw)) return -33;

    returnValue = do_command(gOscilloscopeSession, ":WAV:FORMAT BYTE", errorMessage);
    if (returnValue < 0) return -34;


    /*********************************************************************/
    /*                           Reference                               */
    /*********************************************************************/
    returnValue = do_command(gOscilloscopeSession, ":WAV:SOUR CHAN1", errorMessage);
    if (returnValue < 0) return -35;

    double y_increment;
    returnValue = do_query_number(gOscilloscopeSession, ":WAV:YINC?", &y_increment, errorMessage);
    if (returnValue < 0) return  -36;

    double y_origin;
    returnValue = do_query_number(gOscilloscopeSession, ":WAV:YOR?", &y_origin, errorMessage);
    if (returnValue < 0) return -37;

    returnValue = do_query_ieeeblock_words(gOscilloscopeSession, gIeeeblock, gIeeeblock_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) return -38;

    header_length = (char)gIeeeblock[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.

    ViPBuf waveform = gIeeeblock + header_length + 2;

    for (i = 0; i < data_length; i++) {
        gReferenceBlock[i] = y_increment * ((signed char)waveform[i]) + y_origin;
    }

    /*********************************************************************/
    /*                           Signal                                  */
    /*********************************************************************/
    returnValue = do_command(gOscilloscopeSession, ":WAV:SOUR CHAN2", errorMessage);
    if (returnValue < 0) return -39;

    returnValue = do_query_number(gOscilloscopeSession, ":WAV:YINC?", &y_increment, errorMessage);
    if (returnValue < 0) return  -40;

    returnValue = do_query_number(gOscilloscopeSession, ":WAV:YOR?", &y_origin, errorMessage);
    if (returnValue < 0) return -41;

    returnValue = do_query_ieeeblock_words(gOscilloscopeSession, gIeeeblock, gIeeeblock_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) return -42;

    header_length = (char)gIeeeblock[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.

    waveform = gIeeeblock + header_length + 2;

    for (i = 0; i < data_length; i++) {
        gSignalBlock[i] = y_increment * ((signed char)waveform[i]) + y_origin;
    }




    /*********************************************************************/
    /*                           Trigger                                 */
    /*********************************************************************/
    returnValue = do_command(gOscilloscopeSession, ":WAV:SOUR CHAN3", errorMessage);
    if (returnValue < 0) return  -43;

    returnValue = do_query_ieeeblock_words(gOscilloscopeSession, gIeeeblock, gIeeeblock_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) return -44;

    header_length = gIeeeblock[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.


    /*********************************************************************/
    /*                           Derivate                                */
    /*********************************************************************/

    ViPBuf trigger_waveform = gIeeeblock + header_length + 2;

    for (i = 0; i < data_length; i++) {

        if ((signed char)trigger_waveform[i] > gTrigger_normalizer_treshold) trigger_waveform[i] = gTrigger_max_value;
        else if ((signed char)trigger_waveform[i] < gTrigger_max_value) trigger_waveform[i] = 0;
    }


    for (i = 0; i < data_length - 1; i++) {
        gDerivateBlock[i] = trigger_waveform[i + 1] - trigger_waveform[i];
    }

    /*********************************************************************/
    /*                           Find Waveforms                          */
    /*********************************************************************/


    int risingIndex = 0;
    int fallingIndex = 0;

    for (i = 0; i < data_length - 1; i++) {
        if (gDerivateBlock[i] > gTrigger_treshold) {

            if (risingIndex == 0) {
                gRisingEdges[risingIndex] = i;
                risingIndex++;
            }
            else if (i - gRisingEdges[risingIndex - 1] > gTrigger_pulse_min) {
                gRisingEdges[risingIndex] = i;
                risingIndex++;
            }

        }
        else if (gDerivateBlock[i] < -gTrigger_treshold) {

            if (fallingIndex == 0) {
                gFallingEdges[fallingIndex] = i;
                fallingIndex++;
            }
            else if (i - gFallingEdges[fallingIndex - 1] > gTrigger_pulse_min) {
                gFallingEdges[fallingIndex] = i;
                fallingIndex++;
            }
        }
    }

    int risingEdgesEnd = risingIndex - 1;
    int fallingEdgesEnd = fallingIndex - 1;

    int fallingEdgesBegin = 0;
    int risingEdgesBegin = 0;

    if (gRisingEdges[0] > gFallingEdges[0]) fallingEdgesBegin++;
    if (gRisingEdges[risingEdgesEnd] > gFallingEdges[fallingEdgesEnd]) risingEdgesEnd--;

    int pulseSize = 0;
    int aux = 0;
    pulseSize = gFallingEdges[fallingEdgesBegin] - gRisingEdges[risingEdgesBegin];
    for (i = risingEdgesBegin + 1, j = fallingEdgesBegin + 1; i <= risingEdgesEnd; i++, j++) {
        aux = gFallingEdges[j] - gRisingEdges[i];
        if (aux < pulseSize && aux > 0) pulseSize = aux;
    }

    double* rawSin = gReferenceBlock;
    int Headpointer = 0;
    int Tailpointer = 0;
    double I, Q;

    char buffer[80] = { 0 };

    *averagesDone = risingEdgesEnd - risingEdgesBegin + 1;


    *returnI = 0;
    *returnQ = 0;
    *returnV = 0;


    for (i = risingEdgesBegin; i <= risingEdgesEnd; i++) {
        rawSin = gReferenceBlock + gRisingEdges[i];


        GetCosFromSin(rawSin, gPeriod, gCosRaw, gWaveformHeadCut, pulseSize - gWaveformTailCut, &Headpointer, &Tailpointer);

        GetIQ(rawSin, gCosRaw, gSignalBlock + gRisingEdges[i], Headpointer, Tailpointer, &I, &Q);


        *returnI += I / *averagesDone;
        *returnQ += Q / *averagesDone;
        *returnV += sqrt(I * I + Q * Q) / *averagesDone;
    }

    return 0;
}

int close_resources() {
    if (gCloseOscilloscopeSession) {
        viClose(gOscilloscopeSession);
        gCloseOscilloscopeSession = false;
    }

    if (gCloseResourceManager) {
        viClose(gResourceManager);
        gCloseResourceManager = false;
    }

    if (gCloseIeeeblock) {
        delete[] gIeeeblock;
        gCloseIeeeblock = false;
    }

    if (gCloseReferenceBlock) {
        delete[] gReferenceBlock;
        gCloseReferenceBlock = false;
    }

    if (gCloseSignalBlock) {
        delete[] gSignalBlock;
        gCloseSignalBlock = false;
    }

    if (gCloseDerivateBlock) {
        delete[] gDerivateBlock;
        gCloseDerivateBlock = false;
    }

    if (gCloseFallingEdges) {
        delete[] gFallingEdges;
        gCloseFallingEdges = false;
    }

    if (gCloseRisingEdges) {
        delete[] gRisingEdges;
        gCloseRisingEdges = false;
    }

    if (gCloseCosRaw) {
        delete[] gCosRaw;
        gCloseCosRaw = false;
    }

    return 0;
}

int measure(double* returnI,
    double* returnQ,
    double* returnV,
    char* osc_address,
    int block_size,
    int number_of_averages,
    int* averagesDone,
    char trigger_treshold,
    int pulse_treshold,
    char trigger_normalizer_treshold,
    char* errorMessage) {

    ViSession RM;
    ViSession viOsc;
    ViStatus err;
    int returnValue = 0;
    int data_length = 0;
    int header_length = 0;
    int i,j;

    double* data_reference;
    double* data_signal;
    

    err = viOpenDefaultRM(&RM);
    if (err != VI_SUCCESS) {
        return -1;
    }


    err = viOpen(RM, osc_address, VI_NULL, VI_NULL, &viOsc);
    if (err != VI_SUCCESS) {
        error_handler(viOsc, err, errorMessage);
        returnValue = -2;
        goto close_RM;  // Here I am using gotos to get a correct order of deallocation at the
                        // end of the function. This will be a recurring pattern.
                        // I know there is an usually fear of goto, however I believe
                        // that in the right circumstance it is the right choice.
                        // When using goto I abide by this rules:
                        // always jumps forward;
                        // always to the end of the block;
                        // always use good names for the labels.
    }

    returnValue = do_command(viOsc, ":WAV:FORMAT BYTE", errorMessage);
    if (returnValue < 0) goto close_VI;

    ViPBuf ieeeblock_data = (ViPBuf)calloc(block_size , sizeof(signed char));


    /*********************************************************************/
    /*                           Request Data                            */
    /*********************************************************************/
    /*
    int armed = 1;
    double armed_status = 0;
    
    returnValue = do_command(viOsc, "*CLS", errorMessage); // Clear armed status
    if (returnValue < 0) goto free_ieee_block;
    
    returnValue = do_command(viOsc, ":SINGle", errorMessage);
    if (returnValue < 0) goto free_ieee_block;
    
    returnValue = do_query_number(viOsc, ":AER?", &armed_status, errorMessage);
    if (returnValue < 0) goto free_ieee_block;

    while ((int)armed_status != armed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        returnValue = do_query_number(viOsc, ":AER?", &armed_status, errorMessage);
        if (returnValue < 0) goto free_ieee_block;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));



    */




    /*********************************************************************/
    /*                           Reference                               */
    /*********************************************************************/
    returnValue = do_command(viOsc, ":WAV:SOUR CHAN1", errorMessage);
    if (returnValue < 0) goto free_ieee_block; 

    double y_increment;
    returnValue = do_query_number(viOsc, ":WAV:YINC?", &y_increment, errorMessage);
    if (returnValue < 0) goto free_ieee_block;

    double y_origin;
    returnValue = do_query_number(viOsc, ":WAV:YOR?", &y_origin, errorMessage);
    if (returnValue < 0) goto free_ieee_block;

    returnValue = do_query_ieeeblock_words(viOsc, ieeeblock_data, block_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) goto free_ieee_block;

    header_length = (char)ieeeblock_data[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.

    ViPBuf waveform = ieeeblock_data + header_length + 2;

    data_reference = (double*)malloc(data_length * sizeof(double));
    for (i = 0; i < data_length; i++) {
        data_reference[i] = y_increment * ((signed char)waveform[i]) + y_origin;
    }

    /*********************************************************************/
    /*                           Signal                                  */
    /*********************************************************************/
    returnValue = do_command(viOsc, ":WAV:SOUR CHAN2", errorMessage);
    if (returnValue < 0) goto free_reference;


    returnValue = do_query_number(viOsc, ":WAV:YINC?", &y_increment, errorMessage);
    if (returnValue < 0) goto free_reference;


    returnValue = do_query_number(viOsc, ":WAV:YOR?", &y_origin, errorMessage);
    if (returnValue < 0) goto free_reference;

    returnValue = do_query_ieeeblock_words(viOsc, ieeeblock_data, block_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) goto free_reference;

    header_length = ieeeblock_data[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.

    waveform = ieeeblock_data + header_length + 2;

    data_signal = (double*)malloc(data_length * sizeof(double));
    for (i = 0; i < data_length; i++) {
        data_signal[i] = y_increment * ((signed char)waveform[i]) + y_origin;
    }



    /*********************************************************************/
    /*                           Trigger                                 */
    /*********************************************************************/
    returnValue = do_command(viOsc, ":WAV:SOUR CHAN3", errorMessage);
    if (returnValue < 0) goto free_signal;

    returnValue = do_query_ieeeblock_words(viOsc, ieeeblock_data, block_size, ":WAV:DATA? 1,", &data_length, errorMessage);
    if (returnValue < 0) goto free_signal;

    header_length = ieeeblock_data[1] - 48; // The first byte is just the char '#'. I am subtracting 58 to convert the char '5' to the number 5.
    data_length = data_length - header_length - 3; // Minus 2 because the first two chars are # and the size of the header. Minus 1 because the last is '\n'.




    /*********************************************************************/
    /*                           Derivate                                */
    /*********************************************************************/

    ViPBuf trigger_waveform = ieeeblock_data + header_length + 2;

    for (i = 0; i < data_length ; i++) {

        if ( (signed char)trigger_waveform[i] > trigger_normalizer_treshold) trigger_waveform[i] = 100;
        else if ((signed char)trigger_waveform[i] < 100) trigger_waveform[i] = 0;
        else if ((signed char)trigger_waveform[i] < 0) trigger_waveform[i] = 0;
    }


    signed char* derivate = (signed char*)malloc(sizeof(char) * (data_length - 1));


    for (i = 0; i < data_length - 1; i++) {
        derivate[i] = trigger_waveform[i + 1] - trigger_waveform[i];
    }
    
    

    

    /*********************************************************************/
    /*                           Find Waveforms                          */
    /*********************************************************************/


    int* risingEdges = (int*)malloc(number_of_averages * sizeof(int));
    int* fallingEdges = (int*)malloc(number_of_averages * sizeof(int));
    int pulseSize = 0;


    int risingIndex = 0;
    int fallingIndex = 0;

    for (i = 0; i < data_length - 1; i++) {
        if (derivate[i] > trigger_treshold) {

            if (risingIndex == 0) {
                risingEdges[risingIndex] = i;
                risingIndex++;
            }
            else if (i - risingEdges[risingIndex - 1] > pulse_treshold) {
                risingEdges[risingIndex] = i;
                risingIndex++;
            }

        }
        else if (derivate[i] < -trigger_treshold) {

            if (fallingIndex == 0) {
                fallingEdges[fallingIndex] = i;
                fallingIndex++;
            }
            else if (i - fallingEdges[fallingIndex - 1] > pulse_treshold) {
                fallingEdges[fallingIndex] = i;
                fallingIndex++;
            }
        }
    }

    int risingEdgesSize = risingIndex;
    int fallingEdgesSize = fallingIndex;

    int fallingEdgesBegin = 0;
    int risingEdgesBegin = 0;

    if (risingEdges[0] > fallingEdges[0]) fallingEdgesBegin++;
    if (risingEdges[risingEdgesSize - 1] > fallingEdges[fallingEdgesSize - 1]) risingEdgesSize--;

    
    /*
    printf("rising Edges ");
    for (i = risingEdgesBegin; i < risingEdgesSize; i++) {
        printf("%d ", risingEdges[i]);
    }

    printf("\n");

    printf("falling Edges ");
    for (i = fallingEdgesBegin; i < fallingEdgesSize; i++)
        printf("%d ", fallingEdges[i]); {
    }
    printf("\n");
    */
    

    /* Get the minimal pulse*/
    int aux = 0;
    pulseSize = fallingEdges[fallingEdgesBegin] - risingEdges[risingEdgesBegin];
    /*for (i = risingEdgesBegin + 1, j = fallingEdgesBegin + 1; i < risingEdgesSize && j < fallingEdgesSize; i++, j++) {
        aux = fallingEdges[j] - risingEdges[i];
        if (aux < pulseSize) pulseSize = aux;
    }*/

    double* rawCos = (double*)calloc(pulseSize , sizeof(double));
    double* rawSin = data_reference;
    double period = 10;
    int Headpointer = 0;
    int Tailpointer = 0;
    double I, Q, meanV;

    /*********************************************************************/
    /*                        Save one Waveform                          */
    /*********************************************************************/
#define WAVEFORM_HEAD_CUT 50

    /*
    rawSin = data_reference + risingEdges[0];

    GetCosFromSin(rawSin, period, rawCos, WAVEFORM_HEAD_CUT, pulseSize, &Headpointer, &Tailpointer);

    file = fopen("sin.bin", "wb");
    fwrite(rawSin+ Headpointer, sizeof(double), Tailpointer- Headpointer, file);
    fclose(file);

    file = fopen("cos.bin", "wb");
    fwrite(rawCos + Headpointer, sizeof(double), Tailpointer - Headpointer, file);
    fclose(file);

    file = fopen("data.bin", "wb");
    fwrite(data_signal + risingEdges[0] + Headpointer, sizeof(double), Tailpointer - Headpointer, file);
    fclose(file);
    
    */

    /*********************************************************************/
    /*                           Get I and V                             */
    /*********************************************************************/
    FILE* fileI;
    FILE* fileQ;
    FILE* filePointer;

    char buffer[80] = { 0 };
 
    *averagesDone = risingEdgesSize - risingEdgesBegin;


    *returnI = 0;
    *returnQ = 0;
    *returnV = 0;


    /*
    fileI = fopen("I.csv", "w");
    fileQ = fopen("Q.csv", "w");
    filePointer = fopen("pointers.csv", "w");

    buffer[0] = '\0';
    strcat(buffer, "I\n");
    fwrite(buffer, 1, strlen(buffer), fileI);

    buffer[0] = '\0';
    strcat(buffer, "Q\n");
    fwrite(buffer, 1, strlen(buffer), fileQ);

    buffer[0] = '\0';
    strcat(buffer, "Counter, Head,Tail, Edges\n");
    fwrite(buffer, 1, strlen(buffer), filePointer);


    fclose(filePointer);
    fclose(fileI);
    fclose(fileQ);

    */

    for (i = risingEdgesBegin; i < risingEdgesSize; i++) {
        rawSin = data_reference + risingEdges[i];


        GetCosFromSin(rawSin, period, rawCos, WAVEFORM_HEAD_CUT, pulseSize, &Headpointer, &Tailpointer);

        GetIQ(rawSin, rawCos, data_signal + risingEdges[i], Headpointer, Tailpointer, &I, &Q);
            

        *returnI += I / *averagesDone;
        *returnQ += Q / *averagesDone;

        *returnV += sqrt(I*I + Q*Q) / *averagesDone;
        /*
        fileI = fopen("I.csv", "a");
        fileQ = fopen("Q.csv", "a");
        filePointer = fopen("pointers.csv", "a");
        
        buffer[0] = '\0';
        sprintf(buffer, "%lf\n", I);
        fwrite(buffer, 1, strlen(buffer),fileI);
        
        buffer[0] = '\0';
        sprintf(buffer, "%lf\n", Q);
        fwrite(buffer, 1, strlen(buffer), fileQ);

        buffer[0] = '\0';
        sprintf(buffer, "%d,%d,%d, %d\n", i, Headpointer, Tailpointer, risingEdges[i]);
        fwrite(buffer, 1, strlen(buffer), filePointer);


        fclose(filePointer);
        fclose(fileI);
        fclose(fileQ);

        */
    }







free_cos:
    free(rawCos);
free_edges:
    free(fallingEdges);
    free(risingEdges);
free_derivate:
    free(derivate);
free_signal:
    free(data_signal);
free_reference:
    free(data_reference);
free_ieee_block:
    free(ieeeblock_data);
close_VI:
    viClose(viOsc);
close_RM: 
    viClose(RM);
    

    return returnValue;
}

int do_query_ieeeblock_words(ViSession vi, ViPBuf ieeeblock_data, int data_length, char* query, int* returnedDataLength, char* errorMessage)
{
    char message[80];
    ViStatus err;

    strcpy(message, query);
    strcat(message, "\n");

    err = viPrintf(vi, message);
    if (err != VI_SUCCESS) {
        error_handler(vi, err, errorMessage);
        return -7;
    }

    err = viRead(vi, ieeeblock_data, data_length, (ViPUInt32)returnedDataLength);
    if (err != VI_SUCCESS) {
        error_handler(vi, err, errorMessage);
        return -8;
    }


    return 0;
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

int do_query_number(ViSession vi, char* query, double* result, char* errorMessage)
{

    char message[80];
    strcpy(message, query);
    strcat(message, "\n");

    ViStatus err = viPrintf(vi, message);
    if (err != VI_SUCCESS) {
        error_handler(vi, err, errorMessage);
        return -5;
    }

    err = viScanf(vi, "%lf", result);
    if (err != VI_SUCCESS) {
        error_handler(vi, err, errorMessage);
        return -6;
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