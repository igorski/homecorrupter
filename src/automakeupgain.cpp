/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Igor Zinken - https://www.igorski.nl
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
#include "automakeupgain.h"
#include "calc.h"

namespace Igorski {

/* public methods */

void AutoMakeUpGain::prepare( float sampleRate )
{
    rmsWindowSize = static_cast<int>( sampleRate * WINDOW_SIZE );
    rmsWindowSize = std::max( 1, rmsWindowSize );

    smoother.reset( sampleRate, GAIN_SMOOTHING );
    smoother.setCurrentAndTargetValue( 1.0f );
}

void AutoMakeUpGain::apply( float* pre, float* post, int bufferSize  )
{
    float inRMS  = computeRMS( pre, bufferSize );
    float outRMS = computeRMS( post, bufferSize );

    float makeup = ( outRMS > 1e-9f ) ? ( inRMS / outRMS ) : 1.0f;

    makeup = Calc::constrain( 0.25f, 4.0f, makeup );

    
    smoother.setTargetValue( makeup );
    float smoothGain = smoother.getNextValue();
    smoother.skip( bufferSize );

    for ( int i = 0; i < bufferSize; ++i ) {
        post[ i ] *= smoothGain;
    }
}

/* private methods */

float AutoMakeUpGain::computeRMS( const float* data, int numSamples )
{
    double sumSquares = 0.0;

    for ( int i = 0; i < numSamples; ++i ) {
        auto sampleValue = static_cast<double>( data[ i ]);
        sumSquares += sampleValue * sampleValue;
    }
    return static_cast<float>( std::sqrt( sumSquares / ( double ) numSamples + 1e-12 ));
}

} // E.O. namespace