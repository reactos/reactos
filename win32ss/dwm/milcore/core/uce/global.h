// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------------
//

//
//  Abstract:
//    Global composition engine functionality/data structures go into this file.
//
//---------------------------------------------------------------------------------

#pragma once

class CMediaControl;

extern CCriticalSection g_csCompositionEngine;
extern CMediaControl* g_pMediaControl;

extern bool s_fRDP;
extern bool s_fRecordUCE;
extern bool s_fTRPC;
extern bool s_fTRPCOverride;

HRESULT UpdateSchedulerSettings(
    int nPriority
    );

HRESULT EnsurePartitionManager(
    int nPriority
    );

void ReleasePartitionManager();

HRESULT GetCompositionEngineComposedEventId(
    __out_ecount(1) UINT *pcEventId
    );



