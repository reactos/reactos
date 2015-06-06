////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
 Module Name: Physical.cpp

 Execution: Kernel mode only

 Description:

   Contains code that implement read/write operations for physical device
*/

#include            "udf.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID        UDF_FILE_PHYSICAL

#include "Include/phys_lib.cpp"

