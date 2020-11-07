// this is a Node.js script that can conveniently generate a lot of the boilerplate
// code necessary to generate a model for your plugins adjustable parameters, exposing
// it as a public API to DAWs and generating all UI related code

// define your plugins controller model here
// NOTE: all values are floats and while these are usually normalized to the 0 - 1 range, any arbitrary value is supported
//
// format: {
//     name: String,             // used to calculate derived variable names from
//     descr: String,            // description (exposed to user via host)
//     unitDescr: String,        // meaningful description of the unit represented by this value (exposed to user via host)
//     value: {
//         min: String|Number, // minimum value accepted for this parameter (fractional values require .f suffix)
//         max: String|Number, // maximum value accepted for this parameter (fractional values require .f suffix)
//         def: String|Number, // optional, default value for this parameter, falls back to min (fractional values require .f suffix)
//         type: String,       // optional, defaults to float, accepts:
//                             // 'bool' where the value is either 0 or 1 (on/off)
//                             // 'pct' (multiplied by 100)
//     },
//     ui: {               // optional, when defined, will create entry in .uidesc
//         x: Number,      // x, y coordinates and width and height of control
//         y: Number,
//         w: Number,
//         h: Number.
//     },
//     normalizedDescr: Boolean, // optional, whether to display the value in the host normalized (otherwise falls back to 0 - 1 range), defaults to false
//     customDescr: String,      // optional, custom instruction used in controller.cpp to format value
// }
const MODEL = [
    {
        name: 'resampleRate',
        descr: 'Resample rate',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', def: '1.f', type: 'percent' },
        ui: { x: 292, y: 196, w: 134, h: 21 },
        customDescr: 'sprintf( text, "%.2d Hz", ( int ) (( Igorski::VST::SAMPLE_RATE - Igorski::PluginProcess::MIN_SAMPLE_RATE ) * valueNormalized ) + ( int ) Igorski::PluginProcess::MIN_SAMPLE_RATE );'
    },
    {
        name: 'bitDepth',
        descr: 'Resolution',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', def: '1.f', type: 'percent' },
        ui: { x: 292, y: 272, w: 134, h: 21 },
        // note we treat full resolution as 16-bits (but is in fact whatever host is)
        customDescr: 'sprintf( text, "%.d Bits", ( int ) ( 15 * valueNormalized ) + 1 );'
    },
    {
        name: 'playbackRate',
        descr: 'Playback rate',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', def: '1.f', type: 'percent' },
        ui: { x: 292, y: 346, w: 134, h: 21 },
        customDescr: 'sprintf( text, "%.2d %%", ( int ) (( valueNormalized * ( 100.f * Igorski::PluginProcess::MIN_PLAYBACK_SPEED )) + ( Igorski::PluginProcess::MIN_PLAYBACK_SPEED * 100 )));'
    },
    // LFO (note constants for min/max defined in global.h)
    {
        name: 'resampleLfo',
        descr: 'Resampling LFO',
        unitDescr: 'Hz',
        value: { min: '0.f', max: '10.f' },
        ui: { x: 516, y: 196, w: 134, h: 21 },
        normalizedDescr: true
    },
    {
        name: 'resampleLfoDepth',
        descr: 'Resampling LFO depth',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', type: 'percent' },
        ui: { x: 516, y: 227, w: 134, h: 21 }
    },
    {
        name: 'bitCrushLfo',
        descr: 'Bit crush LFO',
        unitDescr: 'Hz',
        value: { min: '0.f', max: '10.f' },
        ui: { x: 516, y: 272, w: 134, h: 21 },
        normalizedDescr: true
    },
    {
        name: 'bitCrushLfoDepth',
        descr: 'Bit crush LFO depth',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', type: 'percent' },
        ui: { x: 516, y: 300, w: 134, h: 21 }
    },
    {
        name: 'playbackRateLfo',
        descr: 'Playback LFO',
        unitDescr: 'Hz',
        value: { min: '0.f', max: '10.f' },
        ui: { x: 516, y: 346, w: 134, h: 21 },
        normalizedDescr: true
    },
    {
        name: 'playbackRateLfoDepth',
        descr: 'Playback LFO depth',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', type: 'percent' },
        ui: { x: 516, y: 376, w: 134, h: 21 }
    },
    {
        name: 'wetMix',
        descr: 'Wet mix',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', def: '1.f', type: 'percent' },
        ui: { x: 292, y: 427, w: 134, h: 21 }
    },
    {
        name: 'dryMix',
        descr: 'Dry mix',
        unitDescr: '%',
        value: { min: '0.f', max: '1.f', type: 'percent' },
        ui: { x: 516, y: 427, w: 134, h: 21 }
    }
];

