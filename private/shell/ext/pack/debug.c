//
//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "priv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "shellext.ini"
#define SZ_DEBUGSECTION     "packager"
#define SZ_MODULE           "PACKAGER"
#define DECLARE_DEBUG
#include <debug.h>


#undef DebugMsg
#define DebugMsg TraceMsg
                        