/* tuner made using zero crossing rate
 * to obtain the voice frequency.
 * Didn't use fft cause its a bit more
 * complex than i thougth
 * */
#include "waveFileHeader.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define MAX_SAMPLES 1048

struct Note {
    char* note;
};

/* ========= global variables ========== */
short displayData[MAX_SAMPLES];
volatile int dataIndex = 0;
float global_frequency = 0.0f;

// similar to bisection medthod, thanks numerical analysis
void analyze_zcr(short* inputSignal, ma_uint32 frameCount) {
    float zeroCrossingRate = 0.0;
    short val = inputSignal[0];
    if (abs(val) < 100) val = 0;
    bool wasPositive = val > 0;
    bool isPositive;

    for (int i = 1; i < frameCount; i++) {
        val = inputSignal[i];
        if (abs(val) <= 100) val = 0;
        isPositive = val > 0;
        if (wasPositive != isPositive) {
            zeroCrossingRate++;
            wasPositive = isPositive;
        }
    }

    float time_in_seconds = (float)frameCount / sample_rate;
    global_frequency = (zeroCrossingRate/2) / time_in_seconds;
}

/* ====== callback ======= */
// function called whenever a chunk of the audio buffer is full and ready to be elaborated
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    FILE* fd = (FILE*)pDevice->pUserData;
    if(fd == NULL) return;

    short* inputSamples = (short*)pInput;
    short denormalized_sample[frameCount];
    analyze_zcr(inputSamples, frameCount);

    for(int i = 0; i < frameCount; i++) {

        // thing for the raylib plot
        if (dataIndex < MAX_SAMPLES) {
            displayData[dataIndex] = inputSamples[i];
            dataIndex++;
        }
        if (dataIndex >= MAX_SAMPLES) {
            dataIndex = 0;
        }
    }

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
    screenInit();

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
