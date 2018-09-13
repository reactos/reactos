//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include <windows.h>
#include <ccstock.h>

// Define some things for debug.h
//
#define SZ_DEBUGINI     "shellext.ini"
#define SZ_DEBUGSECTION "webcheck"
#define SZ_MODULE       "WEBCHECK"
#define DECLARE_DEBUG
#include <debug.h>


