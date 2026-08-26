#ifndef __LINEAR_SMOOTHING_H_INCLUDED__
#define __LINEAR_SMOOTHING_H_INCLUDED__

#include "global.h"

/**
 * Basically aping JUCE's excellent juce::SmoothedValue<T>
 */
class LinearSmoothing {
    public:
        LinearSmoothing() = default;

        void reset( float sampleRate, float smoothingTimeInSeconds ) {
            currentSampleRate = sampleRate;
            smoothingTime = smoothingTimeInSeconds;
            resetSteps();
        }

        // sets target value instantly
        void setCurrentAndTargetValue( float value ) {
            currentValue = value;
            targetValue = value;
            stepsRemaining = 0;
            stepSize = 0.0f;
        }

        // sets target value to glide to
        void setTargetValue( float value ) {
            if ( value == targetValue ) {
                return;
            }
            targetValue = value;
            resetSteps();
        }

        // gets the next value (for an individual sample in a buffer)
        float getNextValue() {
            if ( stepsRemaining > 0 ) {
                currentValue += stepSize;
                --stepsRemaining;
                
                if ( stepsRemaining == 0 ) {
                    currentValue = targetValue;
                }
            }
            return currentValue;
        }

        // fast-forward the values by provided amonut of samples
        void skip( int numSamples ) {
            if ( stepsRemaining > 0 ) {
                if ( numSamples >= stepsRemaining ) {
                    currentValue = targetValue;
                    stepsRemaining = 0;
                    stepSize = 0.0f;
                } else {
                    currentValue += stepSize * numSamples;
                    stepsRemaining -= numSamples;
                }
            }
        }

    private:
        float currentSampleRate = Igorski::VST::DEFAULT_SAMPLE_RATE;
        float smoothingTime = 0.02f; // 20 milliseconds default
        
        float currentValue = 0.0f;
        float targetValue = 0.0f;
        float stepSize = 0.0f;
        int stepsRemaining = 0;

        void resetSteps() {
            if ( currentSampleRate <= 0.0f || smoothingTime <= 0.0f ) {
                currentValue = targetValue;
                stepsRemaining = 0;
                stepSize = 0.0f;
                return;
            }

            stepsRemaining = static_cast<int>( currentSampleRate * smoothingTime );

            if ( stepsRemaining > 0 ) {
                stepSize = ( targetValue - currentValue ) / static_cast<float>( stepsRemaining );
            } else {
                currentValue = targetValue;
                stepSize = 0.0f;
            }
        }
};

#endif