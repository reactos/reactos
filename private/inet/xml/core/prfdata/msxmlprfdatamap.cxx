/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/******************************************************************************
Module name: HWInputPrfDataMap.cpp
Notices: Written 1998 by Jeffrey Richter
Description: Performance object and counter map
******************************************************************************/

#include "core.hxx"
#pragma hdrstop

#ifdef PRFDATA

#include <windows.h>
#include "prfdata.hxx"
#include "msxmlprfdatamap.hxx"


///////////////////////////////////////////////////////////////////////////////

PRFDATA_MAP_BEGIN()
   PRFDATA_MAP_OBJ(PRFOBJ_MSXML, L"MSXML", 
      L"The MSXML perf counters ",
      PERF_DETAIL_NOVICE, PRFCTR_GCCOUNT, 5, 20)

   PRFDATA_MAP_CTR(PRFCTR_GCCOUNT,  L"GC",  
      L"Counts GC activity",  
      PERF_DETAIL_NOVICE, 0, PERF_COUNTER_RAWCOUNT)

   PRFDATA_MAP_CTR(PRFCTR_OBJECTS,  L"Objects",  
      L"Counts live GC'able objects",  
      PERF_DETAIL_NOVICE, 0, PERF_COUNTER_RAWCOUNT)

   PRFDATA_MAP_CTR(PRFCTR_PAGES,  L"Pages",  
      L"Counts committed virtual memory",  
      PERF_DETAIL_NOVICE, 0, PERF_COUNTER_RAWCOUNT)

   PRFDATA_MAP_CTR(PRFCTR_HEAPSIZE,  L"Heap",  
      L"Counts heap size",  
      PERF_DETAIL_NOVICE, 0, PERF_COUNTER_RAWCOUNT)

   PRFDATA_MAP_CTR(PRFCTR_OOM,  L"OutOfMemory",  
      L"Counts out of memory exceptions",  
      PERF_DETAIL_NOVICE, 0, PERF_COUNTER_RAWCOUNT)

PRFDATA_MAP_END("MSXML")


///////////////////////////////// End Of File /////////////////////////////////

#endif
