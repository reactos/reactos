// We don't use the debugging macros ourselves, but we use
// RunInstallUninstallStubs2, which uses IsOS() in stocklib,
// and stocklib uses the debugging macros, so we have to do all this
// crap to keep the linker happy.
//

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

#include <windows.h>
#include <ccstock.h>

// Define some things for debug.h
//
#define SZ_DEBUGINI     "shellext.ini"
#define SZ_DEBUGSECTION "userstub"
#define SZ_MODULE       "USERSTUB"
#define DECLARE_DEBUG
#include <debug.h>
