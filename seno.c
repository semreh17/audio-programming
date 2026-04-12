/* this small piece of code that allows to visualize a wave with a certain
 * frequency, it is possibile to increase or decrease the frequency to see
 * how the wave changes
 */

/* ======== includes ========= */
#include "waveFileHeader.h"
#include <stdbool.h>
#include <string.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_SAMPLES 1024

/* ========= global variables ========== */
short displayData[MAX_SAMPLES];
volatile int dataIndex = 0; // volatile so the compiler won't optimize it
float phase = 0.0f;
float frequency = 440.0f; // 440 Hz (A4 note)
float amplitude = 0.8f;   // Volume (0.0 a 1.0)


/* ====== callback ======= */
// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;
    if(fd == NULL) return;

    short denormalized_sample[frameCount];

    float phase_increment = (2.0f * M_PI * frequency) / sample_rate;

    for (int i = 0; i < frameCount; i++) {
        float value = sinf(phase);
        phase += phase_increment;

        short final_sample = (short)(value * amplitude * 32767.0f);

        denormalized_sample[i] = final_sample;
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
    memcpy(pOutput, denormalized_sample, sizeof(short) * frameCount);
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

    struct device ma_device;

    // device configuration
    deviceSetup(&ma_device, true, data_callback, fd);

    if (ma_device_init(NULL, &ma_device.deviceConfig, &ma_device.actualDevice) != MA_SUCCESS) {
        printf("initialization error");
        return -1;
    }

    int start_audio = ftell(fd);

    ma_device_start(&ma_device.actualDevice);
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP)) frequency += 10;
        if (IsKeyDown(KEY_DOWN)) frequency -= 10;
        BeginDrawing();
        ClearBackground(BLACK);

        char driveText[32];
        sprintf(driveText, "frequency: %.1f", frequency);
        DrawText(driveText, 10, 40, 20, WHITE);
        DrawText("speak...", 10, 10, 20, DARKGRAY);

        int centerY = SCREEN_HEIGHT / 2;

        for (int i = 0; i < MAX_SAMPLES - 1; i++) {
            // half screen
            float y1 = centerY + (displayData[i] / 32768.0f) * 200;
            float y2 = centerY + (displayData[i+1] / 32768.0f) * 200;

            // fill the screen
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

    ma_device_uninit(&ma_device.actualDevice);
    fclose(fd);
}
