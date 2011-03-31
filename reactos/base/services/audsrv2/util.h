//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    util.h
//
// Abstract:    
//      This is the header file for general helper functions
//

#pragma once

//
// Get the size for a WAVEFORMATEX-based structure.  It properly handles extended
// structures (such as WAVEFORMATEXTENSIBLE), and WAVE_FORMAT_PCM structures with bogus sizes.
//
DWORD GetWfxSize(const WAVEFORMATEX* pwfxSrc);


// Debug Trace Macro's

#define TRACE_LOWEST (6)
#define TRACE_LOW (5)
#define TRACE_NORMAL (4)
#define TRACE_HIGH (3)
#define TRACE_HIGHEST (2)
#define TRACE_ERROR (1)
#define TRACE_MUTED (0)
#define TRACE_ALWAYS (-1)

#ifdef __FUNCTION__
#define TRACE_ENTER() DebugPrintf(TRACE_LOWEST, TEXT("%s Enter"), __FUNCTION__ "()")
#define TRACE_LEAVE()  DebugPrintf(TRACE_LOWEST, TEXT("%s Leave"), __FUNCTION__ "()")
#define TRACE_LEAVE_HRESULT(hr) DebugPrintf(TRACE_LOWEST, TEXT("%s Leave, returning %#08x"), __FUNCTION__,hr)
#else
#define TRACE_ENTER() 
#define TRACE_LEAVE() 
#define TRACE_LEAVE_HRESULT(hr) DebugPrintf(TRACE_LOWEST, TEXT("%s, line %s, returning %#08x"), __FILE__,__LINE__,hr)
#endif

// test tone generation
BOOL 
GeneratePCMTestTone
( 
    LPVOID          pDataBuffer, 
    DWORD           cbDataBuffer,
    UINT            nSamplesPerSec,
    UINT            nChannels,
    WORD            wBitsPerSample,
    double          dFreq,
    double          dAmpFactor=1.0
);

//The higher the numer, the more the debug output
// Range is from 0 to 6.
extern LONG g_lDebugLevel;
void DebugPrintf(LONG lDebugLevel, LPCTSTR pszFormat, ... );

