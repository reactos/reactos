//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996-1997.
//
//  File:       downbase.hxx
//
//  Contents:   Base interfaces for download subsystem
//
//----------------------------------------------------------------------------

#ifndef I_DOWNBASE_HXX_
#define I_DOWNBASE_HXX_
#pragma INCMSG("--- Beg 'downbase.hxx'")

// Debugging ------------------------------------------------------------------

MtExtern(Dwn)

// CBaseFT --------------------------------------------------------------------

class CBaseFT : public CVoid
{

private:

    // This forces subclasses to declare their own operator new and delete,
    // so that we get better memory metering numbers.

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    ULONG               AddRef()               { return((ULONG)InterlockedIncrement((LONG *)&_ulRefs)); }
    ULONG               Release();
    ULONG               SubAddRef()            { return((ULONG)InterlockedIncrement((LONG *)&_ulAllRefs)); }
    ULONG               SubRelease();
    CRITICAL_SECTION *  GetPcs() { return(_pcs); }
    void                SetPcs(CRITICAL_SECTION *pcs) { _pcs = pcs; }
    ULONG               GetRefs()              { return(_ulRefs); }
    ULONG               GetAllRefs()           { return(_ulAllRefs); }

    #if DBG==1
    void                EnterCriticalSection();
    void                LeaveCriticalSection();
    BOOL                EnteredCriticalSection();
    #else
    void                EnterCriticalSection() { if (_pcs) ::EnterCriticalSection(_pcs); }
    void                LeaveCriticalSection() { if (_pcs) ::LeaveCriticalSection(_pcs); }
    #endif

protected:

                        CBaseFT(CRITICAL_SECTION * pcs = NULL);
    virtual            ~CBaseFT();
    virtual void        Passivate();
    ULONG               InterlockedRelease()   { return((ULONG)InterlockedDecrement((LONG *)&_ulRefs)); }

private:

    CRITICAL_SECTION *  _pcs;
    ULONG               _ulRefs;
    ULONG               _ulAllRefs;

    #if DBG==1
    DWORD               _dwThread;
    ULONG               _cEnter;
    #endif               

};

// CExecFT --------------------------------------------------------------------

class CExecFT : public CBaseFT
{
    typedef CBaseFT super;

private:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(Dwn))

public:

    DWORD               GetThreadId()       { return(_dwThreadId); }
    HRESULT             Launch(BOOL fWait);

protected:

                        CExecFT(CRITICAL_SECTION * pcs = NULL);
    virtual            ~CExecFT();
    void                Shutdown(DWORD dwTimeOut = 30000);
    virtual HRESULT     ThreadInit() = 0;
    virtual void        ThreadExec() = 0;
    virtual void        ThreadTerm() = 0;
    virtual void        ThreadExit();
    virtual void        Passivate();
    static DWORD WINAPI StaticThreadProc(void * pv);
    DWORD               ThreadProc();

    // Data members

    THREAD_HANDLE       _hThread;
    DWORD               _dwThreadId;
    EVENT_HANDLE        _hEvent;
    HRESULT             _hrInit;
};

// ----------------------------------------------------------------------------

#pragma INCMSG("--- End 'downbase.hxx'")
#else
#pragma INCMSG("*** Dup 'downbase.hxx'")
#endif
