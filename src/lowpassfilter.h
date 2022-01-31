/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2022 Igor Zinken - https://www.igorski.nl
 *
 * Adaptation of source provided in the JUCE library:
 * Copyright (c) 2020 - Raw Material Software Limited
 *
 * JUCE is an open source library subject to commercial or open-source
 * licensing.
 *
 * The code included in this file is provided under the terms of the ISC license
 * http://www.isc.org/downloads/software-support-policy/isc-license. Permission
 * To use, copy, modify, and/or distribute this software for any purpose with or
 * without fee is hereby granted provided that the above copyright notice and
 * this permission notice appear in all copies.
 *
 * JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
 * EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
 * DISCLAIMED.
 */
#ifndef __LOWPASSFILTER_H_INCLUDED__
#define __LOWPASSFILTER_H_INCLUDED__

#include "calc.h"

namespace Igorski {
class LowPassFilter
{
    public:
        LowPassFilter();
        ~LowPassFilter();

        void setRatio( float frequencyRatio );
        void applyFilter( float* samples, int bufferSize );
        void resetFilter();

        inline float applySingle( float sample ) {
            float out = coefficients[ 0 ] * sample
                      + coefficients[ 1 ] * x1
                      + coefficients[ 2 ] * x2
                      - coefficients[ 4 ] * y1
                      - coefficients[ 5 ] * y2;

            UNDENORMALISE( out );

            x2 = x1;
            x1 = sample;
            y2 = y1;
            y1 = out;

            return out;
        }

    private:
        void setFilterCoefficients( float c1, float c2, float c3, float c4, float c5, float c6 );

        float coefficients[ 6 ];
        float x1 = 0.f;
        float x2 = 0.f;
        float y1 = 0.f;
        float y2 = 0.f;
};
}

#endif
