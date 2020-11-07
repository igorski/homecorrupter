/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Igor Zinken - https://www.igorski.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PluginProcess__H_INCLUDED__
#define __PluginProcess__H_INCLUDED__

#include "global.h"
#include "audiobuffer.h"
#include "bitcrusher.h"
#include "limiter.h"
#include "lowpassfilter.h"
#include <vector>

using namespace Steinberg;

namespace Igorski {
class PluginProcess
{
    // dithering constants

    const float DITHER_WORD_LENGTH = pow( 2.0, 15 );        // 15 implies 16-bit depth
    const float DITHER_WI          = 1.0f / DITHER_WORD_LENGTH;
    const float DITHER_DC_OFFSET   = DITHER_WI * 0.5f;      // apply in resampling routine to remove DC offset
    const float DITHER_AMPLITUDE   = DITHER_WI / RAND_MAX;  // 2 LSB

    public:
        static constexpr float MIN_PLAYBACK_SPEED = .5f;
        static constexpr float MIN_SAMPLE_RATE    = 1000.f;

        PluginProcess( int amountOfChannels );
        ~PluginProcess();

        // apply effect to incoming sampleBuffer contents

        template <typename SampleType>
        void process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
            int bufferSize, uint32 sampleFramesSize
        );

        void setResampleRate( float value );
        void setResampleLfo( float LFORatePercentage, float LFODepth );
        void setPlaybackRate( float value );
        void setPlaybackRateLfo( float LFORatePercentage, float LFODepth );
        void setDryMix( float value );
        void setWetMix( float value );
        void resetReadWritePointers(); // invoke on host sequencer start

        BitCrusher* bitCrusher;
        Limiter*    limiter;

    private:
        AudioBuffer* _recordBuffer; // buffer used to record incoming signal
        AudioBuffer* _preMixBuffer; // buffer used for the pre-effect mixing

        float _dryMix;
        float _wetMix;
        int _amountOfChannels;
        std::vector<LowPassFilter*> _lowPassFilters;

        // read/write pointers for the record buffer used for record and playback

        float _readPointer;
        int _writePointer;
        int _maxRecordBufferSize;

        // down sampling

        float  _downSampleAmount;
        float  _tempDownSampleAmount;
        float  _maxDownSample;
        float* _lastSamples; // last written sample, per channel

        // clock speed

        float _playbackRate;
        float _tempPlaybackRate;
        int   _sampleIncr;

        // oscillators

        LFO* _downSampleLfo;
        LFO* _playbackRateLfo;

        bool  _hasDownSampleLfo;
        float _downSampleLfoDepth;
        float _downSampleLfoRange;
        float _downSampleLfoMax;
        float _downSampleLfoMin;

        bool  _hasPlaybackRateLfo;
        float _playbackRateLfoDepth;
        float _playbackRateLfoRange;
        float _playbackRateLfoMax;
        float _playbackRateLfoMin;

        // caching of values

        void cacheValues();
        void cacheLfo();
        void cacheMaxDownSample();

        // ensures the pre- and post mix buffers match the appropriate amount of channels
        // and buffer size. this also clones the contents of given in buffer into the pre-mix buffer
        // the buffers are pooled so this can be called upon each process cycle without allocation overhead

        template <typename SampleType>
        void prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize );
};
}

#include "plugin_process.tcc"

#endif
