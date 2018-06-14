#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include <stdio.h> //printf
#include <stdlib.h> //free
#include <complex.h>

struct SoapySDRDevice *Setup(void);
void DeviceInfo(struct SoapySDRDevice *sdr);
void Read_1024_samples(struct SoapySDRDevice *sdr, double freq, complex float *buffer);
void Close(struct SoapySDRDevice *sdr2);

const double MAX_FREQ = 3.8e9;
const double Min_FREQ = 10e9;

size_t length;

int main(int argc, char *argv[])
{
    complex float *buf;
    struct SoapySDRDevice *sdr = Setup();
    DeviceInfo(sdr);
    Read_1024_samples(sdr, 433.92e6, buf);
    Close(sdr);
}

struct SoapySDRDevice *Setup(void) {
    //enumerate devices
    SoapySDRKwargs *results = SoapySDRDevice_enumerate(NULL, &length);
    for (size_t i = 0; i < length; i++)
    {
        printf("Found device #%d: ", (int)i);
        for (size_t j = 0; j < results[i].size; j++)
        {
            printf("%s=%s, ", results[i].keys[j], results[i].vals[j]);
        }
        printf("\n");
    }
    SoapySDRKwargsList_clear(results, length);

    //create device instance
    //args can be user defined or from the enumeration result
    SoapySDRKwargs args = {};
    SoapySDRKwargs_set(&args, "driver", "lime");
    SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
    SoapySDRKwargs_clear(&args);

    if (sdr == NULL)
    {
        printf("SoapySDRDevice_make fail: %s\n", SoapySDRDevice_lastError());
        return EXIT_FAILURE;
    }
    return sdr;
}

void DeviceInfo(struct SoapySDRDevice *sdr) {
     //query device info
    char** names = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx antennas: ");
    for (size_t i = 0; i < length; i++) printf("%s, ", names[i]);
    printf("\n");
    SoapySDRStrings_clear(&names, length);

    names = SoapySDRDevice_listGains(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx gains: ");
    for (size_t i = 0; i < length; i++) printf("%s, ", names[i]);
    printf("\n");
    SoapySDRStrings_clear(&names, length);

    SoapySDRRange *ranges = SoapySDRDevice_getFrequencyRange(sdr, SOAPY_SDR_RX, 0, &length);
    printf("Rx freq ranges: ");
    for (size_t i = 0; i < length; i++) printf("[%g Hz -> %g Hz], ", ranges[i].minimum, ranges[i].maximum);
    printf("\n");
    free(ranges);
}


void Read_1024_samples(struct SoapySDRDevice *sdr, double freq, complex float *buffer) {
    // check freq is within range
    if( freq < 10e3) {
        freq = 10e3;
        printf("Frequency out of range setting min: \n");
    }
    if ( freq > MAX_FREQ) {
        freq = MAX_FREQ;
    }
    //apply settings
    if (SoapySDRDevice_setSampleRate(sdr, SOAPY_SDR_RX, 0, 1e6) != 0)
    {
        printf("setSampleRate fail: %s\n", SoapySDRDevice_lastError());
    }
    if (SoapySDRDevice_setFrequency(sdr, SOAPY_SDR_RX, 0, 912.3e6, NULL) != 0)
    {
        printf("setFrequency fail: %s\n", SoapySDRDevice_lastError());
    }

    //setup a stream (complex floats)
    SoapySDRStream *rxStream;
    if (SoapySDRDevice_setupStream(sdr, &rxStream, SOAPY_SDR_RX, SOAPY_SDR_CF32, NULL, 0, NULL) != 0)
    {
        printf("setupStream fail: %s\n", SoapySDRDevice_lastError());
    }
    SoapySDRDevice_activateStream(sdr, rxStream, 0, 0, 0); //start streaming

    //create a re-usable buffer for rx samples
    complex float buff[1024];

    //receive some samples
    for (size_t i = 0; i < 10; i++)
    {
        void *buffs[] = {buff}; //array of buffers
        int flags; //flags set by receive operation
        long long timeNs; //timestamp for receive buffer
        int ret = SoapySDRDevice_readStream(sdr, rxStream, buffs, 1024, &flags, &timeNs, 100000);
        printf("ret=%d, flags=%d, timeNs=%lld\n", ret, flags, timeNs);
        for(size_t j = 0; j < ret; j++) {
            printf("inside: %d \n", buffs[j]);
        }
    }

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