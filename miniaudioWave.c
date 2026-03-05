#include <stdio.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../audio/miniaudio.h"

void* buffer;


// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;

    if(fd == NULL) return;

    // here frameCount frames (aka sample) are ridden from pInput and stored inside the filde descriptor fd
    fwrite(pInput, sizeof(short), frameCount, fd);
}

// =========== wave file header =============
const char* chunk_id   = "RIFF";
const char* chunk_size = "----";
const char* format     = "WAVE";

const char* subchunk1_id  = "fmt ";
const int subchunk1_size  = 16;
const short int audio_format    = 1; 
const short int num_channels    = 1;
const int sample_rate     = 44100;
const short int bits_per_sample = 16;
const int byte_rate       = (sample_rate * bits_per_sample * num_channels) / 8;
const short int block_align     = (bits_per_sample * num_channels) / 8;

const char* subchunk2_id = "data";
const char* subchunk2_size = "----";

int main(int argc, char** argv) {

    FILE *fd = fopen("test.wav", "wb");

    if (fd == NULL) {
        return -1;
    }

    // filling the wave header
    int file_start = ftell(fd);
    fwrite(chunk_id, strlen(chunk_id), 1, fd);
    fwrite(chunk_size, strlen(chunk_size), 1, fd);
    fwrite(format, strlen(format), 1, fd);
    fwrite(subchunk1_id, strlen(subchunk1_id), 1, fd);
    fwrite(&subchunk1_size, sizeof(subchunk1_size), 1, fd);
    fwrite(&audio_format, sizeof(audio_format), 1, fd);
    fwrite(&num_channels, sizeof(num_channels), 1, fd);
    fwrite(&sample_rate, sizeof(sample_rate), 1, fd);
    fwrite(&byte_rate, sizeof(byte_rate), 1, fd);
    fwrite(&block_align, sizeof(block_align), 1, fd);
    fwrite(&bits_per_sample, sizeof(bits_per_sample), 1, fd);
    fwrite(subchunk2_id, strlen(subchunk2_id), 1, fd);
    fwrite(subchunk2_size, strlen(subchunk2_size), 1, fd);


    ma_device_config deviceConfig;
    ma_device actualDevice;

    // device configuration
    deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.format = ma_format_s16;
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate = 44100;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = fd;

    if (ma_device_init(NULL, &deviceConfig, &actualDevice) != MA_SUCCESS) {
        printf("initialization error");
        return -1;
    }

    int start_audio = ftell(fd);

    ma_device_start(&actualDevice);
    printf("press a key to stop \n");
    getchar();

    int end_audio = ftell(fd);

    fseek(fd, -4, start_audio);
    int subchunk2_actual_size = end_audio - start_audio;
    fwrite(&subchunk2_actual_size, sizeof(int), 1, fd);

    fseek(fd, 4, file_start);
    int chunk_actual_size = end_audio - 8;
    fwrite(&chunk_actual_size, sizeof(int), 1, fd);
    
    fclose(fd);
}
