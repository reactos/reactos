/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/******************************************************************************
Module name: MSXMLPrfDataMap.h
Description: Definition of performance objects and counters
******************************************************************************/
#ifndef MSXMLPRFDATAMAP_H
#define MSXMLPRFDATAMAP_H

#ifdef PRFDATA

#include "prfdata.hxx"


///////////////////////////////////////////////////////////////////////////////


PRFDATA_DEFINE_OBJECT(PRFOBJ_MSXML,              100);
PRFDATA_DEFINE_COUNTER(PRFCTR_GCCOUNT,            101);
PRFDATA_DEFINE_COUNTER(PRFCTR_OBJECTS,            102);
PRFDATA_DEFINE_COUNTER(PRFCTR_PAGES,              103);
PRFDATA_DEFINE_COUNTER(PRFCTR_HEAPSIZE,           104);
PRFDATA_DEFINE_COUNTER(PRFCTR_OOM,                105);

extern bool g_PerfTrace;
extern CPrfData::INSTID g_GCCountPerfInstId;
extern CPrfData::INSTID g_ObjectPerfInstId;
extern CPrfData::INSTID g_PagesPerfInstId;
extern CPrfData::INSTID g_HeapSizePerfInstId;
extern CPrfData::INSTID g_OutOfMemoryInstId;
///////////////////////////////// End Of File /////////////////////////////////

#endif

#endif
