//
// tmpfile_initializer.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// CRT initializers and terminators have been extracted from the main CRT sources
// to enable the CRT DLL to be built with LTCG enabled.  The source files in which
// the CRT initializers and terminators are defined cannot be compiled as /GL
// because the compiler will optimize them away during link-time code generation.
// We inhibit this optimization by defining the initializers and terminators in
// separate source files that are not compiled with /GL.
//
#include <corecrt_internal.h>

extern "C" void __cdecl __acrt_uninitialize_tmpfile();

extern "C" _CRTALLOC(".CRT$XPX") _PVFV const __acrt_tmpfile_terminator = __acrt_uninitialize_tmpfile;