// DO NOT CHANGE BELOW

const fs = require( 'fs' );

const SOURCE_FOLDER      = './src';
const RESOURCE_FOLDER    = './resource';
const START_OF_OUTPUT_ID = '// --- AUTO-GENERATED START';
const END_OF_OUTPUT_ID   = '// --- AUTO-GENERATED END';

function replaceContent( fileData, lineContent, startId = START_OF_OUTPUT_ID, endId = END_OF_OUTPUT_ID ) {
    // first generate the appropriate regular expression
    // result with default values should equal /(\/\/ AUTO-GENERATED START)([\s\S]*?)(\/\/ AUTO-GENERATED END)/g
    const regex = new RegExp( `(${startId.replace(/\//g, '\\/')})([\\s\\S]*?)(${endId.replace(/\//g, '\\/')})`, 'g' );

    // find the currently existing data which will be replaced
    const dataToReplace = fileData.match( regex );
    if ( !dataToReplace ) {
        return fileData;
    }
    // format the output as a String
    const output = `${startId}
${lineContent.join( '\n' )}\n
${endId}`;

    return fileData.replace( dataToReplace, output );
}

function generateNamesForParam({ name }) {
    const pascalCased = `${name.charAt(0).toUpperCase()}${name.slice(1)}`;
    // the model name
    const model = `f${pascalCased}`;
    // param reflects the model name
    const param = `${name}Param`;
    // paramId is used to identify the UI controls linked to the model
    const paramId = `k${pascalCased}Id`;
    // saved is used to describe a temporary variable to retrieve saved states
    const saved = `saved${pascalCased}`;
    // toSave is used to describe a temporary variable used to save a state
    const toSave = `toSave${pascalCased}`;

    return {
        model,
        param,
        paramId,
        saved,
        toSave
    }
}

function generateParamIds() {
    const outputFile    = `${SOURCE_FOLDER}/paramids.h`;
    const fileData      = fs.readFileSync( outputFile, { encoding:'utf8', flag:'r' });
    const lines = [];

    MODEL.forEach(( entry, index ) => {
        const { descr } = entry;
        const { paramId } = generateNamesForParam( entry );
        const line = `    ${paramId} = ${index},    // ${descr}`;
        lines.push( line );
    });
    fs.writeFileSync( outputFile, replaceContent( fileData, lines ));
}

function generateVstHeader() {
    const outputFile    = `${SOURCE_FOLDER}/vst.h`;
    const fileData      = fs.readFileSync( outputFile, { encoding:'utf8', flag:'r' });
    const lines = [];

    MODEL.forEach( entry => {
        const { descr, value } = entry;
        const { model } = generateNamesForParam( entry );
        const line = `        float ${model} = ${value.def ?? value.min};    // ${descr}`;
        lines.push( line );
    });
    fs.writeFileSync( outputFile, replaceContent( fileData, lines ));
}

