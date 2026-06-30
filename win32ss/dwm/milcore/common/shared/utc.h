// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:  Universal time definitions
//
//------------------------------------------------------------------------

#pragma once

#define UTC_UNITS_PER_MILLISECOND static_cast<UTC_TIME>(10000L)
#define UTC_UNITS_PER_SECOND (UTC_UNITS_PER_MILLISECOND * 1000)
#define UTC_UNITS_PER_MINUTE (UTC_UNITS_PER_SECOND * 60)
#define UTC_UNITS_PER_HOUR   (UTC_UNITS_PER_MINUTE * 60)
typedef ULONGLONG UTC_TIME;

ULONGLONG ConvertTime(
    ULONGLONG tCurrent,
     __in_range(>,0) ULONGLONG unitsCurrent,
     __in_range(>,0) ULONGLONG unitsNew
    );

