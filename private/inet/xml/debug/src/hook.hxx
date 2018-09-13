/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/

//+------------------------------------------------------------------------
//
//  File:       hook.hxx
//
//  Contents:   Hook interfaces used for debugging
//
//  History:    09-Jul-97   JohnV Created
//
//-------------------------------------------------------------------------

#ifndef _HOOK_HXX_
#define _HOOK_HXX_

#ifndef NO_DEBUG_HOOK

#ifndef HOOKCLIENT
// Includes needed for implementation
#include "_rtext.h"
#include "_txtsave.h"
#endif

#ifdef HOOKCLIENT
#define HOOKMETH(x) ;
#else
#define HOOKMETH(x) { x }
#endif  // HOOKCLIENT

#define HOOKCLASS(hkcl) \
     class hkcl##Hook

#define HOOKIMPL(hkcl) \
     hkcl * _impl;

#define HOOK0(ret, hkcl) \
     virtual ret hkcl() HOOKMETH( return _impl->hkcl(); )

#define HOOK1(ret, hkcl, t1, a1) \
     virtual ret hkcl(t1 a1) HOOKMETH( return _impl->hkcl(a1); )

#define HOOK2(ret, hkcl, t1, a1, t2, a2) \
     virtual ret hkcl(t1 a1, t2 a2) HOOKMETH( return _impl->hkcl(a1, a2); )

#define HOOK3(ret, hkcl, t1, a1, t2, a2, t3, a3) \
     virtual ret hkcl(t1 a1, t2 a2, t3 a3) HOOKMETH( return _impl->hkcl(a1, a2, a3); )

#define HOOKCREATE0(hkcl) \
     virtual hkcl * New##hkcl##Hook() HOOKMETH( return ::new hkcl##Hook(); )

#define HOOKCREATE1(hkcl, t1, a1) \
     virtual hkcl##Hook * New##hkcl##Hook(t1 a1) HOOKMETH( return ::new hkcl##Hook(a1); )

#define HOOKCTOR0(hkcl) \
     hkcl##Hook()      HOOKMETH( _impl = ::new hkcl; )

#define HOOKCTOR1(hkcl, t1, a1) \
     hkcl##Hook(t1 a1) HOOKMETH( _impl = ::new hkcl(a1); )

#define HOOKDTOR(hkcl) \
     virtual ~hkcl##Hook() HOOKMETH( delete _impl; )


//
// Forward class declarations
//
class CElement;
class CRchTxtPtr;
class CTxtEdit;


//
// Wraps the rich text pointer class
//
HOOKCLASS(CRchTxtPtr)
{
public:
    HOOKIMPL(CRchTxtPtr)
            
    HOOKCTOR1(CRchTxtPtr, CTxtEdit *, ped)
    HOOKDTOR(CRchTxtPtr);

    HOOK0(LONG, GetCchRun)

    virtual LONG GetText(LONG cch, TCHAR * pch)
    HOOKMETH( return _impl->_rpTX.GetText(cch, pch); )
    
    HOOK0(CBranchPtr, CurrBranch)
    HOOK0(BOOL, OnLastRun)
    HOOK0(BOOL, MoveToNextRun)
};

//
// Global hook functions
//
class CHook
{
public:
    virtual HRESULT VFormat(DWORD dwOptions, void * pvOutput, int cchOutput,
        TCHAR * pchFmt, void * pvArgs)
    HOOKMETH( return ::VFormat(dwOptions, pvOutput, cchOutput, pchFmt, pvArgs); )

    virtual HRESULT SaveTree(CTxtEdit * ped, IStream * pstm)
    HOOKMETH( CStreamWriteBuff swb(pstm); \
              CTreeSaver ts(TempBranchPtr(ped), &swb); \
              return ts.Save(); )

    virtual BOOL SameScope(CElement * pel1, CElement * pel2)
    HOOKMETH( return ::SameScope(pel1, pel2); )

    HOOKCREATE1(CRchTxtPtr, CTxtEdit *, ped);
};

#endif  // NO_DEBUG_HOOK

#endif  // _HOOK_H_
