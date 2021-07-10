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
#include <algorithm>
#include "calc.h"

namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    if ( bufferSize == 0 ) {
        return; // validator Variable Block Size test
    }

    // input and output buffers can be float or double as defined
    // by the templates SampleType value. Internally we process
    // audio as floats

    SampleType inSample;
    int32 i, l;

    bool mixDry = _dryMix != 0.f;

    SampleType dryMix = ( SampleType ) _dryMix;
    SampleType wetMix = ( SampleType ) _wetMix;

    prepareMixBuffers( numInChannels, bufferSize );

    float readPointer;
    int writePointer;
    int recordMax = _maxRecordBufferSize - 1; // never record beyond the record buffer size (duh...)

    int t, t2;
    float incr;
    float frac, s1, s2;

    int maxBufferPos  = bufferSize - 1;
    int maxReadOffset = _writePointer + maxBufferPos; // never read beyond the range of the current incoming input

    float curSample, nextSample, lastSample, outSample;

    // cache oscillator positions (are reset for each channel where the last iteration is saved)

    float downSampleLfoAcc   = _downSampleLfo->getAccumulator();
    float playbackRateLfoAcc = _playbackRateLfo->getAccumulator();
    float lfoValue;

    // temp variables for dithering

    int r1 = 0;
    int r2 = 0;

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        readPointer  = _readPointer;
        writePointer = _writePointer;

        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

        LowPassFilter* lowPassFilter = _lowPassFilters.at( c );

        _downSampleLfo->setAccumulator( downSampleLfoAcc );
        _playbackRateLfo->setAccumulator( playbackRateLfoAcc );

        lastSample = _lastSamples[ c ];

        // write input into the record buffer (converting to float when necessary)

        for ( i = 0; i < bufferSize; ++i, ++writePointer ) {
            if ( writePointer > recordMax ) {
                writePointer = 0;
            }
            channelRecordBuffer[ writePointer ] = ( float ) channelInBuffer[ i ];
        }

        // write current read range into the premix buffer, downsampling as necessary

        i = 0;

        while ( i < bufferSize ) {
            t  = ( int ) readPointer;
            t2 = std::min( recordMax, t + _sampleIncr );

            // this fractional is in the 0 - 1 range
            // NOTE: we have commented this calculation
            // as the result is devilishly tasty when down sampling

            frac = /*readPointer - t :*/ 0.f;

            s1 = channelRecordBuffer[ t ];
            s2 = channelRecordBuffer[ t2 ];

            // we apply a lowpass filter to prevent interpolation artefacts

            curSample = lowPassFilter->applySingle( s1 + ( s2 - s1 ) * frac );
            outSample = curSample * .5f;

            int start = i;
            for ( l = std::min( bufferSize, start + _sampleIncr ); i < l; ++i ) {
                r2 = r1;
                r1 = rand();

                nextSample = outSample + lastSample;
                lastSample = nextSample * .25f;

                // write sample into the output buffer, corrected for DC offset and dithering applied

                channelPreMixBuffer[ i ] = nextSample + DITHER_DC_OFFSET + DITHER_AMPLITUDE * ( r1 - r2 );

                // run the oscillators, note we multiply by .5 and add .5 to make the LFO's bipolar waveforms unipolar

                if ( _hasDownSampleLfo ) {
                    lfoValue = _downSampleLfo->peek() * .5f + .5f;
                    setActualDownSampling( std::min( _downSampleLfoMax, _downSampleLfoMin + _downSampleLfoRange * lfoValue ) * _maxDownSample );
                    l = std::min( bufferSize, start + _sampleIncr );
                }

                if ( _hasPlaybackRateLfo ) {
                    lfoValue = _playbackRateLfo->peek() * .5f + .5f;
                    setActualPlaybackRate( std::min( _playbackRateLfoMax, _playbackRateLfoMin + _playbackRateLfoRange * lfoValue ));
                }
            }

            // note we cannot cache the increment value as its parts are altered by the oscillators in the render cycle above
            incr = _fSampleIncr * _actualPlaybackRate;

            if (( readPointer += incr ) > maxReadOffset ) {
                readPointer = ( float ) _writePointer; // don't go to 0.f but align with current write offset to play "current audio"
            }
        }

        // apply bit crusher

        bitCrusher->process( channelPreMixBuffer, bufferSize );

        // mix the input and processed mix buffers into the output buffer

        for ( i = 0; i < bufferSize; ++i ) {

            // before writing to the out buffer we take a snapshot of the current in sample
            // value as VST2 in Ableton Live supplies the same buffer for inBuffer and outBuffer!

            inSample = channelInBuffer[ i ];

            // wet mix (e.g. the effected signal)

            channelOutBuffer[ i ] = (( SampleType ) channelPreMixBuffer[ i ] ) * wetMix;

            // dry mix (e.g. mix in the input signal)

            if ( mixDry ) {
                channelOutBuffer[ i ] += ( inSample * dryMix );
            }
        }
        // update channel properties
        _lastSamples[ c ] = lastSample;
    }
    // update read/write indices
    _readPointer  = readPointer;
    _writePointer = writePointer;

    // limit the output signal in case its gets hot (e.g. on heavy bit reduction)
    limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

}
