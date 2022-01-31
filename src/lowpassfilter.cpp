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
#include "lowpassfilter.h"
#include "global.h"
#include <algorithm>
#include <cmath>

namespace Igorski {

/* constructor / destructor */

LowPassFilter::LowPassFilter()
{

}

LowPassFilter::~LowPassFilter()
{

}

/* public methods */

void LowPassFilter::setRatio( float frequencyRatio )
{
    const float proportionalRate = frequencyRatio > 1.0f ? 0.5f / frequencyRatio : 0.5f  * frequencyRatio;
    const float n = 1.f / tan(( float ) VST::PI * std::max( 0.001f, proportionalRate ));
    const float nSquared = n * n;

    float c1 = 1.f / ( 1.f + VST::SQRT_TWO * n + nSquared );

    UNDENORMALISE( c1 );

    setFilterCoefficients(
        c1,
        c1 * 2.f,
        c1,
        1.f,
        c1 * 2.f * ( 1.f - nSquared ),
        c1 * ( 1.f - VST::SQRT_TWO * n + nSquared )
    );
}

void LowPassFilter::applyFilter( float* samples, int amountOfSamples )
{
    while ( --amountOfSamples >= 0 )
    {
        *samples++ = applySingle( *samples );
    }
}

void LowPassFilter::resetFilter()
{
    x1 = 0.f;
    x2 = 0.f;
    y1 = 0.f;
    y2 = 0.f;
}

/* private methods */

void LowPassFilter::setFilterCoefficients( float c1, float c2, float c3, float c4, float c5, float c6 )
{
    // c4 is always passed as 1.f making the value of const float a equal to 1.f
    /*
    const float a = 1.f / c4;

    c1 *= a;
    c2 *= a;
    c3 *= a;
    c5 *= a;
    c6 *= a;
    */
    coefficients[ 0 ] = c1;
    coefficients[ 1 ] = c2;
    coefficients[ 2 ] = c3;
    coefficients[ 3 ] = c4;
    coefficients[ 4 ] = c5;
    coefficients[ 5 ] = c6;
}

}