function generateVstImpl() {
    const outputFile = `${SOURCE_FOLDER}/vst.cpp`;
    let fileData     = fs.readFileSync( outputFile, { encoding:'utf8', flag:'r' });

    const processLines = [];
    const setStateLines = [];
    const setStateSwapLines = [];
    const setStateApplyLines = [];
    const getStateLines = [];
    const getStateSwapLines = [];
    const getStateApplyLines = [];

    MODEL.forEach( entry => {
        const { model, paramId, saved, toSave } = generateNamesForParam( entry );

        // 1. Homecorrupter::process
        processLines.push(`
                    case ${paramId}:
                        if ( paramQueue->getPoint( numPoints - 1, sampleOffset, value ) == kResultTrue )
                            ${model} = ( float ) value;
                        break;`);

        // 2. Homecorrupter::setState

        setStateLines.push(`
    float ${saved} = 0.f;
    if ( state->read( &${saved}, sizeof ( float )) != kResultOk )
        return kResultFalse;`);

        setStateSwapLines.push(`   SWAP_32( ${saved} )`);   // byte swap
        setStateApplyLines.push(`    ${model} = ${saved};`); // assignment to model

        // 3. Homecorrupter::getState

        getStateLines.push(`    float ${toSave} = ${model};`);
        getStateSwapLines.push(`   SWAP_32( ${toSave} )`);   // byte swap
        getStateApplyLines.push(`    state->write( &${toSave}, sizeof( float ));` );
    });

    let startId = '// --- AUTO-GENERATED PROCESS START';
    let endId   = '// --- AUTO-GENERATED PROCESS END';
    fileData = replaceContent( fileData, processLines, startId, endId );

    startId = '// --- AUTO-GENERATED SETSTATE START';
    endId   = '// --- AUTO-GENERATED SETSTATE END';
    fileData = replaceContent( fileData, setStateLines, startId, endId );

    startId = '// --- AUTO-GENERATED SETSTATE SWAP START';
    endId   = '// --- AUTO-GENERATED SETSTATE SWAP END';
    fileData = replaceContent( fileData, setStateSwapLines, startId, endId );

    startId = '// --- AUTO-GENERATED SETSTATE APPLY START';
    endId   = '// --- AUTO-GENERATED SETSTATE APPLY END';
    fileData = replaceContent( fileData, setStateApplyLines, startId, endId );

    startId = '// --- AUTO-GENERATED GETSTATE START';
    endId   = '// --- AUTO-GENERATED GETSTATE END';
    fileData = replaceContent( fileData, getStateLines, startId, endId );

    startId = '// --- AUTO-GENERATED GETSTATE SWAP START';
    endId   = '// --- AUTO-GENERATED GETSTATE SWAP END';
    fileData = replaceContent( fileData, getStateSwapLines, startId, endId );

    startId = '// --- AUTO-GENERATED GETSTATE APPLY START';
    endId   = '// --- AUTO-GENERATED GETSTATE APPLY END';
    fileData = replaceContent( fileData, getStateApplyLines, startId, endId );

    fs.writeFileSync( outputFile, fileData );
}

function generateController() {
    const outputFile = `${SOURCE_FOLDER}/ui/controller.cpp`;
    let fileData     = fs.readFileSync( outputFile, { encoding:'utf8', flag:'r' });

    const initLines = [];
    const setStateLines = [];
    const setStateSwapLines = [];
    const setStateSetParamLines = [];
    const getParamLines = [];
    let line;

    MODEL.forEach( entry => {
        const { param, paramId, saved } = generateNamesForParam( entry );
        const { descr, unitDescr, normalizedDescr, customDescr } = entry;

        let { min, max, def, type } = entry.value;
        if ( !def ) {
            def = min;
        }

        // 1. PluginController::initialize

        if ( type === 'bool' ) {
            line = `
    parameters.addParameter(
        USTRING( "${descr}" ), ${min}, ${max}, ${def}, ParameterInfo::kCanAutomate, ${paramId}, unitId
    );`;
        } else {
            line = `
    RangeParameter* ${param} = new RangeParameter(
        USTRING( "${descr}" ), ${paramId}, USTRING( "${unitDescr}" ),
        ${min}, ${max}, ${def},
        0, ParameterInfo::kCanAutomate, unitId
    );
    parameters.addParameter( ${param} );`;
        }
        initLines.push( line );

        // 2. PluginController::setComponentState

        setStateLines.push(`
        float ${saved} = 1.f;
        if ( state->read( &${saved}, sizeof( float )) != kResultOk )
            return kResultFalse;`);

        // endian swap
        setStateSwapLines.push(`    SWAP_32( ${saved} )`);

        // set param normalized
        setStateSetParamLines.push(`        setParamNormalized( ${paramId}, ${saved} );`);

        // 3. PluginController::getParamStringByValue
        // TODO: we can optimize this by grouping case values

        line = `
        case ${paramId}:`;

        if ( customDescr ) {
           line += `
            ${customDescr}`;
        } else if ( type === 'bool' ) {
            line += `
            sprintf( text, "%s", ( valueNormalized == 0 ) ? "Off" : "On" );`;
        } else if ( type === 'percent' ) {
            line += `
            sprintf( text, "%.2d %%", ( int ) ( valueNormalized * 100.f ));`;
        } else if ( normalizedDescr ) {
            line += `
            sprintf( text, "%.2f ${unitDescr}", normalizedParamToPlain( tag, valueNormalized ));`;
        } else {
            line += `
            sprintf( text, "%.2f", ( float ) valueNormalized );`;
        }

        line += `
            Steinberg::UString( string, 128 ).fromAscii( text );
            return kResultTrue;`;

        getParamLines.push( line );
    });
    fileData = replaceContent( fileData, initLines );

    let startId = '// --- AUTO-GENERATED SETSTATE START';
    let endId   = '// --- AUTO-GENERATED SETSTATE END';
    fileData = replaceContent( fileData, setStateLines, startId, endId );

    startId = '// --- AUTO-GENERATED SETSTATE SWAP START';
    endId   = '// --- AUTO-GENERATED SETSTATE SWAP END';
    fileData = replaceContent( fileData, setStateSwapLines, startId, endId );

    startId = '// --- AUTO-GENERATED SETSTATE SETPARAM START';
    endId   = '// --- AUTO-GENERATED SETSTATE SETPARAM END';
    fileData = replaceContent( fileData, setStateSetParamLines, startId, endId );

    startId = '// --- AUTO-GENERATED GETPARAM START';
    endId   = '// --- AUTO-GENERATED GETPARAM END';
    fileData = replaceContent( fileData, getParamLines, startId, endId );

    fs.writeFileSync( outputFile, fileData );
}

