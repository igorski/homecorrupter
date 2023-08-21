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
template <typename SampleType>
void Limiter::process( SampleType** outputBuffer, int bufferSize, int numOutChannels )
{
//    if ( gain > 0.9999f && outputBuffer->isSilent() )
//    {
//        // don't process if input is silent
//        return;
//    }

    SampleType gain, level, leftSample, rightSample;

    gain = _gain;

    bool hasRight = ( numOutChannels > 1 );

    SampleType* leftBuffer  = outputBuffer[ 0 ];
    SampleType* rightBuffer = hasRight ? outputBuffer[ 1 ] : 0;

    if ( _softKnee )
    {
        for ( int i = 0; i < bufferSize; ++i ) {

            leftSample  = leftBuffer[ i ];
            rightSample = hasRight ? rightBuffer[ i ] : 0;

            level = SampleType ( 1.0 / ( 1.0 + pThreshold * fabs( leftSample + rightSample )));

            if ( gain > level ) {
                gain = gain - _attack * ( gain - level );
            }
            else {
                gain = gain + _release * ( level - gain );
            }

            leftBuffer[ i ] = ( leftSample * _trim * gain );

            if ( hasRight )
                rightBuffer[ i ] = ( rightSample * _trim * gain );
        }
    }
    else
    {
        for ( int i = 0; i < bufferSize; ++i ) {

            leftSample  = leftBuffer[ i ];
            rightSample = hasRight ? rightBuffer[ i ] : 0;

            level = SampleType ( 0.5 * gain * fabs( leftSample + rightSample ));

            if ( level > pThreshold ) {
                gain = gain - ( _attack * ( level - pThreshold ));
            }
            else {
                // below threshold
                gain = gain + SampleType ( _release * ( 1.0 - gain ));
            }

            leftBuffer[ i ] = ( leftSample * _trim * gain );

            if ( hasRight )
                rightBuffer[ i ] = ( rightSample * _trim * gain );
        }
    }
    _gain = gain;
}
