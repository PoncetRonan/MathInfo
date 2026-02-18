#pragma once

#include "ofMain.h"
#include "def.h"

class Oscillator
{
public:

    Oscillator();

    void setup(float sr);

    // Setters
    void setFrequency(float freq);
    void setWaveform(Waveform type);
    void setBrightness(int bright);

    // Get next sample
    float getNextSample();
    int brightness;
    Waveform waveform;
    int octave;

private:
    float sampleRate;
    float frequency;
    float phase;
    float phaseIncrement;



    void updatePhaseIncrement();
};
