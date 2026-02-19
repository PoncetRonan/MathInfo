#include "Oscillator.h"
#include <glm/glm.hpp>   // for glm::two_pi
#include <algorithm>
#include <cmath>


Oscillator::Oscillator()
{
}

// Constructor with sample rate
void Oscillator::setup(float sr)
{
    sampleRate = sr;
    frequency = 440.0f;
    phase = 0.0f;
    waveform = SINE;
    brightness = 10;
    octave=1;
    updatePhaseIncrement();
}

void Oscillator::setFrequency(float freq)
{
    frequency = freq;
    updatePhaseIncrement();
}

void Oscillator::setWaveform(Waveform type)
{
    waveform = type;
}

void Oscillator::setBrightness(float bright)
{
    brightness = bright;
}

float Oscillator::getNextSample()
{
    phase += phaseIncrement;

    if (phase >= glm::two_pi<float>())
        phase -= glm::two_pi<float>();

    switch (waveform)
    {
        case SINE:
            return sin(phase);

        case SQUARE:
        {
            float maxHarmonic = static_cast<float>(sampleRate / (2 * frequency));
            float effectiveBrightness = std::min(brightness, maxHarmonic);
            int intBrightness=static_cast<int>(effectiveBrightness);
            float floatBrightness = effectiveBrightness - intBrightness;

            float scarre = 0.0f;
            for (int k = 0; k < intBrightness + 1; k++){
                if(k == intBrightness){
                    scarre += floatBrightness * (sin(phase * (2*k + 1)) / (2*k + 1));
                }
                else{
                scarre += sin(phase * (2*k + 1)) / (2*k + 1);
            }}
            return 4.0f / glm::pi<float>() * scarre;
        }

        case SAW:
        {
            float maxHarmonic = static_cast<float>(sampleRate / (2 * frequency));
            float effectiveBrightness = std::min(brightness, maxHarmonic);
            int intBrightness=static_cast<int>(effectiveBrightness);
            float floatBrightness = effectiveBrightness - intBrightness;

            if (effectiveBrightness == 0)
                return sin(phase);

            float sscie = 0.0f;
            for (int k = 1; k <= intBrightness + 1; k++)
            {
                if(k == intBrightness){
                float sign = (k % 2 == 0) ? -1.0f : 1.0f;
                sscie += floatBrightness *(sign * sin(phase * k) / k);
                }
                else{
                float sign = (k % 2 == 0) ? -1.0f : 1.0f;
                sscie += sign * sin(phase * k) / k;
            }}

            return 2.0f / glm::pi<float>() * sscie;
        }

        case TRIANGLE:
        {
            float maxHarmonic = static_cast<float>(sampleRate / (2 * frequency));
            float effectiveBrightness = std::min(brightness, maxHarmonic);
            int intBrightness=static_cast<int>(effectiveBrightness);
            float floatBrightness = effectiveBrightness - intBrightness;
            float striangle = 0.0f;
            for (int k = 0; k < intBrightness +1; k++)

            {
                int harmonic = 2*k + 1;
                float sign = (k % 2 == 0) ? 1.0f : -1.0f;
                if(k == intBrightness){
                striangle += floatBrightness*(sign * sin(phase * harmonic) / (harmonic * harmonic));

                }
                else{
                striangle += sign * sin(phase * harmonic) / (harmonic * harmonic);
            }}

            return 8.0f / (glm::pi<float>() * glm::pi<float>()) * striangle;
        }

        default:
            return 0.0f;
    }
}

void Oscillator::updatePhaseIncrement()
{
    phaseIncrement = glm::two_pi<float>() * frequency / sampleRate;
}
