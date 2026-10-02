// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definitions of Just-In-Time code generator exposed routines.
//
//-----------------------------------------------------------------------------

#pragma once

//+-----------------------------------------------------------------------------
//
//  Class:
//      CJitterAccess
//
//  Synopsis:
//      Exposes Just-In-Time code generator for client.
//
//------------------------------------------------------------------------------
class CJitterAccess
{
public:
    static __checkReturn HRESULT Enter(UINT16 usCallParametersSize);
    static void Leave();

    static UINT8* AllocFlushMemory(UINT32 cbSize);
    static __checkReturn HRESULT Compile(__deref_out UINT8 **ppBinaryCode);
    static UINT32 GetCodeSize();
    static void CodeFree(__in void *pBinaryCode);

    static void SplitFlow();
    static void SetFlow(UINT32 uFlowID);
    static void ReverseFlow(UINT32 uFlowID);
    static void MergeFlow();

    static void SetClientData(void *pClientData);
    static void* GetClientData();

#if DBG_DUMP
    static void SetDumpFile(WarpPlatform::FileHandle hDumpFile);
#endif //DBG_DUMP

    static void SetMode(UINT32 uParameterIdx, INT32 nParameterValue);
    // supported values of uParameterIdx in SetMode:
    static const int sc_uidAllowEBP = 0;
    static const int sc_uidEnableShuffling = 1;
    static const int sc_uidEnableTotalBubbling = 2;
    static const int sc_uidUseNegativeStackOffsets = 3;
    static const int sc_uidUseSSE41 = 4;
    static const int sc_uidAvoidMOVDs = 5;
    static const int sc_uidEnableMemShuffling = 6;
};


