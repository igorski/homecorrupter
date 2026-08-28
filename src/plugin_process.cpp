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
#include "plugin_process.h"
#include "calc.h"
#include <math.h>
#include <algorithm>

namespace Igorski {

PluginProcess::PluginProcess( int amountOfChannels, float sampleRate, int maxBufferSize )
{
    _amountOfChannels = amountOfChannels;

    _lowPassFilters.resize( amountOfChannels );
    _makeUpGainProcessors.resize( amountOfChannels );
    _channelStates.resize( amountOfChannels );

    _dryMix = 0.f;
    _wetMix = 1.f;

    // create the child processors

    bitCrusher = new BitCrusher( 1.f, .5f, 1.f );
    limiter    = new Limiter( 0.3f, 0.5f, 0.9f, true, sampleRate );

    // buffers will be lazily created in the process function as they correspond to the host buffer size
    _recordBuffer  = nullptr;
    _preMixBuffer  = nullptr;

    // oscillators
    _hasDownSampleLfo   = false;
    _hasPlaybackRateLfo = false;

    // read / write variables

    _readPointer  = 0.f;
    _writePointer = 0;

    _downSampleNormalised   = 0.f;
    _downSampleAmount       = 0.f;
    _actualDownSampleAmount = 1.f;
    _playbackRate           = 0.f;
    _actualPlaybackRate     = 1.f;

    setHostProperties( sampleRate, maxBufferSize );
    setResampleRate( _actualDownSampleAmount );
    setPlaybackRate( _actualPlaybackRate );
}

PluginProcess::~PluginProcess()
{
    if ( _scratchBuffer != nullptr ) {
        delete[] _scratchBuffer;
        _scratchBuffer = nullptr;
    }
    delete bitCrusher;
    delete limiter;
    delete _recordBuffer;
    delete _preMixBuffer;
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

void PluginProcess::setHostProperties( float sampleRate, int maxBufferSize )
{
    cacheMaxDownSample();

    if ( _hostSampleRate != sampleRate ) {
        _hostSampleRate = sampleRate;
    
        setResampleRate( _downSampleNormalised );

        limiter->setSampleRate( _hostSampleRate );
        bitCrusher->setSampleRate( _hostSampleRate );
        _downSampleLfo.setSampleRate( _hostSampleRate );
        _playbackRateLfo.setSampleRate( _hostSampleRate );

        for ( int c = 0; c < _amountOfChannels; ++c ) {
            _makeUpGainProcessors.at( c ).prepare( _hostSampleRate );
        }
    }

    if ( _hostBufferSize < maxBufferSize ) {
        _hostBufferSize = maxBufferSize;
        if ( _scratchBuffer != nullptr ) {
            delete[] _scratchBuffer;
        }
        _scratchBuffer = new float[ _hostBufferSize ];
    }
}

void PluginProcess::setResampleRate( float value )
{
    float normalisedDownSampleAmount = normalisedToDownSampleAmount( value );
    
    // no change in either resample rate or host sample rate value
    // no need to trigger changes

    if ( normalisedDownSampleAmount == _downSampleAmount ) {
        return;
    }

    float tempRatio = _actualDownSampleAmount / std::max( 0.000000001f, _downSampleAmount );

    // this should reflect the value visible in the UI
    _targetRate = VST::MIN_SAMPLE_RATE + value * ( _hostSampleRate - VST::MIN_SAMPLE_RATE );
    
    _downSampleNormalised = value;
    _downSampleAmount = normalisedDownSampleAmount;

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
        _downSampleLfo.setRate(
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
    float scaledAmount = Calc::scale( value, 1, VST::MIN_PLAYBACK_SPEED ) + VST::MIN_PLAYBACK_SPEED;

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
        _playbackRateLfo.setRate(
            VST::MIN_LFO_RATE() + (
                LFORatePercentage * ( VST::MAX_LFO_RATE() - VST::MIN_LFO_RATE() )
            )
        );

    // turning LFO off
    if ( !_hasPlaybackRateLfo && wasEnabled ) {
        _actualPlaybackRate = _playbackRate;

        // when playback rate LFO is deactivated and there is no slowdown for the playback rate
        // active: sync the read pointer with the write pointer to align with incoming audio

        if ( !isSlowedDown() ) {
            _readPointer = static_cast<float>( _writePointer );
            syncFilterPointers();
        }
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
    syncFilterPointers();
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
    _sampleIncr = static_cast<int>( roundf( _actualDownSampleAmount ));
    _sampleIncr = std::max( 1, std::min( _sampleIncr, static_cast<int>( _maxDownSample )));

    // update the lowpass filters to the appropriate cutoff

    float ratio = 1.f + ( _actualDownSampleAmount / _maxDownSample );
    for ( int c = 0; c < _amountOfChannels; ++c ) {
        _lowPassFilters.at( c ).setRatio( ratio );
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
    _maxDownSample = _hostSampleRate / VST::MIN_SAMPLE_RATE;
}

void PluginProcess::setActualDownSampling( float value )
{
    bool wasDownSampled     = isDownSampled();
    _actualDownSampleAmount = value;
    cacheDownSamplingValues();

    // if down sampling is deactivated and there is no oscillation for the down sample rate
    // and no playback slowdown taking place: sync the read pointer with the write pointer

    if ( wasDownSampled && !isDownSampled() && !_hasDownSampleLfo && !isSlowedDown() && !_hasPlaybackRateLfo ) {
        _readPointer = static_cast<float>( _writePointer );
        syncFilterPointers();
    }
}

void PluginProcess::setActualPlaybackRate( float value )
{
    bool wasSlowedDown  = isSlowedDown();
    _actualPlaybackRate = value;

    // when slowdown is deactivated and there is no oscillation for the playback rate
    // taking place: sync the read pointer with the write pointer to align with incoming audio

    if ( wasSlowedDown && !isSlowedDown() && !_hasPlaybackRateLfo ) {
        _readPointer = static_cast<float>( _writePointer );
        syncFilterPointers();
    }
}

}
