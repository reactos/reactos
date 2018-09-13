/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    tlog.h

Abstract:

    This module implements the travel log functionality for file open
    and save as dialogs.

Revision History:
    02-20-98          arulk                 created

--*/
#ifndef _TLOG_H_
#define _TLOG_H_

#ifdef __cplusplus

#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>


//
//  Defines for Travel Log.
//
#define TRAVEL_BACK             0x0001
#define TRAVEL_FORWARD          0x0002



//--------------------------------------------------------------------
//Travel Log Link Class Definition
//--------------------------------------------------------------------
class TLogLink
{
public:
    TLogLink();
    TLogLink(LPITEMIDLIST pidl);    
    ~TLogLink();
    UINT AddRef();
    UINT Release();
    TLogLink *GetNextLink() { return _ptllNext;};
    TLogLink *GetPrevLink() { return _ptllPrev;};

    void SetNextLink(TLogLink* ptllNext);    

    HRESULT GetPidl(LPITEMIDLIST* ppidl);    
    HRESULT SetPidl(LPITEMIDLIST pidl);

    BOOL    CanTravel(int iDir);


private:
    UINT _cRef;
    LPITEMIDLIST _pidl;
    TLogLink * _ptllPrev;
    TLogLink * _ptllNext;
};



//------------------------------------------------------------------------
//Travel Log Class Definition
//------------------------------------------------------------------------
class TravelLog
{
public:
    friend HRESULT Create_TravelLog(TravelLog *pptlog);
    TravelLog();
    ~TravelLog();
    UINT AddRef();
    UINT Release();
    HRESULT AddEntry(LPITEMIDLIST pidl);
    BOOL CanTravel(int iDir);
    HRESULT Travel(int iDir);
    HRESULT GetCurrent(LPITEMIDLIST *ppidl);

private:
    UINT _cRef;
    TLogLink *_ptllCurrent;
    TLogLink *_ptllRoot;
};

#endif //_cplusplus

#ifdef _cplusplus
extern "C" {
#endif //_cplusplus

HRESULT Create_TravelLog(TravelLog **pptlog);

#ifdef _cplusplus
extern "C"
};
#endif //_cplusplus

#endif //_TLOG_H_