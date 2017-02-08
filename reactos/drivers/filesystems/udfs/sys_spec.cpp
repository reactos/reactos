////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*************************************************************************
*
* File: Sys_Spec.cpp
*
* Module: UDF File System Driver 
* (both User and Kernel mode execution)
*
* Description:
*   Contains system-secific code 
*
*************************************************************************/


#include "udffs.h"
// define the file specific bug-check id
#define         UDF_BUG_CHECK_ID        UDF_FILE_SYS_SPEC

#include "Include/Sys_spec_lib.cpp"

//#include "Include/tools.cpp"
