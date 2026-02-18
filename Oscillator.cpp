#include "Oscillator.h"
#include <glm/glm.hpp>   // for glm::two_pi
#include <algorithm>
#include <cmath>

// Constructor with sample rate
Oscillator::Oscillator(float sr)
{
    sampleRate = sr;
    frequency = 440.0f;
    phase = 0.0f;
    waveform = SINE;
    brightness = 10;
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

void Oscillator::setBrightness(int bright)
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
            int maxHarmonic = static_cast<int>(sampleRate / (2 * frequency));
            int effectiveBrightness = std::min(brightness, maxHarmonic);

            float scarre = 0.0f;
            for (int k = 0; k < effectiveBrightness; k++)
                scarre += sin(phase * (2*k + 1)) / (2*k + 1);

            return 4.0f / glm::pi<float>() * scarre;
        }

        case SAW:
        {
            int maxHarmonic = static_cast<int>(sampleRate / (2 * frequency));
            int effectiveBrightness = std::min(brightness, maxHarmonic);

            if (effectiveBrightness == 0)
                return sin(phase);

            float sscie = 0.0f;
            for (int k = 1; k <= effectiveBrightness; k++)
            {
                float sign = (k % 2 == 0) ? -1.0f : 1.0f;
                sscie += sign * sin(phase * k) / k;
            }

            return 2.0f / glm::pi<float>() * sscie;
        }

        case TRIANGLE:
        {
            int maxHarmonic = static_cast<int>(sampleRate / (2 * frequency));
            int effectiveBrightness = std::min(brightness, maxHarmonic);

            float striangle = 0.0f;
            for (int k = 0; k < effectiveBrightness; k++)
            {
                int harmonic = 2*k + 1;
                float sign = (k % 2 == 0) ? 1.0f : -1.0f;
                striangle += sign * sin(phase * harmonic) / (harmonic * harmonic);
            }

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
