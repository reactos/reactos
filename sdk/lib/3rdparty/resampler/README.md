# resampler
A Simple and Efficient Audio Resampler Implementation in C.

Example
=======

```C

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
```




# Donating

If you found this project useful, consider buying me a coffee

<a href="https://www.buymeacoffee.com/gaozhihan" target="_blank"><img src="https://img2018.cnblogs.com/blog/824862/201809/824862-20180930223603138-1708589189.png" alt="Buy Me A Coffee" style="height: auto !important;width: auto !important;" ></a>
