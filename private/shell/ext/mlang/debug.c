//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "private.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI     "shellext.ini"
#define SZ_DEBUGSECTION "mlang"
#define SZ_MODULE       "MLANG"
#define DECLARE_DEBUG
#include <debug.h>
