/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    tlog.cpp

Abstract:

    This module implements the travel log functionality for file open
    and save as dialogs.

Revision History:
    02-20-98          arulk                 created

--*/
#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <commdlg.h>
#include "util.h"

#include "tlog.h"

//-------------------------------------------------------------------------
// Travel Log Link implementation
//-------------------------------------------------------------------------

TLogLink::TLogLink()
:_cRef(1), _pidl(NULL), _ptllNext(NULL), _ptllPrev(NULL)
{
}


TLogLink::TLogLink(LPITEMIDLIST  pidl)
:_cRef(1), _pidl(NULL), _ptllNext(NULL), _ptllPrev(NULL)
{
    _pidl = ILClone(pidl);
}

TLogLink::~TLogLink()
{
    if (_pidl)
    {
        ILFree(_pidl);
    }

    if (_ptllNext)
    {
        _ptllNext->Release();
    }
}

UINT TLogLink::AddRef()
{
    return ++_cRef;
}

UINT TLogLink::Release()
{
    if (--_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


void TLogLink::SetNextLink(TLogLink* ptllNext)
{
    //Do we already have Next Link ?
    if (_ptllNext)
    {
        // Release the next link
        _ptllNext->Release();
    }

    //Set the given pointer as our next pointer
    _ptllNext = ptllNext;

    if (_ptllNext)
    {
        //Since we are caching the pointer , Add reference to it
        _ptllNext->AddRef();

        //Also update the prev link of our new pointer to point to us
        _ptllNext->_ptllPrev = this;
    }
}


HRESULT TLogLink::GetPidl(LPITEMIDLIST* ppidl)
{
    *ppidl = ILClone(_pidl);
    if (*ppidl)
        return NOERROR;
    else {
        return E_OUTOFMEMORY;
    }
}

HRESULT TLogLink::SetPidl(LPITEMIDLIST pidl)
{
    if (_pidl)
    {
        ILFree(_pidl);
    }
    _pidl = ILClone(pidl);
    return NOERROR;
}

BOOL TLogLink::CanTravel(int iDir)
{
    BOOL fRet = FALSE;
    switch ( iDir )
    {
        case ( TRAVEL_BACK ) :
        {
            if (_ptllPrev != NULL)
            {
                fRet = TRUE;
            }
            break;
        }

        case ( TRAVEL_FORWARD ) :
        {
            if (_ptllNext !=NULL)
            {
                fRet = TRUE;
            }
            break;
        }
    }

    return fRet;
}


//----------------------------------------------------------------------------------
//Travel Log Class  Implementation
//----------------------------------------------------------------------------------

TravelLog::TravelLog()
:_cRef(1),_ptllCurrent(NULL), _ptllRoot(NULL)
{
}


TravelLog::~TravelLog()
{
    if (_ptllRoot)
    {
        _ptllRoot->Release();
    }
}

UINT TravelLog::AddRef()
{
   return  ++_cRef;
}


UINT TravelLog::Release()
{
    if (--_cRef > 0 )
        return _cRef;

    delete this;
    return 0;
}


HRESULT TravelLog::AddEntry(LPITEMIDLIST pidl)
{
    TLogLink  *ptll =  new TLogLink(pidl);
    if (!ptll)
        return E_FAIL;

    if (_ptllCurrent) {
        _ptllCurrent->SetNextLink(ptll);
        ptll->Release();
    }
    else
    {
        _ptllRoot = ptll;
    }

    _ptllCurrent = ptll;
    
    return NOERROR;
}


BOOL TravelLog::CanTravel(int iDir)
{
    if (_ptllCurrent)
    {
        return _ptllCurrent->CanTravel(iDir);
    }
    return FALSE;
}

HRESULT TravelLog::Travel(int iDir)
{
    HRESULT hres = E_FAIL;
    TLogLink *ptll;
    switch(iDir)
    {
        case ( TRAVEL_FORWARD ) :
        {
            if (CanTravel(iDir))
            {
                ptll = _ptllCurrent->GetNextLink();
                _ptllCurrent = ptll;
                hres = NOERROR;
            }
            break;

        }

        case ( TRAVEL_BACK ):
        {
            if (CanTravel(iDir))
            {
                ptll = _ptllCurrent->GetPrevLink();
                _ptllCurrent = ptll;
                hres = NOERROR;
            }
            break;

        }
    }

    return hres;
}


HRESULT TravelLog::GetCurrent(LPITEMIDLIST *ppidl)
{
    //Set the return value. Just in case
    *ppidl = NULL;
    if (_ptllCurrent)
    {
        return _ptllCurrent->GetPidl(ppidl);
        
    }
    return E_FAIL;
}

HRESULT Create_TravelLog(TravelLog **pptlog)
{
    HRESULT hres = E_FAIL;
    *pptlog = new TravelLog();

    if (*pptlog)
    {
        hres = S_OK;
    }

    return hres;
}
