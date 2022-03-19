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
#include "global.h"
#include "vst.h"
#include "paramids.h"
#include "calc.h"

#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include <stdio.h>

namespace Igorski {

float VST::SAMPLE_RATE = 44100.f; // updated in setupProcessing()

//------------------------------------------------------------------------
// Plugin Implementation
//------------------------------------------------------------------------
Homecorrupter::Homecorrupter()
: pluginProcess( nullptr )
, outputGainOld( 0.f )
, currentProcessMode( -1 ) // -1 means not initialized
{
    // register its editor class (the same as used in vstentry.cpp)
    setControllerClass( VST::PluginControllerUID );

    // should be created on setupProcessing, this however doesn't fire for Audio Unit using auval?
    pluginProcess = new PluginProcess( 2 );
}

//------------------------------------------------------------------------
Homecorrupter::~Homecorrupter()
{
    // free all allocated resources
    delete pluginProcess;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::initialize( FUnknown* context )
{
    //---always initialize the parent-------
    tresult result = AudioEffect::initialize( context );
    // if everything Ok, continue
    if ( result != kResultOk )
        return result;

    //---create Audio In/Out buses------
    addAudioInput ( STR16( "Stereo In" ),  SpeakerArr::kStereo );
    addAudioOutput( STR16( "Stereo Out" ), SpeakerArr::kStereo );

    //---create Event In/Out buses (1 bus with only 1 channel)------
    addEventInput( STR16( "Event In" ), 1 );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::terminate()
{
    // nothing to do here yet...except calling our parent terminate
    return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::setActive (TBool state)
{
    if (state)
        sendTextMessage( "Homecorrupter::setActive (true)" );
    else
        sendTextMessage( "Homecorrupter::setActive (false)" );

    // reset output level meter
    outputGainOld = 0.f;

    // call our parent setActive
    return AudioEffect::setActive( state );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::process( ProcessData& data )
{
    // In this example there are 4 steps:
    // 1) Read inputs parameters coming from host (in order to adapt our model values)
    // 2) Read inputs events coming from host (note on/off events)
    // 3) Apply the effect using the input buffer into the output buffer

    //---1) Read input parameter changes-----------
    IParameterChanges* paramChanges = data.inputParameterChanges;
    if ( paramChanges )
    {
        int32 numParamsChanged = paramChanges->getParameterCount();
        // for each parameter which are some changes in this audio block:
        for ( int32 i = 0; i < numParamsChanged; i++ )
        {
            IParamValueQueue* paramQueue = paramChanges->getParameterData( i );
            if ( paramQueue )
            {
                ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                switch ( paramQueue->getParameterId())
                {
// --- AUTO-GENERATED PROCESS START

                    case kResampleRateId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fResampleRate = ( float ) value;
                        break;

                    case kBitDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fBitDepth = ( float ) value;
                        break;

                    case kPlaybackRateId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fPlaybackRate = ( float ) value;
                        break;

                    case kResampleLfoId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fResampleLfo = ( float ) value;
                        break;

                    case kResampleLfoDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fResampleLfoDepth = ( float ) value;
                        break;

                    case kBitCrushLfoId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fBitCrushLfo = ( float ) value;
                        break;

                    case kBitCrushLfoDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fBitCrushLfoDepth = ( float ) value;
                        break;

                    case kPlaybackRateLfoId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fPlaybackRateLfo = ( float ) value;
                        break;

                    case kPlaybackRateLfoDepthId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fPlaybackRateLfoDepth = ( float ) value;
                        break;

                    case kWetMixId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fWetMix = ( float ) value;
                        break;

                    case kDryMixId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            fDryMix = ( float ) value;
                        break;

// --- AUTO-GENERATED PROCESS END
                    case kBypassId:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value) == kResultTrue ) {
                            _bypass = value >= 0.5f;
                        }
                        break;
                }
                syncModel();
            }
        }
    }

    // according to docs: processing context (optional, but most welcome)

    if ( data.processContext != nullptr )
    {
        bool wasPlaying = isPlaying;

        // when host starts sequencer, ensure the process read pointer and write pointers are reset

        isPlaying = data.processContext->state & ProcessContext::kPlaying;

        if ( !wasPlaying && isPlaying ) {
            pluginProcess->resetReadWritePointers();
        }

        // clear the record buffers on sequencer start / stop to prevent slowed down playback from
        // reading old data that has been recorded into its "future"

        if ( wasPlaying != isPlaying ) {
            pluginProcess->clearBuffer();
        }

        // in case you want to do tempo synchronization with the host
        /*
        pluginProcess->setTempo(
            data.processContext->tempo, data.processContext->timeSigNumerator, data.processContext->timeSigDenominator
        );
        */
    }

    //---2) Read input events-------------
//    IEventList* eventList = data.inputEvents;


    //-------------------------------------
    //---3) Process Audio---------------------
    //-------------------------------------

    if ( data.numInputs == 0 || data.numOutputs == 0 )
    {
        // nothing to do
        return kResultOk;
    }

    int32 numInChannels  = data.inputs[ 0 ].numChannels;
    int32 numOutChannels = data.outputs[ 0 ].numChannels;

    // --- get audio buffers----------------
    uint32 sampleFramesSize = getSampleFramesSizeInBytes( processSetup, data.numSamples );
    void** in  = getChannelBuffersPointer( processSetup, data.inputs [ 0 ] );
    void** out = getChannelBuffersPointer( processSetup, data.outputs[ 0 ] );

    bool isDoublePrecision = ( data.symbolicSampleSize == kSample64 );

    if ( _bypass )
    {
        // bypass mode, ensure output equals input

        for ( int32 i = 0; i < numInChannels; i++ ) {
            if ( in[ i ] != out[ i ]) {
                memcpy( out[ i ], in[ i ], sampleFramesSize );
            }
        }
    }
    else
    {
        // process the incoming sound!

        if ( isDoublePrecision ) {
            // 64-bit samples, e.g. Reaper64
            pluginProcess->process<double>(
                ( double** ) in, ( double** ) out, numInChannels, numOutChannels,
                data.numSamples, sampleFramesSize
            );
        }
        else {
            // 32-bit samples, e.g. Ableton Live, Bitwig Studio... (oddly enough also when 64-bit?)
            pluginProcess->process<float>(
                ( float** ) in, ( float** ) out, numInChannels, numOutChannels,
                data.numSamples, sampleFramesSize
            );
        }
    }

    // output flags

    data.outputs[ 0 ].silenceFlags = false; // there should always be output
    float outputGain = pluginProcess->limiter->getLinearGR();

    //---4) Write output parameter changes-----------
    IParameterChanges* outParamChanges = data.outputParameterChanges;
    // a new value of VuMeter will be sent to the host
    // (the host will send it back in sync to our controller for updating our editor)
    if ( !isDoublePrecision && outParamChanges && outputGainOld != outputGain ) {
        int32 index = 0;
        IParamValueQueue* paramQueue = outParamChanges->addParameterData( kVuPPMId, index );
        if ( paramQueue )
            paramQueue->addPoint( 0, outputGain, index );
    }
    outputGainOld = outputGain;
    return kResultOk;
}

//------------------------------------------------------------------------
tresult Homecorrupter::receiveText( const char* text )
{
    // received from Controller
    fprintf( stderr, "[Homecorrupter] received: " );
    fprintf( stderr, "%s", text );
    fprintf( stderr, "\n" );

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::setState( IBStream* state )
{
    // called when we load a preset, the model has to be reloaded

// --- AUTO-GENERATED SETSTATE START

    float savedResampleRate = 0.f;
    if ( state->read( &savedResampleRate, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedBitDepth = 0.f;
    if ( state->read( &savedBitDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedPlaybackRate = 0.f;
    if ( state->read( &savedPlaybackRate, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedResampleLfo = 0.f;
    if ( state->read( &savedResampleLfo, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedResampleLfoDepth = 0.f;
    if ( state->read( &savedResampleLfoDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedBitCrushLfo = 0.f;
    if ( state->read( &savedBitCrushLfo, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedBitCrushLfoDepth = 0.f;
    if ( state->read( &savedBitCrushLfoDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedPlaybackRateLfo = 0.f;
    if ( state->read( &savedPlaybackRateLfo, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedPlaybackRateLfoDepth = 0.f;
    if ( state->read( &savedPlaybackRateLfoDepth, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedWetMix = 0.f;
    if ( state->read( &savedWetMix, sizeof ( float )) != kResultOk )
        return kResultFalse;

    float savedDryMix = 0.f;
    if ( state->read( &savedDryMix, sizeof ( float )) != kResultOk )
        return kResultFalse;

// --- AUTO-GENERATED SETSTATE END

    int32 savedBypass = 0;
    if ( state->read( &savedBypass, sizeof ( int32 )) != kResultOk )
        return kResultFalse;

#if BYTEORDER == kBigEndian

// --- AUTO-GENERATED SETSTATE SWAP START
   SWAP_32( savedResampleRate )
   SWAP_32( savedBitDepth )
   SWAP_32( savedPlaybackRate )
   SWAP_32( savedResampleLfo )
   SWAP_32( savedResampleLfoDepth )
   SWAP_32( savedBitCrushLfo )
   SWAP_32( savedBitCrushLfoDepth )
   SWAP_32( savedPlaybackRateLfo )
   SWAP_32( savedPlaybackRateLfoDepth )
   SWAP_32( savedWetMix )
   SWAP_32( savedDryMix )

// --- AUTO-GENERATED SETSTATE SWAP END

#endif

// --- AUTO-GENERATED SETSTATE APPLY START
    fResampleRate = savedResampleRate;
    fBitDepth = savedBitDepth;
    fPlaybackRate = savedPlaybackRate;
    fResampleLfo = savedResampleLfo;
    fResampleLfoDepth = savedResampleLfoDepth;
    fBitCrushLfo = savedBitCrushLfo;
    fBitCrushLfoDepth = savedBitCrushLfoDepth;
    fPlaybackRateLfo = savedPlaybackRateLfo;
    fPlaybackRateLfoDepth = savedPlaybackRateLfoDepth;
    fWetMix = savedWetMix;
    fDryMix = savedDryMix;

// --- AUTO-GENERATED SETSTATE APPLY END

    _bypass = savedBypass > 0;

    syncModel();

    // Example of using the IStreamAttributes interface
    FUnknownPtr<IStreamAttributes> stream (state);
    if ( stream )
    {
        IAttributeList* list = stream->getAttributes ();
        if ( list )
        {
            // get the current type (project/Default..) of this state
            String128 string = {0};
            if ( list->getString( PresetAttributes::kStateType, string, 128 * sizeof( TChar )) == kResultTrue )
            {
                UString128 tmp( string );
                char ascii[128];
                tmp.toAscii( ascii, 128 );
                if ( !strncmp( ascii, StateType::kProject, strlen( StateType::kProject )))
                {
                    // we are in project loading context...
                }
            }

            // get the full file path of this state
            TChar fullPath[1024];
            memset( fullPath, 0, 1024 * sizeof( TChar ));
            if ( list->getString( PresetAttributes::kFilePathStringType,
                 fullPath, 1024 * sizeof( TChar )) == kResultTrue )
            {
                // here we have the full path ...
            }
        }
    }
    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::getState( IBStream* state )
{
    // here we save the model values

// --- AUTO-GENERATED GETSTATE START
    float toSaveResampleRate = fResampleRate;
    float toSaveBitDepth = fBitDepth;
    float toSavePlaybackRate = fPlaybackRate;
    float toSaveResampleLfo = fResampleLfo;
    float toSaveResampleLfoDepth = fResampleLfoDepth;
    float toSaveBitCrushLfo = fBitCrushLfo;
    float toSaveBitCrushLfoDepth = fBitCrushLfoDepth;
    float toSavePlaybackRateLfo = fPlaybackRateLfo;
    float toSavePlaybackRateLfoDepth = fPlaybackRateLfoDepth;
    float toSaveWetMix = fWetMix;
    float toSaveDryMix = fDryMix;

// --- AUTO-GENERATED GETSTATE END

    int32 toSaveBypass = _bypass ? 1 : 0;

#if BYTEORDER == kBigEndian

// --- AUTO-GENERATED GETSTATE SWAP START
   SWAP_32( toSaveResampleRate )
   SWAP_32( toSaveBitDepth )
   SWAP_32( toSavePlaybackRate )
   SWAP_32( toSaveResampleLfo )
   SWAP_32( toSaveResampleLfoDepth )
   SWAP_32( toSaveBitCrushLfo )
   SWAP_32( toSaveBitCrushLfoDepth )
   SWAP_32( toSavePlaybackRateLfo )
   SWAP_32( toSavePlaybackRateLfoDepth )
   SWAP_32( toSaveWetMix )
   SWAP_32( toSaveDryMix )

// --- AUTO-GENERATED GETSTATE SWAP END

    SWAP_32( toSaveBypass );

#endif

// --- AUTO-GENERATED GETSTATE APPLY START
    state->write( &toSaveResampleRate, sizeof( float ));
    state->write( &toSaveBitDepth, sizeof( float ));
    state->write( &toSavePlaybackRate, sizeof( float ));
    state->write( &toSaveResampleLfo, sizeof( float ));
    state->write( &toSaveResampleLfoDepth, sizeof( float ));
    state->write( &toSaveBitCrushLfo, sizeof( float ));
    state->write( &toSaveBitCrushLfoDepth, sizeof( float ));
    state->write( &toSavePlaybackRateLfo, sizeof( float ));
    state->write( &toSavePlaybackRateLfoDepth, sizeof( float ));
    state->write( &toSaveWetMix, sizeof( float ));
    state->write( &toSaveDryMix, sizeof( float ));

// --- AUTO-GENERATED GETSTATE APPLY END

    state->write( &toSaveBypass, sizeof( int32 ));

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::setupProcessing( ProcessSetup& newSetup )
{
    // called before the process call, always in a disabled state (not active)

    // here we keep a trace of the processing mode (offline,...) for example.
    currentProcessMode = newSetup.processMode;

    VST::SAMPLE_RATE = newSetup.sampleRate;

    // spotted to fire multiple times...

    if ( pluginProcess != nullptr ) {
        delete pluginProcess;
    }

    // TODO: creating a bunch of extra channels for no apparent reason?
    // get the correct channel amount and don't allocate more than necessary...
    pluginProcess = new PluginProcess( 6 );

    syncModel();

    return AudioEffect::setupProcessing( newSetup );
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::setBusArrangements( SpeakerArrangement* inputs,  int32 numIns,
                                                 SpeakerArrangement* outputs, int32 numOuts )
{
    if ( numIns == 1 && numOuts == 1 )
    {
        // the host wants Mono => Mono (or 1 channel -> 1 channel)
        if ( SpeakerArr::getChannelCount( inputs[0])  == 1 &&
             SpeakerArr::getChannelCount( outputs[0]) == 1 )
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at( 0 ));
            if ( bus )
            {
                // check if we are Mono => Mono, if not we need to recreate the buses
                if ( bus->getArrangement() != inputs[0])
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Mono In" ),  inputs[0] );
                    addAudioOutput( STR16( "Mono Out" ), inputs[0] );
                }
                return kResultOk;
            }
        }
        // the host wants something else than Mono => Mono, in this case we are always Stereo => Stereo
        else
        {
            AudioBus* bus = FCast<AudioBus>( audioInputs.at(0));
            if ( bus )
            {
                tresult result = kResultFalse;

                // the host wants 2->2 (could be LsRs -> LsRs)
                if ( SpeakerArr::getChannelCount(inputs[0]) == 2 && SpeakerArr::getChannelCount( outputs[0]) == 2 )
                {
                    removeAudioBusses();
                    addAudioInput  ( STR16( "Stereo In"),  inputs[0] );
                    addAudioOutput ( STR16( "Stereo Out"), outputs[0]);
                    result = kResultTrue;
                }
                // the host want something different than 1->1 or 2->2 : in this case we want stereo
                else if ( bus->getArrangement() != SpeakerArr::kStereo )
                {
                    removeAudioBusses();
                    addAudioInput ( STR16( "Stereo In"),  SpeakerArr::kStereo );
                    addAudioOutput( STR16( "Stereo Out"), SpeakerArr::kStereo );
                    result = kResultFalse;
                }
                return result;
            }
        }
    }
    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::canProcessSampleSize( int32 symbolicSampleSize )
{
    if ( symbolicSampleSize == kSample32 )
        return kResultTrue;

    // we support double processing
    if ( symbolicSampleSize == kSample64 )
        return kResultTrue;

    return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Homecorrupter::notify( IMessage* message )
{
    if ( !message )
        return kInvalidArgument;

    if ( !strcmp( message->getMessageID(), "BinaryMessage" ))
    {
        const void* data;
        uint32 size;
        if ( message->getAttributes ()->getBinary( "MyData", data, size ) == kResultOk )
        {
            // we are in UI thread
            // size should be 100
            if ( size == 100 && ((char*)data)[1] == 1 ) // yeah...
            {
                fprintf( stderr, "[Homecorrupter] received the binary message!\n" );
            }
            return kResultOk;
        }
    }
    return AudioEffect::notify( message );
}

void Homecorrupter::syncModel()
{
    // forward the protected model values onto the plugin process and related processors

    pluginProcess->setResampleRate( fResampleRate );
    pluginProcess->bitCrusher->setAmount( fBitDepth );
    pluginProcess->setPlaybackRate( fPlaybackRate );

    // note we attenuate the signal at lower bit depths as the dynamic range decreases and volume builds up
    if ( fBitDepth == 1.f ) {
        pluginProcess->bitCrusher->setOutputMix( 1.f );
    } else {
        pluginProcess->bitCrusher->setOutputMix( fBitDepth > .4f ? 1.25f : .25f );
    }

    // oscillators
    pluginProcess->setResampleLfo( fResampleLfo, fResampleLfoDepth );
    pluginProcess->setPlaybackRateLfo( fPlaybackRateLfo, fPlaybackRateLfoDepth );
    pluginProcess->bitCrusher->setLFO( fBitCrushLfo, fBitCrushLfoDepth );

    // output mix
    pluginProcess->setDryMix( fDryMix );
    pluginProcess->setWetMix( fWetMix );
}

}