function generateUI() {
    const outputFile = `${RESOURCE_FOLDER}/plugin.uidesc`;
    let fileData     = fs.readFileSync( outputFile, { encoding:'utf8', flag:'r' });

    const controlLines = [];
    const tagLines     = [];

    MODEL.filter(entry => !!entry.ui)
         .forEach(( entry, index ) =>
    {
        const { x, y, w, h } = entry.ui;
        const { descr }      = entry;
        let { min, max, def, type } = entry.value;
        if ( !def ) {
            def = min;
        }

        const { param } = generateNamesForParam( entry );
        let control;

        if ( type === 'bool' ) {
            control = `<view control-tag="Unit1::${param}" class="CCheckBox" origin="${x}, ${y}" size="${w}, ${h}"
              max-value="${max}" min-value="${min}" default-value="${def}"
              background-offset="0, 0" boxfill-color="~ GreenCColor" autosize="bottom"
              boxframe-color="~ BlackCColor" checkmark-color="~ BlackCColor"
              draw-crossbox="true" font="~ NormalFontSmall" font-color="Light Grey"
              autosize-to-fit="false" frame-width="1"
              mouse-enabled="true" opacity="1" round-rect-radius="0"
              title="${descr}" transparent="false" wants-focus="true" wheel-inc-value="0.1"
        />`;
        } else {
            control = `<view control-tag="Unit1::${param}" class="CSlider" origin="${x}, ${y}" size="${w}, ${h}"
              max-value="${max}" min-value="${min}" default-value="${def}"
              background-offset="0, 0" bitmap="slider_background"
              bitmap-offset="0, 0" draw-back="false" draw-back-color="~ WhiteCColor" draw-frame="false"
              draw-frame-color="~ WhiteCColor" draw-value="false" draw-value-color="~ WhiteCColor" draw-value-from-center="false"
              draw-value-inverted="false" handle-bitmap="slider_handle" handle-offset="0, 0"
              mode="free click" mouse-enabled="true" opacity="1" orientation="horizontal" reverse-orientation="false"
              transparent="true" transparent-handle="true" wheel-inc-value="0.1" zoom-factor="10"
        />`;
        }
        controlLines.push(`
        <!-- ${descr} -->
        ${control}` );

        tagLines.push(`       <control-tag name="Unit1::${param}" tag="${index}" />` );
    });

    let startId = '<!-- AUTO-GENERATED CONTROLS START -->';
    let endId   = '<!-- AUTO-GENERATED CONTROLS END -->';
    fileData = replaceContent( fileData, controlLines, startId, endId );

    startId = '<!-- AUTO-GENERATED TAGS START -->';
    endId   = '<!-- AUTO-GENERATED TAGS END -->';
    fileData = replaceContent( fileData, tagLines, startId, endId );

    fs.writeFileSync( outputFile, fileData );
}

(function execute() {
    try {
        generateParamIds();
        generateVstHeader();
        generateVstImpl();
        generateController();
        generateUI();

        console.log( 'Successfully generated the plugin model.' );
    } catch ( e ) {
        console.error( 'Something went horribly wrong when generate the model.', e );
    }
})();
