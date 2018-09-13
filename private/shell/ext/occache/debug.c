//
//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "init.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "shellext.ini"
#define SZ_DEBUGSECTION     "occache"
#define SZ_MODULE           "OCCACHE"
#define DECLARE_DEBUG
#include <debug.h>

