
// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include "priv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "MSIEFTP"
#define SZ_MODULE           "MSIEFTP"
#define DECLARE_DEBUG

#include <ccstock.h>
#include <debug.h>
