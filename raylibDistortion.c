/* code that tries to emulate the logic behind a simple
 * distortion pedal that takes the audio samples, adds
 * gain and apply clipping 
 * */


/* ======== includes ========= */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "../audio/miniaudio.h"

#include "raylib.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_SAMPLES 2048

/* ========= global variables ========== */
short displayData[MAX_SAMPLES];
volatile int dataIndex = 0;
float global_time = 0.0f;
float drive_pot = 10.0f;

/* =========== wave file header ============= */
const char* chunk_id   = "RIFF";
const char* chunk_size = "----";
const char* format     = "WAVE";

const char* subchunk1_id        = "fmt ";
const int subchunk1_size        = 16;
const short int audio_format    = 1;
const short int num_channels    = 1;
const int sample_rate           = 44100;
const short int bits_per_sample = 16;
const int byte_rate             = (sample_rate * bits_per_sample * num_channels) / 8;
const short int block_align     = (bits_per_sample * num_channels) / 8;

const char* subchunk2_id   = "data";
const char* subchunk2_size = "----";

void wavHeaderFiller(FILE *fd) {
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

/* ====== callback ======= */
// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;
    if(fd == NULL) return;

    short* inputSamples = (short*)pInput;
    short denormalized_sample[frameCount];

    for(int i = 0; i < frameCount; i++) {
        float output = 0.0f;
        float x = inputSamples[i] / 32768.0f;

        // pre-gain
        x = x * drive_pot;
        float saturation = 1.0f - expf(-fabsf(x));

        if (x > 0) {
            denormalized_sample[i] = (short)(saturation * 32767.0f);
        } else {
            denormalized_sample[i] = (short)(-saturation * 32767.0f);
        }

        // its not needed to denormalize since the applied function
        // already returns values between -1 and 1
        // denormalized_sample[i] = (short) (x * 32768.0f);

        // thing for the raylib plot
        if (dataIndex < MAX_SAMPLES) {
            displayData[dataIndex] = denormalized_sample[i];
            dataIndex++;
        }
        if (dataIndex >= MAX_SAMPLES) {
            dataIndex = 0;
        }
    }

    // here frameCount frames (aka sample) are ridden from pInput and stored inside the filde descriptor fd
    fwrite(denormalized_sample, sizeof(short), frameCount, fd);
}


int main(int argc, char** argv) {

    FILE *fd = fopen("test.wav", "wb");

    if (fd == NULL) {
        return -1;
    }

    // filling the wave header
    int file_start = ftell(fd);

    wavHeaderFiller(fd);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C Audio Visualizer");
    SetTargetFPS(64);

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
    while (!WindowShouldClose()) {

        if (IsKeyDown(KEY_UP)) drive_pot += 0.5f;
        if (IsKeyDown(KEY_DOWN)) drive_pot -= 0.5f;

        BeginDrawing();
        ClearBackground(BLACK);


        char driveText[32];
        sprintf(driveText, "drive: %.1f", drive_pot);
        DrawText(driveText, 10, 40, 20, RED);
        DrawText("speak...", 10, 10, 20, DARKGRAY);

        int centerY = SCREEN_HEIGHT / 2;

        for (int i = 0; i < MAX_SAMPLES - 1; i++) {
            // Normalizziamo l'altezza: 32767 -> Metà schermo
            float y1 = centerY + (displayData[i] / 32768.0f) * 200;
            float y2 = centerY + (displayData[i+1] / 32768.0f) * 200;

            // Scaliamo la X per riempire lo schermo
            float x1 = (float)i / MAX_SAMPLES * SCREEN_WIDTH;
            float x2 = (float)(i+1) / MAX_SAMPLES * SCREEN_WIDTH;

            DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 2.0f, GREEN);
        }
        EndDrawing();
    }
    int end_audio = ftell(fd);


    fseek(fd, -4, start_audio);
    int subchunk2_actual_size = end_audio - start_audio;
    fwrite(&subchunk2_actual_size, sizeof(int), 1, fd);

    fseek(fd, 4, file_start);
    int chunk_actual_size = end_audio - 8;
    fwrite(&chunk_actual_size, sizeof(int), 1, fd);

    ma_device_uninit(&actualDevice);
    fclose(fd);
}
