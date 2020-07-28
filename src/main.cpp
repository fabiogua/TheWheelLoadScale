#include "Arduino.h"
#include "Steroido.h"

#if defined(C_DISPLAY) && defined(C_SCALE)
    #error "Only one Device can be compiled at a time (?!?)"
#endif


#ifdef C_DISPLAY
    #include "Display.h"
#endif


#ifdef C_SCALE
    #include "Scale.h"
#endif