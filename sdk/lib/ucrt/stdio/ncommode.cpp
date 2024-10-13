//
// ncommode.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines _commode, which sets the default file commit mode to nocommit.
//
#include <corecrt_internal_stdio.h>



// Set default file commit mode to nocommit
extern "C" { int _commode = 0; }



extern "C" int* __cdecl __p__commode()
{
    return &_commode;
}
