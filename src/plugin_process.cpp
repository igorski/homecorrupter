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
#include "plugin_process.h"
#include "calc.h"
#include <math.h>
#include <algorithm>

namespace Igorski {

PluginProcess::PluginProcess( int amountOfChannels )
{
    _amountOfChannels = amountOfChannels;
    cacheMaxDownSample();

     _lastSamples = new float[ amountOfChannels ];

    for ( int i = 0; i < amountOfChannels; ++i ) {
        _lastSamples[ i ] = 0.f;
        _lowPassFilters.push_back( new LowPassFilter());
    }

    _dryMix = 0.f;
    _wetMix = 1.f;

    // create the child processors

    bitCrusher = new BitCrusher( 1.f, .5f, 1.f );
    limiter    = new Limiter( 10.f, 500.f, .6f );

    // buffers will be lazily created in the process function as they correspond to the host buffer size
    _recordBuffer  = nullptr;
    _preMixBuffer  = nullptr;

    // oscillators
    _downSampleLfo      = new LFO();
    _hasDownSampleLfo   = false;
    _playbackRateLfo    = new LFO();
    _hasPlaybackRateLfo = false;

    // read / write variables

    _readPointer  = 0.f;
    _writePointer = 0;

    _downSampleAmount       = 0.f;
    _actualDownSampleAmount = 1.f;
    _playbackRate           = 0.f;
    _actualPlaybackRate     = 1.f;

    setResampleRate( _actualDownSampleAmount);
    setPlaybackRate( _actualPlaybackRate );
}

PluginProcess::~PluginProcess()
{
    delete[] _lastSamples;

    while ( _lowPassFilters.size() > 0 ) {
        delete _lowPassFilters.at( 0 );
        _lowPassFilters.erase( _lowPassFilters.begin() );
    }

    delete bitCrusher;
    delete limiter;
    delete _recordBuffer;
    delete _preMixBuffer;
    delete _downSampleLfo;
    delete _playbackRateLfo;
}

/* setters */

void PluginProcess::setDryMix( float value )
{
    _dryMix = value;
}

void PluginProcess::setWetMix( float value )
{
    _wetMix = value;
}

void PluginProcess::setResampleRate( float value )
{
    // invert the sampling rate value to determine the down sampling value
    float downSampleValue = abs( value - 1.f );
    float scaledAmount    = Calc::scale( downSampleValue, 1.f, _maxDownSample - 1.f ) + 1.f;

    if ( scaledAmount == _downSampleAmount ) {
        return; // don't trigger changes if value is the same
    }
//    else if ( _recordBuffer != nullptr ) {
//        float ratio  = scaledAmount / _downSampleAmount;
//        _readPointer = std::max( 0.f, std::min(( float ) _recordBuffer->bufferSize - 1.f, ( float ) _writePointer * ratio ));
//    }

    float tempRatio = _actualDownSampleAmount / std::max( 0.000000001f, _downSampleAmount );

    _downSampleAmount = scaledAmount;

    // in case down sampling is attached to oscillator, keep relative offset of currently moving wave in place
    setActualDownSampling( _hasDownSampleLfo ? _downSampleAmount * tempRatio : _downSampleAmount );
    cacheLfo();
}

void PluginProcess::setResampleLfo( float LFORatePercentage, float LFODepth )
{
    bool wasEnabled = _hasDownSampleLfo;
    bool enabled    = LFORatePercentage > 0.f;

    _hasDownSampleLfo = enabled;

    bool hadChange = ( wasEnabled != enabled ) || _downSampleLfoDepth != LFODepth;

    if ( enabled )
        _downSampleLfo->setRate(
            VST::MIN_LFO_RATE() + (
                LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
            )
        );

    // turning LFO off
    if ( !_hasDownSampleLfo && wasEnabled ) {
        _actualDownSampleAmount = _downSampleAmount;
        cacheDownSamplingValues();
    }

    if ( hadChange ) {
        _downSampleLfoDepth = LFODepth;
        cacheLfo();
    }
}

void PluginProcess::setPlaybackRate( float value )
{
    float tempRatio = _actualPlaybackRate / std::max( 0.000000001f, _playbackRate );

    // rate is in 0 - 1 range, playback rate speed support is between 0.5 (half speed) - 1.0f (full speed)
    float scaledAmount = Calc::scale( value, 1, MIN_PLAYBACK_SPEED ) + MIN_PLAYBACK_SPEED;

    if ( scaledAmount == _playbackRate ) {
        return; // don't trigger changes if value is the same
    }

    _playbackRate = scaledAmount;

    // in case playback rate is attached to oscillator, keep relative offset of currently moving wave in place
    setActualPlaybackRate( _hasPlaybackRateLfo ? _playbackRate * tempRatio : _playbackRate );

    cacheLfo();
}

void PluginProcess::setPlaybackRateLfo( float LFORatePercentage, float LFODepth )
{
    bool wasEnabled = _hasPlaybackRateLfo;
    bool enabled    = LFORatePercentage > 0.f;

    _hasPlaybackRateLfo = enabled;

    bool hadChange = ( wasEnabled != enabled ) || _playbackRateLfoDepth != LFODepth;

    if ( enabled )
        _playbackRateLfo->setRate(
            VST::MIN_LFO_RATE() + (
                LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
            )
        );

    // turning LFO off
    if ( !_hasPlaybackRateLfo && wasEnabled ) {
        _actualPlaybackRate = _playbackRate;
    }

    if ( hadChange ) {
        _playbackRateLfoDepth = LFODepth;
        cacheLfo();
    }
}

void PluginProcess::resetReadWritePointers()
{
    _readPointer  = 0.f;
    _writePointer = 0;
}

void PluginProcess::clearBuffer()
{
    if ( _recordBuffer != nullptr ) {
        _recordBuffer->silenceBuffers();
    }
}

/* private methods */

void PluginProcess::cacheDownSamplingValues()
{
    _fSampleIncr = std::max( 1.f, floor( _actualDownSampleAmount ));
    _sampleIncr  = ( int ) _fSampleIncr;

    // update the lowpass filters to the appropriate cutoff

    float ratio = 1.f + ( _actualDownSampleAmount / _maxDownSample );
    for ( int c = 0; c < _amountOfChannels; ++c ) {
        _lowPassFilters.at( c )->setRatio( ratio );
    }
}

void PluginProcess::cacheLfo()
{
    float scaledAmount = _downSampleAmount / _maxDownSample; // 0 - 1 range

    _downSampleLfoRange = scaledAmount * _downSampleLfoDepth;
    _downSampleLfoMax   = std::min( 1.f, scaledAmount + _downSampleLfoRange * .5f );
    _downSampleLfoMin   = std::max( 0.f, scaledAmount - _downSampleLfoRange * .5f );

    _playbackRateLfoRange = _playbackRate * _playbackRateLfoDepth;
    _playbackRateLfoMax   = std::min( 1.f, _playbackRate + _playbackRateLfoRange * .5f );
    _playbackRateLfoMin   = std::max( 0.f, _playbackRate - _playbackRateLfoRange * .5f );
}

void PluginProcess::cacheMaxDownSample()
{
    _maxDownSample = VST::SAMPLE_RATE / MIN_SAMPLE_RATE;
}

void PluginProcess::setActualDownSampling( float value )
{
    bool wasDownSampled     = isDownSampled();
    _actualDownSampleAmount = value;
    cacheDownSamplingValues();

    // if down sampling is deactivated and there is no oscillation for the down sample rate
    // and no playback slowdown taking place: sync the read pointer with the write pointer

    if ( wasDownSampled && !isDownSampled() && !_hasDownSampleLfo && !isSlowedDown() && !_hasPlaybackRateLfo ) {
        _readPointer = ( float ) _writePointer;
    }
}

void PluginProcess::setActualPlaybackRate( float value )
{
    bool wasSlowedDown  = isSlowedDown();
    _actualPlaybackRate = value;

    // if slowdown is deactivated and there is no oscillation for the playback rate
    // and no down sampling taking place: sync the read pointer with the write pointer

    if ( wasSlowedDown && !isSlowedDown() && !_hasPlaybackRateLfo && !isDownSampled() ) {
        _readPointer = ( float ) _writePointer;
    }
}

}
