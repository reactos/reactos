// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Used to assert that an object is entered only once at any given time. 
//
//      The simplest use of this class is simply to inherit and use AssertEntry
//      on every entry point at the outermost scope.
//
//      Class is defined for both checked and retail builds to allow mixing of
//      build types when a class' implementation may be defined in multiple
//      modules as is the case with CWGXBitmap.
//
//------------------------------------------------------------------------------

#pragma once

class CAssertEntry
{
public:
    CAssertEntry();

#if DBG

    void Enter();
    void Leave();

    ~CAssertEntry();

#endif

private:
    
    union EntryState {
        struct {
            ULONG cEntries;
            DWORD dwThreadId;
        };
        LONG64 l64;
    };

#if DBG

    void ForceSetEntryStatus(
        __inout_ecount(1) EntryState *pNewStatus,
        __out_ecount(1) EntryState *pCurStatus,
        INT cEntryIncrement
        );

#endif

    volatile EntryState m_oEntryStatus;
};


#if DBG

// AssertEntry's check for single entry of object for the current
// code block's scope.  The CAssertEntry object is entered at
// the use of AssertEntry and left and the end of the scope.
#define AssertEntry(oCAE)  CGuard<CAssertEntry> oEntryCheck(oCAE)

#else

#define AssertEntry(oCAE)

#endif

