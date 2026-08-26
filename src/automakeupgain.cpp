/*
 * Copyright (c) 2026 Igor Zinken https://www.igorski.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
        sumSquares += data[ i ] * data[ i ];
    }
    return static_cast<float>( std::sqrt( sumSquares / ( double ) numSamples + 1e-12 ));
}

} // E.O. namespace