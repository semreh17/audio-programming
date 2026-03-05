/* ======== includes ========= */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../audio/miniaudio.h"

#include "raylib.h"

/* =========== wave file header ============= */
extern const char* chunk_id   = "RIFF";
extern const char* chunk_size = "----";
extern const char* format     = "WAVE";

extern const char* subchunk1_id        = "fmt ";
extern const int subchunk1_size        = 16;
extern const short int audio_format    = 1;
extern const short int num_channels    = 1;
extern const int sample_rate           = 44100;
extern const short int bits_per_sample = 16;
extern const int byte_rate             = (sample_rate * bits_per_sample * num_channels) / 8;
extern const short int block_align     = (bits_per_sample * num_channels) / 8;

extern const char* subchunk2_id   = "data";
extern const char* subchunk2_size = "----";

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
