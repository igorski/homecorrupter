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
#ifndef __PARAMIDS_HEADER__
#define __PARAMIDS_HEADER__

enum
{
    // ids for all visual controls
    // these identifiers are mapped to the UI in plugin.uidesc
    // and consumed by controller.cpp to update the model

// --- AUTO-GENERATED START
    kResampleRateId = 0,    // Resample rate
    kBitDepthId = 1,    // Resolution
    kPlaybackRateId = 2,    // Playback rate
    kResampleLfoId = 3,    // Resampling LFO
    kResampleLfoDepthId = 4,    // Resampling LFO depth
    kBitCrushLfoId = 5,    // Bit crush LFO
    kBitCrushLfoDepthId = 6,    // Bit crush LFO depth
    kPlaybackRateLfoId = 7,    // Playback LFO
    kPlaybackRateLfoDepthId = 8,    // Playback LFO depth
    kWetMixId = 9,    // Wet mix
    kDryMixId = 10,    // Dry mix

// --- AUTO-GENERATED END

    kVuPPMId // for the Vu value return to host
};

#endif
