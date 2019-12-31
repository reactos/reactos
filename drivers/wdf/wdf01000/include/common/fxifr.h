/*
    This module contains the private IFR tracing definitions
    The structures are common to the In-Flight Recorder (IFR)
    and the IFR-related debug commands in wdfkd.dll.
*/

#ifndef _FXIFR_H
#define _FXIFR_H

#include <initguid.h>

//
// GUID used to tag drivers info in crash dump.
//
// {F87E4A4C-C5A1-4d2f-BFF0-D5DE63A5E4C3}
DEFINE_GUID(WdfDumpGuid2, 
0xf87e4a4c, 0xc5a1, 0x4d2f, 0xbf, 0xf0, 0xd5, 0xde, 0x63, 0xa5, 0xe4, 0xc3);

#define WDF_IFR_LOG_TAG 'gLxF'     // 'FxLg'

#endif //_FXIFR_H