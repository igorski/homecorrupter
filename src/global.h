#ifndef __GLOBAL_HEADER__
#define __GLOBAL_HEADER__

#include "pluginterfaces/base/fplatform.h"
#include "pluginterfaces/base/funknown.h"
#include <math.h>

using namespace Steinberg;

namespace Igorski {
namespace VST {

    static const int   ID       = 97151820;
    static const char* NAME     = "Homecorrupter";
    static const char* VENDOR   = "igorski.nl";

    // generate unique UIDs for these (www.uuidgenerator.net is great for this)

    static const FUID PluginProcessorUID( 0xC0AFA4D6, 0x749F464F, 0xB499A21C, 0x0E48FFA8 );
    static const FUID PluginWithSideChainProcessorUID( 0x749F464F, 0xB499A21C, 0x0E48FFA8, 0xC0AFA4D6 );
    static const FUID PluginControllerUID( 0xB499A21C, 0x0E48FFA8, 0xC0AFA4D6, 0x749F464F );

    extern float SAMPLE_RATE; // set upon initialization, see vst.cpp

    static const float PI       = 3.141592653589793f;
    static const float TWO_PI   = PI * 2.f;
    static const float SQRT_TWO = sqrt( 2 );

    // maximum and minimum rate of oscillation in Hz
    // also see plugin.uidesc to update the controls to match

    static const float MAX_LFO_RATE() { return 10.f; }
    static const float MIN_LFO_RATE() { return .1f; }

    // sine waveform used for the oscillator
    static const float TABLE[ 128 ] = {
        0f, 0.0490677f, 0.0980171f, 0.14673f, 0.19509f, 0.24298f, 0.290285f, 0.33689f, 0.382683f, 0.427555f, 0.471397f, 0.514103f, 0.55557f, 0.595699f, 0.634393f, 0.671559f, 0.707107f, 0.740951f, 0.77301f, 0.803208f, 0.83147f, 0.857729f, 0.881921f, 0.903989f, 0.92388f, 0.941544f, 0.95694f, 0.970031f, 0.980785f, 0.989177f, 0.995185f, 0.998795f, 1f, 0.998795f, 0.995185f, 0.989177f, 0.980785f, 0.970031f, 0.95694f, 0.941544f, 0.92388f, 0.903989f, 0.881921f, 0.857729f, 0.83147f, 0.803208f, 0.77301f, 0.740951f, 0.707107f, 0.671559f, 0.634393f, 0.595699f, 0.55557f, 0.514103f, 0.471397f, 0.427555f, 0.382683f, 0.33689f, 0.290285f, 0.24298f, 0.19509f, 0.14673f, 0.0980171f, 0.0490677f, 1.22465e-16f, -0.0490677f, -0.0980171f, -0.14673f, -0.19509f, -0.24298f, -0.290285f, -0.33689f, -0.382683f, -0.427555f, -0.471397f, -0.514103f, -0.55557f, -0.595699f, -0.634393f, -0.671559f, -0.707107f, -0.740951f, -0.77301f, -0.803208f, -0.83147f, -0.857729f, -0.881921f, -0.903989f, -0.92388f, -0.941544f, -0.95694f, -0.970031f, -0.980785f, -0.989177f, -0.995185f, -0.998795f, -1f, -0.998795f, -0.995185f, -0.989177f, -0.980785f, -0.970031f, -0.95694f, -0.941544f, -0.92388f, -0.903989f, -0.881921f, -0.857729f, -0.83147f, -0.803208f, -0.77301f, -0.740951f, -0.707107f, -0.671559f, -0.634393f, -0.595699f, -0.55557f, -0.514103f, -0.471397f, -0.427555f, -0.382683f, -0.33689f, -0.290285f, -0.24298f, -0.19509f, -0.14673f, -0.0980171f, -0.0490677
    };
}
}

#endif
