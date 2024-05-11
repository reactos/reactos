//
// labs.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines labs(), which computes the absolute value of a number.
//
#include <stdlib.h>

extern "C" {



#pragma function(labs)



long __cdecl labs(long const number)
{
	return number >= 0 ? number : -number;
}



} // extern "C"
