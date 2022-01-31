/**
 * Ported from mdaLimiterProcessor.cpp
 * Created by Arne Scheffler on 6/14/08.
 *
 * mda VST Plug-ins
 *
 * Copyright (c) 2008 Paul Kellett
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
#include "limiter.h"
#include "global.h"
#include "calc.h"
#include <math.h>

// constructors / destructor

Limiter::Limiter()
{
    init( 0.8f, 1.0f, 0.55f, true );
}

Limiter::Limiter( float attackNormalized, float releaseNormalized, float thresholdNormalized )
{
    init( attackNormalized, releaseNormalized, thresholdNormalized, false );
}

Limiter::Limiter( float attackInMicroseconds, float releaseInMilliseconds, float thresholdNormalized, bool softKnee )
{
    init( 0.f, 0.f, thresholdNormalized, softKnee );
    setAttackMicroseconds( attackInMicroseconds );
    setReleaseMilliseconds( releaseInMilliseconds );
}

Limiter::~Limiter()
{
    // nowt...
}

/* public methods */

void Limiter::setAttack( float attackNormalized )
{
    _attack = pow( 10.0, -2.0 * attackNormalized );
}

void Limiter::setAttackMicroseconds( float attackInMicroseconds )
{
    _attack = 1.0 - Igorski::Calc::inverseLog( 1.f / ( attackInMicroseconds / -301030.1f ) / ( float ) Igorski::VST::SAMPLE_RATE, 10 );
}

void Limiter::setRelease( float releaseNormalized )
{
    _release = pow( 10.0, -2.0 - ( 3.0 * releaseNormalized ));
}

void Limiter::setReleaseMilliseconds( float releaseInMilliseconds )
{
    _release = 1.0 - Igorski::Calc::inverseLog( 1.f / ( releaseInMilliseconds / -301.0301f ) / ( float ) Igorski::VST::SAMPLE_RATE, 10 );
}

void Limiter::setThreshold( float thresholdNormalized )
{
    _threshold = thresholdNormalized;
    cacheValues();
}

bool Limiter::getSoftKnee()
{
    return _softKnee;
}

void Limiter::setSoftKnee( bool softKnee )
{
    _softKnee = softKnee;
    cacheValues();
}

float Limiter::getLinearGR()
{
    return ( _gain > 1.0f ) ? 1.0f / ( float ) _gain : 1.0f;
}

/* protected methods */

void Limiter::init( float attackNormalized, float releaseNormalized, float thresholdNormalized, bool softKnee )
{
    _threshold = thresholdNormalized;

    float trim = 0.60f;

    _gain = 1.0f;
    _trim = pow( 10.0, ( 2.0 * trim ) - 1.0 );

    setAttack( attackNormalized );
    setRelease( releaseNormalized );
    setSoftKnee( softKnee );
}

void Limiter::cacheValues()
{
    if ( _softKnee ) {
        pThreshold = pow( 10.0, 1.0 - ( 2.0 * _threshold ));
    } else {
        pThreshold = pow( 10.0, ( 2.0 * _threshold ) - 2.0 );
    }
}
