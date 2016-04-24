////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#ifndef __MY_MEM_H__
#define __MY_MEM_H__

#ifdef UDF_DBG
#define MY_HEAP_TRACK_OWNERS
#define MY_HEAP_TRACK_REF
#define MY_HEAP_CHECK_BOUNDS
#define MY_HEAP_CHECK_BOUNDS_SZ   2
#define MY_HEAP_CHECK_BOUNDS_BSZ  (MY_HEAP_CHECK_BOUNDS_SZ*sizeof(ULONG))
#endif //UDF_DBG

//#define MY_HEAP_FORCE_NONPAGED
//#define MY_USE_INTERNAL_MEMMANAGER

//#include "udffs.h"
#include "Include/mem_tools.h"

#endif // __MY_MEM_H__
