/* tuner made using zero crossing interval
 * to obtain the voice frequency.
 * Didn't use fft cause its a bit more
 * complex than i thougth
 * */
#include "waveFileHeader.h"
#include <stdbool.h>
#include <sys/types.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_SAMPLES 1024
#define ZCR_BUFFER_SIZE 4096

struct Note {
    char* note;
};

/* ========= global variables ========== */
short displayData[MAX_SAMPLES];
volatile int dataIndex = 0;

float global_frequency = 0.0f;
int samples_processed = 0;
unsigned int last_crossing_sample = 0;


void analyze_zci(short* inputSignal, ma_uint32 frameCount) {

    bool is_positive;
    static bool was_positive = false;

    for (int i = 0; i < frameCount; i++) {
        short val = inputSignal[i];

        // using a threshold
        if (val >= 100) {
            is_positive = true;
        } else if (val <= -100) {
            is_positive = false;
        }
        unsigned int sample_number;
        unsigned int period_in_samples;

        // if there's a crossing i start doing things :3
        if (!was_positive && is_positive) {
            // number of the sample being processed at that moment
            sample_number = samples_processed + i;
            if (last_crossing_sample > 0) {
                period_in_samples = sample_number - last_crossing_sample;

                global_frequency = (float)sample_rate / (float)period_in_samples;
            }

            last_crossing_sample = sample_number;
        }
        was_positive = is_positive;
    }
    samples_processed += frameCount;
}

/* ====== callback ======= */
// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;
    if(fd == NULL) return;

    short* inputSamples = (short*)pInput;
    short denormalized_sample[frameCount];

    for(int i = 0; i < frameCount; i++) {

        // thing for the raylib plot
        if (dataIndex < MAX_SAMPLES) {
            displayData[dataIndex] = inputSamples[i];
            dataIndex++;
        } else {
            dataIndex = 0;
        }
    }

    analyze_zci(inputSamples, frameCount);
    // here frameCount frames (aka sample) are ridden from pInput and stored inside the filde descriptor fd
    fwrite(inputSamples, sizeof(short), frameCount, fd);
}

float noteNumberFinder() {
    float n = 12 * log2f(global_frequency/440) + 69;
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

void screenInit() {
    while (!WindowShouldClose()) {

        char noteText[32];
        char frequency_text[32];
        float noteNumber = noteNumberFinder();
        struct Note actualNote = noteRecognizer(noteNumber);

        BeginDrawing();
        ClearBackground(BLACK);

        printf("global freq = %.1f \n", global_frequency);

        sprintf(frequency_text, "frequency = %.1f", global_frequency);
        sprintf(noteText, "note %s", actualNote.note);
        DrawText(frequency_text, 10, 20, 20, RED);
        DrawText(noteText, 10, 70, 20, WHITE);
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
}

int main(int argc, char** argv) {

    FILE *fd = fopen("test.wav", "wb");

    if (fd == NULL) {
        return -1;
    }

    int file_start = ftell(fd);

    wavHeaderFiller(fd);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "C Audio Visualizer");
    SetTargetFPS(64);

    struct device ma_device;

    // device configuration
    deviceSetup(&ma_device, false, data_callback, fd);

    if (ma_device_init(NULL, &ma_device.deviceConfig, &ma_device.actualDevice) != MA_SUCCESS) {
        printf("initialization error");
        return -1;
    }

    int start_audio = ftell(fd);

    ma_device_start(&ma_device.actualDevice);
    screenInit();

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
