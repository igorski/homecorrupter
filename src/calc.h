/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2022 Igor Zinken - https://www.igorski.nl
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
#ifndef __CALC_HEADER__
#define __CALC_HEADER__

#include <cmath>
#include <algorithm>
#include "global.h"

/***
 * Taken from JUCE library
 * This macro can be applied to a float variable to check whether it contains a denormalised value, and to normalise it if necessary.
 * On CPUs that aren't vulnerable to denormalisation problems, this will have no effect.
 * This activates the Flush To Zero(FTZ) and Denormals Are Zero (DAZ) mode for x86 and x64
 */
#define UNDENORMALISE(x) { (x) += 0.1f; (x) -= 0.1f; }

/**
 * convenience utilities to process values
 * common to the VST plugin context
 */
namespace Igorski {
namespace Calc {

    /**
     * convert given value in seconds to the appropriate
     * value in samples (for the current sampling rate)
     */
    inline int secondsToBuffer( float seconds )
    {
        return ( int )( seconds * Igorski::VST::SAMPLE_RATE );
    }

    /**
     * convert given value in milliseconds to the appropriate
     * value in samples (for the current sampling rate)
     */
    inline int millisecondsToBuffer( float milliseconds )
    {
        return secondsToBuffer( milliseconds / 1000.f );
    }

    // convenience method to ensure given value is within the 0.f - +1.f range

    inline float cap( float value )
    {
        return std::min( 1.f, std::max( 0.f, value ));
    }

    // convenience method to ensure a sample is in the valid -1.f - +1.f range

    inline float capSample( float value )
    {
        return std::min( 1.f, std::max( -1.f, value ));
    }

    inline double capSample( double value )
    {
        return std::min( 1.0, std::max( -1.0, value ));
    }

    // convenience method to round given number value to the nearest
    // multiple of valueToRoundTo
    // e.g. roundTo( 236.32, 10 ) == 240 and roundTo( 236.32, 5 ) == 235

    inline float roundTo( float value, float valueToRoundTo )
    {
        float resto = fmod( value, valueToRoundTo );

        if ( resto <= ( valueToRoundTo / 2 ))
            return value - resto;

        return value + valueToRoundTo - resto;
    }

    // convenience method to scale given value and its expected maxValue against
    // an arbitrary range (defined by maxCompareValue in relation to maxValue)

    inline float scale( float value, float maxValue, float maxCompareValue )
    {
        float ratio = maxCompareValue / maxValue;
        return ( float ) ( std::min( maxValue, value ) * ratio );
    }

    // cast a floating point value to a boolean true/false

    inline bool toBool( float value )
    {
        return value >= .5;
    }

    // inverts a pow operation, allowing you to derive the exponent from the known value and base

    inline float inversePow( float value, float base )
    {
        return log( value ) / log( base );
    }

    // inverts a log operation, allowing you to derive the original number from the known value and base
    // e.g. reverting log10( 0.5 ) is inverseLog( -0.3010299956639812, 10 ) == 0.5

    inline float inverseLog( float value, float base )
    {
        return pow( base, value );
    }
}
}

#endif
