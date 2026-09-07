// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Name: utc.cpp
//
//  Description:  UTC time utilities
//
//------------------------------------------------------------------------
#include "precomp.hpp"
//+-----------------------------------------------------------------------
//
//  Member: ConvertTime 
//
//  Synopsis: Converts a time between units
//
//  Returns:  the converted time
//
//------------------------------------------------------------------------
ULONGLONG ConvertTime(
    ULONGLONG tCurrent,
     __in_range(>,0) ULONGLONG unitsCurrent,
     __in_range(>,0) ULONGLONG unitsNew
    )
{
    ULONGLONG tNew;
    ULONGLONG cSeconds;
    ULONGLONG cRemainder;

    cSeconds = tCurrent / unitsCurrent;
    cRemainder = tCurrent - (cSeconds *  unitsCurrent);
    tNew = cSeconds * unitsNew;
    tNew += cRemainder * unitsNew / unitsCurrent;
    
    return tNew;
}

