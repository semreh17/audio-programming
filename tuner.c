/* lets you watch how the sine wave changes incrementing
 * or decrementing the frequency
 * */
#include "waveFileHeader.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_SAMPLES 1024

/* ========= global variables ========== */
short displayData[MAX_SAMPLES];
volatile int dataIndex = 0; // volatile so the compiler won't optimize it
float phase = 0.0f;
float frequency = 440.0f; // 440 Hz (A4 note)
float amplitude = 0.8f;   // Volume (0.0 a 1.0)

struct Note {
    char* note;
};

/* ====== callback ======= */
// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;
    if(fd == NULL) return;

    short denormalized_sample[frameCount];

    float phase_increment = (2.0f * M_PI * frequency) / sample_rate;

    for(int i = 0; i < frameCount; i++) {
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
}

float frequencyChecker() {
    float n = 12 * log2f(frequency/440) + 69;
    return n;
}


struct Note noteRecognizer(float n) {
    struct Note actualNote;
    int roundedNoteNumber = (int)roundf(n);

    // tells which note inside the octave
    int noteIndex = roundedNoteNumber % 12;
    char* noteList[] = { "C", "C#", "D", "D#", "E", "F",
                       "F#", "G", "G#", "A", "A#", "B"};
    if (noteIndex < 0) {
        noteIndex += 12;
    }
    actualNote.note = noteList[noteIndex];
    return actualNote;
}


void screenInitialize() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C Audio Visualizer");
    SetTargetFPS(64);

    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_UP)) frequency += 5;
        if (IsKeyDown(KEY_DOWN) && frequency > 1) frequency -= 5;

        float noteNumber = frequencyChecker();
        struct Note actualNote = noteRecognizer(noteNumber);


        BeginDrawing();
        ClearBackground(BLACK);

        char driveText[32];
        char noteText[32];
        sprintf(driveText, "frequency: %.1f", frequency);
        sprintf(noteText, "note %s", actualNote.note);

        DrawText(driveText, 10, 40, 20, WHITE);
        DrawText(noteText, 10, 70, 20, RED);

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
}

int main(int argc, char** argv) {
    FILE *fd = fopen("test.wav", "wb");
    if (fd == NULL) {
        return -1;
    }

    int file_start = ftell(fd);
    wavHeaderFiller(fd);

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
    screenInitialize();

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
