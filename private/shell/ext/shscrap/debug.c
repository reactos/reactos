//
//
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include <windows.h>
#include <ccstock.h>

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "shscrap"
#define SZ_MODULE           "SHSCRAP"
#define DECLARE_DEBUG
#include <debug.h>
