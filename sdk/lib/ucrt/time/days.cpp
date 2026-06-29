//
// days.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The global _days and _lpdays arrays, which define the number of days that
// contain the number of days from the beginning of the year to the beginning
// of each month.
//
#include <corecrt_internal_time.h>



extern "C" int const _days[] =
{
    -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};



extern "C" int const _lpdays[] =
{
    -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};


