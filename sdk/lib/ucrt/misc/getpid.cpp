//
// getpid.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _getpid() function, which gets the current process identifier.
//
#include <corecrt_internal.h>



// Gets the current process identifier.
extern "C" int __cdecl _getpid()
{
    return GetCurrentProcessId();
}
