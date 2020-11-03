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

namespace Igorski
{
template <typename SampleType>
void PluginProcess::process( SampleType** inBuffer, SampleType** outBuffer, int numInChannels, int numOutChannels,
                             int bufferSize, uint32 sampleFramesSize ) {

    // input and output buffers can be float or double as defined
    // by the templates SampleType value. Internally we process
    // audio as floats

    SampleType inSample;
    int i, l;

    bool mixDry = _dryMix != 0.f;

    SampleType dryMix = ( SampleType ) _dryMix;
    SampleType wetMix = ( SampleType ) _wetMix;

    prepareMixBuffers( inBuffer, numInChannels, bufferSize );

    float readPointer; // TODO: when float pitch shifting works on non/low down sampling settings... when int, has less breakup on shifting down sampling
    int writePointer;
    int recordMax = _maxRecordBufferSize - 1;

    int t, t2;
    int maxT = bufferSize - 1;
    float incr, frac, fracNext, s1, s2;

    // cache oscillator positions (are reset for each channel)

    float downSampleLfoAcc   = _downSampleLfo->getAccumulator();
    float playbackRateLfoAcc = _playbackRateLfo->getAccumulator();

    // dither stuff

    int   r1 = 0, r2 = 0;                //rectangular-PDF random numbers
    float w  = pow(2.0,/*bits-1*/16-1);   //word length (usually bits=16)
    float wi = 1.0f/w;
    float o  = wi * 0.5f;         //remove dc offset
    float d  = wi / RAND_MAX;     //dither amplitude (2 lsb)

    for ( int32 c = 0; c < numInChannels; ++c )
    {
        readPointer  = _readPointer;
        writePointer = _writePointer;

        SampleType* channelInBuffer  = inBuffer[ c ];
        SampleType* channelOutBuffer = outBuffer[ c ];
        float* channelRecordBuffer   = _recordBuffer->getBufferForChannel( c );
        float* channelPreMixBuffer   = _preMixBuffer->getBufferForChannel( c );

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

        // write current read range downsampled into the premix buffer

        i = 0;

        while ( i < bufferSize ) {
            t  = ( int ) readPointer;
            t2 = std::min( maxT, t + 1 );

            // use floating point resolution when sample rate reduction is minimal/non-existent
            // and when lower playback rate is defined (crisper results, with more severe
            // downsampling the artefacts are quite tasty)
            // TODO : if the dithering issue is solved, this nonsense can go as you can see useFloat = false
            // (BUT SHOULD PROBABLY BE TRUE FOR SMOOTH RESULTS??)
            bool useFloat = false;// _tempDownSampleAmount < .1 && _tempPlaybackRate < 1.f;

            // these fractionals are in 0 - 1 range
            frac     = useFloat ? readPointer - t : 0;
            fracNext = useFloat ? ( readPointer + 1 ) : t + 1 - t2;

            s1 = channelRecordBuffer[ t ];
            s2 = channelRecordBuffer[ t2 ];

            float inSample   = ( s1 + ( s2 - s1 ) * frac );
            float outSample  = inSample * .5;
            // used to be the below, no lastSample usage and channelPremixBuffer[ i ] was immediately assigned outSample
            //float nextSample = ( s2 + ( s3 - s2 ) * fracNext ); // references the next sample for anti aliasing purposes (needs t3 and s3)
            //float outSample  = ( inSample * .75 ) + ( nextSample * .25 ); // TODO: smoothing (filter_state ?)

            int start = i;
            for ( l = std::min( bufferSize, start + _sampleIncr ); i < l; ++i ) {

                r2 = r1;
                r1 = rand();

                float nextSample = outSample + lastSample;
                channelPreMixBuffer[ i ] = nextSample + o + d * ( float )( r1 - r2 ); // correct DC offset and apply dither
                lastSample = nextSample * .25;

                // update the increment in case the LFO's have updated the down sampling amount or playback rate
                incr = ( float ) _sampleIncr * _tempPlaybackRate;

                // run the oscillators, note we multiply by .5 and add .5 to make the LFO's bipolar waveforms unipolar

                if ( _hasDownSampleLfo ) {
                    // TODO: doesn't do much?
                    float lfoValue = _downSampleLfo->peek() * .5f + .5f;
                    _tempDownSampleAmount = std::min( _downSampleLfoMax, _downSampleLfoMin + _downSampleLfoRange * lfoValue );
                    cacheValues();
                    l = std::min( bufferSize, start + _sampleIncr );
                }

                if ( _hasPlaybackRateLfo ) {
                    float lfoValue = _playbackRateLfo->peek() * .5f + .5f;
                    _tempPlaybackRate = std::min( _playbackRateLfoMax, _playbackRateLfoMin + _playbackRateLfoRange * lfoValue );
                    cacheValues();
                }
            }

            if (( readPointer += incr ) > recordMax ) {
                readPointer = 0.f;
            }
        }

        // apply bit crusher
        bitCrusher->process( channelPreMixBuffer, bufferSize );

        // mix the input and processed mix buffers into the output buffer

        for ( i = 0; i < bufferSize; ++i ) {
            // wet mix (e.g. the effected signal)
            channelOutBuffer[ i ] = ( SampleType ) channelPreMixBuffer[ i ] * wetMix;
            // dry mix (e.g. mix in the input signal)
            if ( mixDry ) {
                // before writing to the out buffer we take a snapshot of the current in sample
                // value as VST2 in Ableton Live supplies the same buffer for in and out!
                inSample = channelInBuffer[ i ];
                channelOutBuffer[ i ] += ( inSample * dryMix );
            }
        }

        _lastSamples[ c ] = lastSample;
    }
    // update indices
    _readPointer  = readPointer;
    _writePointer = writePointer;

    // limit the output signal in case its gets hot (e.g. on heavy bit reduction)
    limiter->process<SampleType>( outBuffer, bufferSize, numOutChannels );
}

template <typename SampleType>
void PluginProcess::prepareMixBuffers( SampleType** inBuffer, int numInChannels, int bufferSize )
{
    // if the record buffer wasn't created yet or the buffer size has changed
    // delete existing buffer and create new one to match properties

    int recordSize = bufferSize * ( int ) ( MAX_DOWNSAMPLE / MIN_PLAYBACK_SPEED );
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
