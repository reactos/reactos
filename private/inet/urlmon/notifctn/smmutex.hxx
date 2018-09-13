//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       smmutex.hxx
//
//  Contents:   Class definition for shared memory mutex.
//
//  Classes:    CSmMutex
//
//  Functions:
//
//  History:    2-13-1997   JohannP (Johann Posch)   Created
//
//--------------------------------------------------------------------------
#ifndef __SMMUTEX_HXX__
#define __SMMUTEX_HXX__

//+-------------------------------------------------------------------------
//
//  Class:      CSmMutex
//
//  Purpose:    Mutex shared among processes
//
//  Interface:  Get - get the mutex
//              Release - release the mutex for other processes
//              Created - tell whether this process created the mutex
//
//--------------------------------------------------------------------------
class CSmMutex
{
public:

    inline CSmMutex();

    inline ~CSmMutex(void);

    SCODE Init(LPTSTR pszName, BOOL fGet = TRUE);

    inline void Get(void);

    inline void Release(void);

    inline BOOL Created(void);

private:

    BOOL                _fCreated;

    HANDLE              _hMutex;

};

//+---------------------------------------------------------------------------
//
//  Member:     CSmMutex::CSmMutex, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
inline CSmMutex::CSmMutex()
{
    _fCreated = FALSE;
    _hMutex = NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSmMutex::~CSmMutex
//
//  Synopsis:   Clean up mutex when we are done with it.
//
//--------------------------------------------------------------------------
inline CSmMutex::~CSmMutex(void)
{
    // Release the Mutex -- this allows destructors of objects
    // to get the mutex and leave it set until the mutex is released
    // by the destructor.
    Release();

    // Release our handle.
    CloseHandle(_hMutex);
}

//+-------------------------------------------------------------------------
//
//  Member:     CSmMutex::Get
//
//  Synopsis:   Get control of mutex
//
//--------------------------------------------------------------------------
inline void CSmMutex::Get(void)
{
    WaitForSingleObject(_hMutex, INFINITE);
}

//+-------------------------------------------------------------------------
//
//  Member:     CSmMutex::Release
//
//  Synopsis:   Release mutex after a get
//
//--------------------------------------------------------------------------
inline void CSmMutex::Release(void)
{
    ReleaseMutex(_hMutex);
}

//+-------------------------------------------------------------------------
//
//  Member:     CSmMutex::Created
//
//  Synopsis:   Tell whether this process created the mutex
//
//  Returns:    TRUE if this process created the mutex
//
//--------------------------------------------------------------------------
inline BOOL CSmMutex::Created(void)
{
    return _fCreated;
}

//+-------------------------------------------------------------------------
//
//  Class:      CLockSmMutex
//
//  Purpose:    Simple class to guarantee about Mutex is unlocked
//
//--------------------------------------------------------------------------
class CLockSmMutex
{
public:

    CLockSmMutex(CSmMutex& smm);
    ~CLockSmMutex(void);

private:
    CSmMutex&           _smm;
};

//+-------------------------------------------------------------------------
//
//  Member:     CLockSmMutex::CLockSmMutex
//
//  Synopsis:   Get mutex
//
//  Arguments:  [smm] -- mutex to get
//
//--------------------------------------------------------------------------
inline CLockSmMutex::CLockSmMutex(CSmMutex& smm) : _smm(smm)
{
    _smm.Get();
}

//+-------------------------------------------------------------------------
//
//  Member:     CLockSmMutex::~CLockSmMutex
//
//  Synopsis:   Release the mutex
//
//--------------------------------------------------------------------------
inline CLockSmMutex::~CLockSmMutex(void)
{
    _smm.Release();
}

#endif // __SMMUTEX_HXX__
