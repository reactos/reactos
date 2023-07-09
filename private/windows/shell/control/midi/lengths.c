
#include <windows.h>
#include "hack.h"

BYTE _based(_segname("_CODE")) bMidiLengths[] = {
    3,  // STATUS_NOTEOFF
    3,  // STATUS_NOTEON
    3,  // STATUS_POLYPHONICKEY
    3,  // STATUS_CONTROLCHANGE
    2,  // STATUS_PROGRAMCHANGE
    2,  // STATUS_CHANNELPRESSURE
    3,  // STATUS_PITCHBEND
};

BYTE _based(_segname("_CODE")) bSysLengths[] = {
    1,  // STATUS_SYSEX
    2,  // STATUS_QFRAME
    3,  // STATUS_SONGPOINTER
    2,  // STATUS_SONGSELECT
    1,  // STATUS_F4
    1,  // STATUS_F5
    1,  // STATUS_TUNEREQUEST
    1,  // STATUS_EOX
};
