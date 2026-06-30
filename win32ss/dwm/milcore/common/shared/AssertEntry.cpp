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

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  CAssertEntry::CAssertEntry
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CAssertEntry::CAssertEntry()
{
    m_oEntryStatus.cEntries = 0;
    m_oEntryStatus.dwThreadId = 0;
}


#if DBG

// REENTRY_LIMIT is the maximum number of times a single object
// may be entered before Asserting.
#define REENTRY_LIMIT   256

//+------------------------------------------------------------------------
//
//  Function:  CAssertEntry::Enter
//
//  Synopsis:  This method should be called at the beginning of every
//             object's thread section that needs to check that it is
//             indeed called on a single thread at a time.
//
//             A subsequent call to Leave is required.
//
//-------------------------------------------------------------------------
void 
CAssertEntry::Enter()
{
    EntryState oOrgStatus, oNewStatus, oCurStatus;

    oOrgStatus.l64 = m_oEntryStatus.l64;
    oNewStatus.cEntries = oOrgStatus.cEntries + 1;
    oNewStatus.dwThreadId = GetCurrentThreadId();

    //
    // Modify the entry status as early as possible so that if another
    // thread has already entered this object, that thread will assert
    // when it tries to leave.  There is a very slim window in which the
    // second thread could completely leave and change the status such
    // that only this thread will assert.  However, even in this case
    // the second thread's ID will be available in oCurStatus.dwThreadId.
    //
    // Also note that critical sections are not used because they allow
    // a greater chance of missing the double entry.
    //
    
    oCurStatus.l64 = MILInterlockedCompareExchange64(
        &m_oEntryStatus.l64,
        oNewStatus.l64,
        oOrgStatus.l64
        );

    if (oCurStatus.l64 != oOrgStatus.l64)
    {
        ForceSetEntryStatus(&oNewStatus, &oCurStatus, +1);
    }
    
    AssertConstMsgW(oOrgStatus.cEntries < REENTRY_LIMIT,
              L"Calls to Enter exceed re-entry limit.");

    AssertConstMsgW(
        (oCurStatus.l64 == oOrgStatus.l64) &&
        ((oOrgStatus.cEntries == 0) ||
         (oOrgStatus.dwThreadId == oNewStatus.dwThreadId)),
        L"Single threaded method(s) called on multiple threads.\r\n"
        L"Second thread will likely also assert.  If this is a pop-up "
        L"dialog then there is probably a second dialog with a similar"
        L"error message."
        );
}

//+------------------------------------------------------------------------
//
//  Function:  CAssertEntry::Leave
//
//  Synopsis:  This method should be called at the end of every object's
//             thread section that needs to check that it is indeed
//             called on a single thread at a time.
//
//             A prior call to Enter is required.
//
//-------------------------------------------------------------------------
void 
CAssertEntry::Leave()
{
    EntryState oOrgStatus, oNewStatus, oCurStatus;

    oOrgStatus.l64 = m_oEntryStatus.l64;
    oNewStatus.cEntries = oOrgStatus.cEntries - 1;
    oNewStatus.dwThreadId = GetCurrentThreadId();

    //
    // Modify the entry status as late as possible so that if another
    // thread is entering this object, that thread will have a larger
    // window to hit.
    //

    if (oOrgStatus.cEntries != 0)
    {
        oCurStatus.l64 = MILInterlockedCompareExchange64(
            &m_oEntryStatus.l64,
            oNewStatus.l64,
            oOrgStatus.l64
            );

        if (oOrgStatus.l64 != oCurStatus.l64)
        {
            ForceSetEntryStatus(&oNewStatus, &oCurStatus, -1);
        }

        AssertConstMsgW(
            (oCurStatus.l64 == oOrgStatus.l64) &&
            (oOrgStatus.dwThreadId == oNewStatus.dwThreadId),
            L"Single threaded method(s) called on multiple threads.\r\n"
            L"Second thread will likely also assert.  If this is a pop-up "
            L"dialog then there is probably a second dialog with a similar"
            L"error message."
            );
    }

    AssertConstMsgW(
        oOrgStatus.cEntries != 0,
        L"More calls to Enter than to Leave."
        );
}


//+------------------------------------------------------------------------
//
//  Function:  CAssertEntry::~CAssertEntry
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CAssertEntry::~CAssertEntry()
{
    AssertConstMsgW(m_oEntryStatus.cEntries == 0,
              L"Object was entered without being left.\r\n"
              L"This often indicates a previous entry violation.");
}

//+------------------------------------------------------------------------
//
//  Function:  CAssertEntry::ForceSetEntryStatus
//
//  Synopsis:  This method should be called if some other thread changed
//             m_oEntryStatus.l64 between the assignment
//                 oOrgStatus.l64 = m_oEntryStatus.l64;
//             above and the call to MILInterlockedCompareExchange64
//
//             Spin until we can be sure that we set m_oEntryStatus.l64 to the new value.
//             Forcing this variable to be set should cause the other thread to Assert
//             as well.
//              
//  Warning:   This (building your own spin lock) is very bad practice for shipping
//             code. It's okay here because this is debug-build only. Also, this is the way
//             that MILInterlockedExchange64 is implemented for x86 in the kernel. See
//               %sdxroot%\base\ntos\inc\i386_x.h
//             
//-------------------------------------------------------------------------
void
CAssertEntry::ForceSetEntryStatus(
    __inout_ecount(1) EntryState *pNewStatus,
    __out_ecount(1) EntryState *pCurStatus,
    INT cEntryIncrement
    )
{
    EntryState oTempStatus;
    do
    {
        oTempStatus.l64 = m_oEntryStatus.l64;
        pNewStatus->cEntries = static_cast<UINT>(oTempStatus.cEntries + cEntryIncrement);

        pCurStatus->l64 = MILInterlockedCompareExchange64(
            &m_oEntryStatus.l64,
            pNewStatus->l64,
            oTempStatus.l64
            );
    } while (pCurStatus->l64 != oTempStatus.l64);
}


#endif



