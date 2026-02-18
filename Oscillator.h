#pragma once

#include "ofMain.h"

class Oscillator
{
public:

    enum Waveform
    {
        SINE,
        SQUARE,
        SAW,
        TRIANGLE
    };

    // Constructor with sample rate
    explicit Oscillator(float sr);

    // Setters
    void setFrequency(float freq);
    void setWaveform(Waveform type);
    void setBrightness(int bright);

    // Get next sample
    float getNextSample();

private:
    float sampleRate;
    float frequency;
    float phase;
    float phaseIncrement;
    Waveform waveform;
    int brightness;

    void updatePhaseIncrement();
};
