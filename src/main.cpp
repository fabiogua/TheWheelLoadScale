#include "Arduino.h"
#include "Steroido.h"

#define SW_VERSION "V0.0.1"

#if defined(C_DISPLAY) && defined(C_SCALE)
    #error "Only one Device can be compiled at a time (?!?)"
#endif


#ifdef C_DISPLAY
    #include "Display.h"
#endif


#ifdef C_SCALE
    #include "Scale.h"
#endif