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
#ifndef __AUTO_MAKEUP_GAIN_H_INCLUDED__
#define __AUTO_MAKEUP_GAIN_H_INCLUDED__

namespace Igorski {
class AutoMakeUpGain
{
    // values are in seconds

    static constexpr double WINDOW_SIZE    = 0.02;
    static constexpr double GAIN_SMOOTHING = 0.01;

    public:
        void prepare( float sampleRate );

        /**
         * Apply makeup gain to make the differences between
         * provided pre- and post buffer states smaller
         */
        void apply( float* pre, float* post, int bufferSize );

    private:
        int rmsWindowSize = 0;

        float computeRMS( const float* data, int numSamples );
};
}

#endif