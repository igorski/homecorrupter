/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2026 Igor Zinken - https://www.igorski.nl
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

namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    if ( bufferSize <= 0 ) {
        return; // Variable Block Size unit test
    }

    // input and output buffers can be float or double as defined
    // by the templates SampleType value. Internally we process
    // audio as floats

    SampleType inSample;
    int32 i, l;

    bool mixDry = _dryMix != 0.f;

    SampleType dryMix = ( SampleType ) _dryMix;
    SampleType wetMix = ( SampleType ) _wetMix;

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    float readPointer;
    int writePointer;
    int recordMax = _maxRecordBufferSize - 1; // never record beyond the record buffer size (duh...)

    int t;
    float incr, frac, s1, s2;

    int maxBufferPos  = bufferSize - 1;
    int maxReadOffset = _writePointer + maxBufferPos; // never read beyond the range of the current incoming input

    float curSample, outSample;

    // cache oscillator positions (are reset for each channel where the last iteration is saved)

    float downSampleLfoAcc   = _downSampleLfo->getAccumulator();
    float playbackRateLfoAcc = _playbackRateLfo->getAccumulator();
    float lfoValue;

    // temp variables for dithering

    float r1 = 0;
    float r2 = 0;
    float dither = 0;

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        readPointer  = _readPointer;
        writePointer = _writePointer;

        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

        auto lowPassFilter = _lowPassFilters.at( c );
        auto hold = _holdStates.at( c );

        int filterPointer = _filterPointers[ c ];
        float filteredPrev = _filteredPrev[ c ];
        float filteredCur = _filteredCur[ c ];

        _downSampleLfo->setAccumulator( downSampleLfoAcc );
        _playbackRateLfo->setAccumulator( playbackRateLfoAcc );

        float lastSample = _lastSamples[ c ];

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

            if ( hold.remaining == 0 ) {
                t  = ( int ) readPointer;
                // if ( t > recordMax ) {
                //    t -= _maxRecordBufferSize;
                // }
                frac = readPointer - t;
                
                // advance the anti alias filter at host rate through every sample we will read
                // up to and including t + 1 so the interpolation uses filtered values

                while ( filterPointer <= t + 1 ) {
                    int idx = filterPointer;
                    if ( idx > recordMax ) {
                        idx -= _maxRecordBufferSize;
                    }
                    filteredPrev = filteredCur;
                    filteredCur = /*lowPassFilter.applySingle( */channelRecordBuffer[ idx ]; //);
                    ++filterPointer;
                }

                // filteredPrev is sample at t, filteredCur is sample at t + 1

                curSample = filteredPrev + ( filteredCur - filteredPrev ) * frac;

                hold.length = _sampleIncr;
                hold.remaining = hold.length;
                hold.sample = curSample * .667f;
                hold.coeff = std::min( 1.f, SMOOTHING / ( float ) hold.length );

                r2 = r1;
                r1 = _randomizer.nextBipolar();
                hold.dither = DITHER_AMPLITUDE * ( r1 - r2 );

                float incr = ( float ) hold.length * _actualPlaybackRate;

                if (( readPointer += incr ) > maxReadOffset ) {
                    readPointer = ( float ) writePointer;
                    filterPointer = ( int ) readPointer;
                }
            }

            for ( l = i + std::min( bufferSize - i, hold.remaining ); i < l; ++i ) {

                lastSample += ( hold.sample - lastSample ) * hold.coeff;
                
                // write sample into the output buffer, corrected for DC offset and dithering applied
                channelPreMixBuffer[ i ] = lastSample + DITHER_DC_OFFSET + hold.dither;

                // catch denormals
                UNDENORMALISE( channelPreMixBuffer[ i ]);

                --hold.remaining;

                // run the oscillators, note we multiply by .5 and add .5 to make the LFO's bipolar waveforms unipolar

                if ( _hasDownSampleLfo ) {
                    lfoValue = _downSampleLfo->peek() * .5f + .5f;
                    setActualDownSampling( std::min( _downSampleLfoMax, _downSampleLfoMin + _downSampleLfoRange * lfoValue ) * _maxDownSample );
                    // @todo do we need to reset l here (as before) ?
                }

                if ( _hasPlaybackRateLfo ) {
                    lfoValue = _playbackRateLfo->peek() * .5f + .5f;
                    setActualPlaybackRate( std::min( _playbackRateLfoMax, _playbackRateLfoMin + _playbackRateLfoRange * lfoValue ));
                }
            }
        }

        // apply bit crusher (when active)

        if ( bitCrusher->isActive() ) {
            for ( i = 0; i < bufferSize; ++i ) {
                scratchBuffer[ i ] = channelPreMixBuffer[ i ];
            }
            bitCrusher->process( channelPreMixBuffer, bufferSize );

            // apply make-up gain to keep volume balanced
            _makeUpGainProcessors.at( c ).apply( scratchBuffer, channelPreMixBuffer, bufferSize );
        }

        // mix the input and processed mix buffers into the output buffer

        for ( i = 0; i < bufferSize; ++i ) {

            // before writing to the out buffer we take a snapshot of the current in sample
            // value as VST2 in Ableton Live supplies the same buffer for inBuffer and outBuffer!

            inSample = channelInBuffer[ i ];

            // wet mix (e.g. the effected signal)

            channelOutBuffer[ i ] = Calc::capSample(( SampleType ) channelPreMixBuffer[ i ] * wetMix );

            // dry mix (e.g. mix in the input signal)

            if ( mixDry ) {
                channelOutBuffer[ i ] += ( inSample * dryMix );
            }
        }
        // update channel properties
        _lastSamples[ c ] = lastSample;
        _filterPointers[ c ] = filterPointer;
        _filteredPrev[ c ] = filteredPrev;
        _filteredCur[ c ] = filteredCur;
    }
    // update read/write indices
    _readPointer  = readPointer;
    _writePointer = writePointer;

    // limit the output signal in case its gets hot (e.g. on heavy bit reduction)
    limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // variable block size for a smaller block should not require new record buffers
    // only create these when the last size was smaller than the current
    if ( bufferSize <= _lastBufferSize ) {
        return;
    }

    _lastBufferSize = bufferSize;

    // if the record buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    int idealRecordSize = Calc::secondsToBuffer( MAX_RECORD_SECONDS, _hostSampleRate );
    int recordSize      = idealRecordSize + idealRecordSize % bufferSize;

    if ( _recordBuffer == nullptr || _recordBuffer->bufferSize != recordSize ) {
        delete _recordBuffer;
        _recordBuffer = new AudioBuffer( numInChannels, recordSize );
        _maxRecordBufferSize = recordSize;
    }

    // if the pre mix buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    if ( _preMixBuffer == nullptr || _preMixBuffer->bufferSize != bufferSize ) {
        delete _preMixBuffer;
        _preMixBuffer = new AudioBuffer( numInChannels, bufferSize );
    }
}

}
