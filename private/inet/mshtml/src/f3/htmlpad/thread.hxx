//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       thread.hxx
//
//  Contents:   Helper functions for thread management
//
//----------------------------------------------------------------------------

#ifndef _HXX_THREAD
#define _HXX_THREAD

//+----------------------------------------------------------------------------
//
//  Class:      CGlobalLock
//
//  Synopsis:   Smart object to lock/unlock access to all global variables
//              Declare an instance of this class within the scope that needs
//              guarded access. Class instances may be nested.
//
//  Usage:      Lock variables by using the LOCK_GLOBALS marco
//              Simply include this macro within the appropriate scope (as
//              small as possible) to protect access. For example:
//
//-----------------------------------------------------------------------------

class CGlobalLock                           // tag: glock
{
public:
    CGlobalLock()
    {
        EnterCriticalSection(&s_cs);
#if DBG==1
        if (!s_cNesting)
            s_dwThreadID = GetCurrentThreadId();
        else
            Assert(s_dwThreadID == GetCurrentThreadId());
        Assert(++s_cNesting > 0);
#endif
    }

    ~CGlobalLock()
    {
#if DBG==1
        Assert(s_dwThreadID == GetCurrentThreadId());
        Assert(--s_cNesting >= 0);
#endif
        LeaveCriticalSection(&s_cs);
    }

#if DBG==1
    static BOOL IsThreadLocked()
    {
        return (s_dwThreadID == GetCurrentThreadId());
    }
#endif

    // Process attach/detach routines
    static void Init()
    {
        InitializeCriticalSection(&s_cs);
    }

    static void Deinit()
    {
#if DBG==1
        if (s_cNesting)
        {
            TraceTag((tagError, "Global lock count > 0, Count=%0d", s_cNesting));
        }
#endif
        DeleteCriticalSection(&s_cs);
    }

private:
    static CRITICAL_SECTION s_cs;
#if DBG==1
    static DWORD            s_dwThreadID;
    static LONG             s_cNesting;
#endif
};

#define LOCK_GLOBALS    CGlobalLock glock

void IncrementObjectCount();
void DecrementObjectCount();

#endif // #ifndef _HXX_THREAD
