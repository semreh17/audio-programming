/* ======== includes ========= */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../audio/miniaudio.h"

#include "raylib.h"

typedef struct device {
    ma_device_config deviceConfig;
    ma_device actualDevice;
}device;

/* =========== wave file header ============= */
static const char* chunk_id   = "RIFF";
static const char* chunk_size = "----";
static const char* format     = "WAVE";

static const char* subchunk1_id        = "fmt ";
static const int subchunk1_size        = 16;
static const short int audio_format    = 1;
static const short int num_channels    = 1;
static const int sample_rate           = 44100;
static const short int bits_per_sample = 16;
static const int byte_rate             = (sample_rate * bits_per_sample * num_channels) / 8;
static const short int block_align     = (bits_per_sample * num_channels) / 8;

static const char* subchunk2_id   = "data";
static const char* subchunk2_size = "----";

static void wavHeaderFiller(FILE *fd) {
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
}

static void deviceSetup(device *ma_device, bool duplexFlag, ma_device_data_proc data_callback, FILE *fd) {
    if (duplexFlag) {
        ma_device->deviceConfig = ma_device_config_init(ma_device_type_duplex);
        ma_device->deviceConfig.capture.shareMode = ma_share_mode_shared;
        ma_device->deviceConfig.playback.format = ma_format_s16;
        ma_device->deviceConfig.playback.channels = 1;
    }
    else ma_device->deviceConfig = ma_device_config_init(ma_device_type_capture);

    ma_device->deviceConfig.capture.format = ma_format_s16;
    ma_device->deviceConfig.capture.channels = 1;
    ma_device->deviceConfig.sampleRate = 44100;
    ma_device->deviceConfig.dataCallback = data_callback;
    ma_device->deviceConfig.pUserData = fd;
}
