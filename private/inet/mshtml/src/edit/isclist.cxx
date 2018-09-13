//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       ISCLIST.CXX
//
//  Contents:   Implementation of input sequence checker list. Input 
//              Sequence Checkers are used for validating input of languages
//              like Thai, Hindi, and Vietnamese.
//
//  Classes:    CISCList
//
//  History:    10-13-98 - paulnel - created
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

const IID IID_IInputSequenceChecker = {0x6CF60DE0,0x42DC,0x11D2,{0xBE,0x22,0x08,0x00,0x09,0xDC,0x0A,0x8D}};
const IID IID_IEnumInputSequenceCheckers = {0x6FA9A2A8,0x437A,0x11d2,{0x97,0x12,0x00,0xC0,0x4F,0x79,0xE9,0x8B}};
const IID IID_IInputSequenceCheckerContainer = {0x02D887FA,0x4358,0x11D2,{0xBE,0x22,0x08,0x00,0x09,0xDC,0x0A,0x8D}};
const CLSID CLSID_InputSequenceCheckerContainer = {0x02D887FB,0x4358,0x11D2,{0xBE,0x22,0x08,0x00,0x09,0xDC,0x0A,0x8D}};

MtDefine(CAryISCData_pv, "Selection Manager", "CAryISCData::_pv");

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::CISCList
//
//  Synopsis:   Create the ISC list
//
//-----------------------------------------------------------------------------
CISCList::CISCList()
{
    _lcidCurrent = 0;
    _pISCCurrent = NULL;
    _nISCCount = FillList();
}

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::~CISCList
//
//  Synopsis:   Release interfaces and destroy the ISC list
//
//-----------------------------------------------------------------------------
CISCList::~CISCList()
{
    ISCDATA* prgISC;
    int i;
    
    for(i = _aryInstalledISC.Size(), prgISC = _aryInstalledISC;
    i > 0;
    i--, prgISC++)
    {
        // We need to make sure to release the ISC interfaces
        ReleaseInterface(prgISC->pISC);
    }
    _aryInstalledISC.DeleteAll();
}


//-----------------------------------------------------------------------------
//
//  Function:   CISCList::FillList
//
//  Synopsis:   Create the list of ISCs from the COM interface (if available)
//
//  Return:     Number of ISCs installed
//
//-----------------------------------------------------------------------------
int CISCList::FillList()
{
    int nListCount = 0;
    LCID lcidCurrent = LOWORD(GetKeyboardLayout(0));

    HRESULT hr = 0;
    IUnknown *pUnk = NULL;
    IInputSequenceCheckerContainer *pISCCont = NULL;
    IEnumInputSequenceCheckers *pEnum = NULL;
    ULONG lRequested = MAX_ISC_COUNT, lFetched;
    ISCDATA pISCEngines[MAX_ISC_COUNT];

    // cocreate the input sequence checker container
    // NT5 bug 298904 - add context CLSCTX_NO_CODE_DOWNLOAD because this
    //                  does not require a download from the Class Store.
    if(g_dwPlatformID == VER_PLATFORM_WIN32_NT && g_dwPlatformVersion >= 0x00050000)
    {
        hr = CoCreateInstance(CLSID_InputSequenceCheckerContainer, 
                  NULL, 
                  CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD, 
                  IID_IUnknown,
                  (void**)&pUnk);
    }
    else
    {
        hr = CoCreateInstance(CLSID_InputSequenceCheckerContainer, 
                  NULL, 
                  CLSCTX_INPROC_SERVER, 
                  IID_IUnknown,
                  (void**)&pUnk);
    }


    if(hr || !pUnk)
    goto Cleanup;

    // get the container interface
    hr = pUnk->QueryInterface(IID_IInputSequenceCheckerContainer, (void**)&pISCCont);

    if(hr || !pISCCont)
    goto Cleanup;

    // enum the input sequence checkers
    hr = pISCCont->EnumISCs(&pEnum);

    if(hr | !pEnum)
    goto Cleanup;

    do
    {
        // fetch any input sequence checkers
        hr = pEnum->Next(lRequested, (ISCDATA*) pISCEngines, &lFetched);

        if(FAILED(hr))
            goto Cleanup;

        for (ULONG i = 0; i < lFetched; i++)
        {       
            // load ISC interfaces into the list.
            hr = Add(pISCEngines[i].lcidChecker, pISCEngines[i].pISC);

            if(!hr)
                nListCount++;
        }

    } while (lFetched != 0);

    // set the ISC to the current keyboard LCID
    SetActive(lcidCurrent);

Cleanup:
    ReleaseInterface(pUnk);
    ReleaseInterface(pISCCont);
    ReleaseInterface(pEnum);

    return nListCount;
}

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::SetActive
//
//  Synopsis:   Set the current ISC to the LCID passed in
//
//  Return:     A pointer to the ISC. This will be NULL if an ISC for the 
//              LCID does not exist.
//
//-----------------------------------------------------------------------------
IInputSequenceChecker* CISCList::SetActive(LCID lcidISC)
{
    // make sure to find the ISC first. We will short circuit inside of Find
    // if _lcidCurrent = lcidISC
    _pISCCurrent = Find(lcidISC);
    _lcidCurrent = lcidISC;

    return _pISCCurrent;
}

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::CheckInputSequence
//
//  Synopsis:   Pass the input character and a buffer of characters before the
//              location of insertion to be analysed for correctness of addition
//              to text store
//
//  Return:     A BOOL indicating whether the input character should be added
//              to the text store
//
//-----------------------------------------------------------------------------
BOOL CISCList::CheckInputSequence(LPTSTR pszISCBuffer, long ich, WCHAR chTest)
{
    BOOL fAccept=TRUE;
    HRESULT hr;

    Assert(_pISCCurrent);
    hr = _pISCCurrent->CheckInputSequence(pszISCBuffer, ich, chTest, &fAccept);

    // We don't want to lock a person out from editing in
    // the event the sequence checker is hosed.
    return (hr == S_OK ? fAccept : TRUE);
}

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::Add
//
//  Synopsis:   Add an Input Sequence Check data item to the list of checkers.
//
//  Return:     HRESULT indicating success or error condition
//
//-----------------------------------------------------------------------------
HRESULT CISCList::Add(LCID lcidISC, IInputSequenceChecker* pISC)
{
    ISCDATA newISC;
    
    if(pISC == NULL)
    return E_POINTER;

    if(Find(lcidISC))
    return CONNECT_E_ADVISELIMIT;

    newISC.lcidChecker = lcidISC;
    newISC.pISC = pISC;

    return _aryInstalledISC.AppendIndirect(&newISC);
}

//-----------------------------------------------------------------------------
//
//  Function:   CISCList::Find
//
//  Synopsis:   Find the ISC process that correspondes to an LCID
//
//  Return:     A pointer to the ISC process that corresponds to the LCID. 
//              This will be NULL if an ISC for the LCID does not exist.
//
//-----------------------------------------------------------------------------
IInputSequenceChecker* CISCList::Find(LCID lcidISC)
{
    if(lcidISC == _lcidCurrent)
    return _pISCCurrent;

    ISCDATA* prgISC;
    int i;

    for(i = _aryInstalledISC.Size(), prgISC = _aryInstalledISC;
    i > 0;
    i--, prgISC++)
    {
        if(prgISC->lcidChecker == lcidISC)
        {
            return prgISC->pISC;
        }
    }
    return NULL;
}

