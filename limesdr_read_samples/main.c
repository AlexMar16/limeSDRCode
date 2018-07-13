#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <complex.h>
#include <math.h>

#define NUM_SAMPLES 2040

struct SoapySDRDevice *Setup(void);
void DeviceInfo();
void Read_n_samples(struct SoapySDRDevice *sdr, int num_samples, double freq, complex float *buffer);
void Close();

size_t length;

int main(int argc, char *argv[]) {
    struct SoapySDRDevice *sdr = Setup();
    DeviceInfo(sdr);

    for(int i = 0; i < 100; i++) {
        complex float *buffn = malloc(sizeof(complex float)*NUM_SAMPLES);
        Read_n_samples(sdr, NUM_SAMPLES, 433.92e6, buffn);

        for(int j = 0; j < NUM_SAMPLES; j++) {

            float amplitude = sqrt(pow(creal(buffn[j]), 2) + pow(cimag(buffn[j]), 2));
            printf("--%f\n", amplitude);

        }

        free(buffn);
    }

    Close(sdr);
    return 0;
}

struct SoapySDRDevice *Setup(void) {
    //enumerate devices
    SoapySDRKwargs *results = SoapySDRDevice_enumerate(NULL, &length);
    for (size_t i = 0; i < length; i++) {
        printf("Found device #%d: ", (int)i);
        for (size_t j = 0; j < results[i].size; j++) {
            printf("%s=%s, ", results[i].keys[j], results[i].vals[j]);
        }
        printf("\n");
    }
    SoapySDRKwargsList_clear(results, length);
    //create device instance
    //args can be user defined or from the enumeration result
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args,"driver", "lime");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);
    
    if(sdr == NULL) {
        printf("SoapySDRDevice_make fail: %s\n", SoapySDRDevice_lastError());
        //return EXIT_FAILURE;
    }

    return sdr;
}

void DeviceInfo(struct SoapySDRDevice *sdr) {
    //query device info
    char** names = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx antennas: ");
    for (size_t i = 0; i < length; i++) printf("%s, ", names[i]);
    printf("\n");

    SoapySDRStrings_clear(&names,length);
    names = SoapySDRDevice_listGains(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx gains: ");
    for (size_t i = 0; i < length; i++) printf("%s, ", names[i]);
    printf("\n");

    SoapySDRStrings_clear(&names, length);
    SoapySDRRange *ranges = SoapySDRDevice_getFrequencyRange(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx freq ranges: ");
    for(size_t i = 0; i < length; i++) printf("[%g Hz -> %g Hz], ", ranges[i].minimum, ranges[i].maximum);
    printf("\n");

    free(ranges);
}

void Read_n_samples(struct SoapySDRDevice *sdr, int num_samples, double freq, complex float *buffer){
    // check freq is within range
    if (freq <10e3) {
        freq = 10e3;
        printf("Frequency out of range setting min: \n");
    }
    if (freq >3.8e9) {
        freq = 3.8e9;
        printf("Frequency out of range setting max: \n");
    }

    //apply settings
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, 1e6) != 0) {
        printf("setSampleRate fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setAntenna(sdr, SOAPY_SDR_RX, 0, "LNAL") != 0) {
        printf("setAntenna fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setGain(sdr, SOAPY_SDR_RX, 0, 20) != 0) {
        printf("setAntenna fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, freq, NULL) != 0) {
        printf("setFrequency fail: %s\n", SoapySDRDevice_lastError());
    }

    //setup a stream (complex floats)
    SoapySDRStream *rxStream;
    if (SoapySDRDevice_setupStream(sdr, &rxStream, SOAPY_SDR_RX, SOAPY_SDR_CF32, NULL, 0, NULL) != 0) {
        printf("setupStream fail: %s\n", SoapySDRDevice_lastError());
    }

    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); //start streaming
    void *buffs[] = {buffer}; //array of buffers
    int flags; //flags set by receive operation
    long long timeNs; //timestamp for receive buffer

    printf("reading %d samples\n", num_samples);
    int ret = SoapySDRDevice_readStream(sdr, rxStream, buffs, num_samples, &flags, &timeNs, 1e9);

    //shutdown the stream
    SoapySDRDevice_deactivateStream(sdr, rxStream, 0, 0); //stop streaming
    SoapySDRDevice_closeStream(sdr, rxStream);
}

void Close(struct SoapySDRDevice *sdr2) {
    //cleanup device handle
    SoapySDRDevice_unmake(sdr2);
    printf("Done\n");
    //return EXIT_SUCCESS;
}   