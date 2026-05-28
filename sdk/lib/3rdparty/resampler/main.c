#ifdef __cplusplus
extern "C" {
#endif
#define  _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DR_WAV_IMPLEMENTATION

#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION


#include "dr_mp3.h"

#include "timing.h"


void wavWrite_f32(char *filename, float *buffer, int sampleRate, uint32_t totalSampleCount, uint32_t channels) {
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = channels;
    format.sampleRate = (drwav_uint32) sampleRate;
    format.bitsPerSample = 32;
    drwav *pWav = drwav_open_file_write(filename, &format);
    if (pWav) {
        drwav_uint64 samplesWritten = drwav_write(pWav, totalSampleCount, buffer);
        drwav_uninit(pWav);
        if (samplesWritten != totalSampleCount) {
            fprintf(stderr, "write file [%s] error.\n", filename);
            exit(1);
        }
    }
}

float *wavRead_f32(const char *filename, uint32_t *sampleRate, uint64_t *sampleCount, uint32_t *channels) {
    drwav_uint64 totalSampleCount = 0;
    float *input = drwav_open_file_and_read_pcm_frames_f32(filename, channels, sampleRate, &totalSampleCount);
    if (input == NULL) {
        drmp3_config pConfig;
        input = drmp3_open_file_and_read_f32(filename, &pConfig, &totalSampleCount);
        if (input != NULL) {
            *channels = pConfig.outputChannels;
            *sampleRate = pConfig.outputSampleRate;
        }
    }
    if (input == NULL) {
        fprintf(stderr, "read file [%s] error.\n", filename);
        exit(1);
    }
    *sampleCount = totalSampleCount * (*channels);
    return input;
}


void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    } else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.') {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/') {
            p++;
            break;
        }
    if (name) {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir) {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}


uint64_t Resample_f32(const float *input, float *output, int inSampleRate, int outSampleRate, uint64_t inputSize,
                      uint32_t channels) {
    if (input == NULL)
        return 0;

    uint64_t outputSize = (uint64_t) (inputSize * (double) outSampleRate / (double) inSampleRate);
    outputSize -= outputSize % channels;

    if (output == NULL)
        return outputSize;

    double stepDist = ((double) inSampleRate / (double) outSampleRate);
    const uint64_t fixedFraction = (1LL << 32);
    const double normFixed = (1.0 / (1LL << 32));
    uint64_t step = ((uint64_t) (stepDist * fixedFraction + 0.5));
    uint64_t curOffset = 0;

    for (uint32_t i = 0; i < outputSize - channels; i += channels) {
        uint64_t currentPos = curOffset >> 32;
        uint64_t nextPos = currentPos + 1;
        for (uint32_t c = 0; c < channels; c++) {
            float current = input[currentPos * channels + c];
            float next = input[nextPos * channels + c];
            double frac = (curOffset & (fixedFraction - 1)) * normFixed;
            *output++ = (float) (current + (next - current) * frac);
        }
        curOffset += step;
        uint64_t frameSkip = (curOffset >> 32);
        curOffset &= (fixedFraction - 1);
        input += frameSkip * channels;
    }
    uint64_t lastPos = (curOffset >> 32);
    for (uint32_t c = 0; c < channels; c++) {
        *output++ = input[lastPos * channels + c];
    }
    return outputSize;
}

uint64_t Resample_s16(const int16_t *input, int16_t *output, int inSampleRate, int outSampleRate, uint64_t inputSize,
                      uint32_t channels) {
    if (input == NULL)
        return 0;

    uint64_t outputSize = (uint64_t) (inputSize * (double) outSampleRate / (double) inSampleRate);
    outputSize -= outputSize % channels;

    if (output == NULL)
        return outputSize;

    double stepDist = ((double) inSampleRate / (double) outSampleRate);
    const uint64_t fixedFraction = (1LL << 32);
    const double normFixed = (1.0 / (1LL << 32));
    uint64_t step = ((uint64_t) (stepDist * fixedFraction + 0.5));
    uint64_t curOffset = 0;

    for (uint32_t i = 0; i < outputSize - channels; i += channels) {
        uint64_t currentPos = curOffset >> 32;
        uint64_t nextPos = currentPos + 1;
        for (uint32_t c = 0; c < channels; c++) {
            int16_t current = input[currentPos * channels + c];
            int16_t next = input[nextPos * channels + c];
            double frac = (curOffset & (fixedFraction - 1)) * normFixed;
            *output++ = (int16_t) (current + (next - current) * frac);
        }
        curOffset += step;
        uint64_t frameSkip = (curOffset >> 32);
        curOffset &= (fixedFraction - 1);
        input += frameSkip * channels;
    }
    uint64_t lastPos = (curOffset >> 32);
    for (uint32_t c = 0; c < channels; c++) {
        *output++ = input[lastPos * channels + c];
    }
    return outputSize;
}

void printUsage() {
    printf("usage:\n");
    printf("./Resampler input.wav 48000\n");
    printf("./Resampler input.mp3 16000\n");
    printf("or\n");
    printf("./Resampler input.wav output.wav 8000\n");
    printf("./Resampler input.mp3 output.wav 44100\n");
    printf("press any key to exit.\n");
    getchar();
}

void resampler(char *in_file, char *out_file, uint32_t targetSampleRate) {
    if (targetSampleRate == 0) {
        printUsage();
        return;
    }
    uint32_t sampleRate = 0;
    uint64_t sampleCount = 0;
    uint32_t channels = 0;
    float *input = wavRead_f32(in_file, &sampleRate, &sampleCount, &channels);
    uint64_t targetSampleCount = Resample_f32(input, NULL, sampleRate, targetSampleRate, sampleCount, channels);
    if (input) {
        float *output = (float *) malloc(targetSampleCount * sizeof(float));
        if (output) {
            double startTime = now();
            Resample_f32(input, output, sampleRate, targetSampleRate, sampleCount / channels, channels);
            double time_interval = calcElapsed(startTime, now());
            printf("time interval: %f ms\n ", (time_interval * 1000));
            wavWrite_f32(out_file, output, targetSampleRate, (uint32_t) targetSampleCount, channels);
            free(output);
        }
        free(input);
    }
}


int main(int argc, char *argv[]) {
    printf("Audio Processing\n");
    printf("blog:http://cpuimage.cnblogs.com/\n");
    printf("Audio Resampler\n");
    if (argc < 3) {
        printUsage();
        return -1;
    }
    char *in_file = argv[1];
    if (argc > 3) {
        char *out_file = argv[2];
        uint32_t targetSampleRate = (uint32_t) atoi(argv[3]);
        resampler(in_file, out_file, targetSampleRate);
    } else {
        uint32_t targetSampleRate = (uint32_t) atoi(argv[2]);
        char drive[3];
        char dir[256];
        char fname[256];
        char ext[256];
        char out_file[1024];
        splitpath(in_file, drive, dir, fname, ext);
        sprintf(out_file, "%s%s%s_out.wav", drive, dir, fname);
        resampler(in_file, out_file, targetSampleRate);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
