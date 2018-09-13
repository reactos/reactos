#include <windows.h>

//=============================================================================
//
// This file defines the MULTIMON stub module that fakes multiple monitor 
// apis on pre Memphis Win32 OSes
// this is the only file that defines COMPILE_MULTIMON_STUBS 
//=============================================================================

#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
