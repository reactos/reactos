//
//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "precomp.h"
#include "ccstock.h"
// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "deskcpl"
#define SZ_MODULE           "DESKCPL"
#define DECLARE_DEBUG
#include "debug.h"

// Include the standard helper functions to dump common ADTs
//#include "..\lib\dump.c"


