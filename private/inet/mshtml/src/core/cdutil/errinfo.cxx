//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       errinfo.cxx
//
//  Contents:   CErrorInfo
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

const CLSID CLSID_CErrorInfo = { 0x44102920, 0xD5AA, 0x11ce, 0xB6, 0x56, 0x00, 0xAA, 0x00, 0x4C, 0xD6, 0xD8 };

extern HRESULT GetSolutionText(HRESULT hrError, LPTSTR pstr, int cch);

MtDefine(CErrorInfo, ObjectModel, "CErrorInfo")

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::CErrorInfo
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------

CErrorInfo::CErrorInfo()
{
    _ulRefs = 1;
    IncrementSecondaryObjectCount( 2 );
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::~CErrorInfo
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------

CErrorInfo::~CErrorInfo()
{
    int c;
    TCHAR **ppch;

    for (ppch = _apch, c = EPART_LAST; c > 0; ppch++, c--)
    {
        delete *ppch;
    }

    DecrementSecondaryObjectCount( 2 );
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::QueryInterface, IUnknown
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IErrorInfo ||
        iid == IID_IUnknown)
    {
        *ppv = (IErrorInfo *)this;
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else if (iid == CLSID_CErrorInfo)
    {
        *ppv = this;
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::SetTextV/SetText
//
//  Synopsis:   Set text of error description.
//
//  Arguments:  epart   Part of error message, taken from EPART enum.
//              ids     String id of format string.
//              pvArgs  Arguments
//
//----------------------------------------------------------------------------

void
CErrorInfo::SetTextV(EPART epart, UINT ids, void *pvArgs)
{
    if (_apch[epart])
    {
        delete _apch[epart];
        _apch[epart] = NULL;
    }

    IGNORE_HR(VFormat(
            FMT_OUT_ALLOC, 
            &_apch[epart], 
            0, 
            MAKEINTRESOURCE(ids),
            pvArgs));
}

void __cdecl
CErrorInfo::SetText(EPART epart, UINT ids, ...)
{
    va_list arg;

    va_start(arg, ids);
    SetTextV(epart, ids, &arg);
    va_end(arg);    
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetGUID, IErrorInfo
//
//  Synopsis:   Return iid of interface defining error code.
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::GetGUID(GUID *pguid)
{
    // Assume OS defined errors only.
    *pguid = g_Zero.clsid;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetSource, IErrorInfo
//
//  Synopsis:   Get progid of error source.
//
//----------------------------------------------------------------------------

extern "C" CLSID CLSID_CCDControl;

HRESULT
CErrorInfo::GetSource(BSTR *pbstrSource)
{
    HRESULT hr; 
    OLECHAR * pstrProgID = NULL;

    *pbstrSource = 0;
    if (_clsidSource == g_Zero.clsid)
        RRETURN(E_FAIL);

    hr = THR(ProgIDFromCLSID(_clsidSource, &pstrProgID));
    if (hr)
        goto Cleanup;

    hr = THR(FormsAllocString(pstrProgID, pbstrSource));

Cleanup:

    CoTaskMemFree(pstrProgID);
    RRETURN(hr);
}
        
//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetHelpFile, IErrorInfo
//
//  Synopsis:   Get help file describing error.
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::GetHelpFile(BSTR *pbstrHelpFile)
{
    RRETURN(THR(FormsAllocString(GetFormsHelpFile(), pbstrHelpFile)));
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetHelpContext, IErrorInfo
//
//  Synopsis:   Get help context describing error.
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::GetHelpContext(DWORD *pdwHelpContext)
{
    *pdwHelpContext = _dwHelpContext;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetDescription, IErrorInfo
//
//  Synopsis:   Get description of the error.
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::GetDescription(BSTR *pbstrDescription)
{
    HRESULT hr;
    BSTR    bstrSolution = NULL;
    BSTR    bstrDescription = NULL;

    hr = THR(GetDescriptionEx(&bstrDescription, &bstrSolution));
    if (hr)
        goto Cleanup;

    if (bstrDescription && bstrSolution)
    {
        hr = THR(FormsAllocStringLen(
                (LPCTSTR)NULL, 
                FormsStringLen(bstrDescription) + 2 + FormsStringLen(bstrSolution),
                pbstrDescription));
        if (hr)
            goto Cleanup;

        _tcscpy(*pbstrDescription, bstrDescription);
        _tcscat(*pbstrDescription, TEXT("  "));         //  Review: FE grammar?
        _tcscat(*pbstrDescription, bstrSolution);
    }
    else
    {
        *pbstrDescription = bstrDescription;
        bstrDescription = NULL;
    }

Cleanup:
    FormsFreeString(bstrDescription);
    FormsFreeString(bstrSolution);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetMemberName
//
//  Synopsis:   Get name of member _dispidInvoke in interface _iidInvoke.
//
//----------------------------------------------------------------------------

HRESULT
CErrorInfo::GetMemberName(BSTR *pbstrName)
{
    HRESULT hr;
    ITypeInfo *pTI = NULL;

    hr = THR(LoadF3TypeInfo(_iidInvoke, &pTI));
    if (hr)
        goto Cleanup;

    hr = THR(pTI->GetDocumentation(_dispidInvoke, pbstrName, NULL, NULL, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pTI);
    RRETURN(hr);
   
}

//+---------------------------------------------------------------------------
//
//  Method:     CErrorInfo::GetDescriptionEx, IErrorInfo2
//
//  Synopsis:   
//
//  Arguments:  
//
//----------------------------------------------------------------------------

HRESULT 
CErrorInfo::GetDescriptionEx(
    BSTR *pbstrDescription,
    BSTR *pbstrSolution)
{
    HRESULT hr = S_OK;
    TCHAR   achBufAction[FORMS_BUFLEN];
    TCHAR   achBufError[FORMS_BUFLEN];
    TCHAR   achBufSolution[FORMS_BUFLEN];
    TCHAR * pchAction;
    TCHAR * pchError;
    TCHAR * pchSolution;
    BSTR    bstrMemberName = NULL;

    *pbstrDescription = NULL;
    *pbstrSolution = NULL;

    if (_apch[EPART_ACTION])
    {
        pchAction = _apch[EPART_ACTION];
    }
    else if (_invkind == INVOKE_PROPERTYPUT &&
            _hr == E_INVALIDARG)
    {
        pchAction = NULL;
    }
    else if (_invkind)
    {
        hr = THR(GetMemberName(&bstrMemberName));
        if (hr)
            goto Error;

        hr = THR(Format(
                0, 
                achBufAction, 
                ARRAY_SIZE(achBufAction),
                _invkind == INVOKE_PROPERTYPUT ? 
                    MAKEINTRESOURCE(IDS_EA_SETTING_PROPERTY) :
                (_invkind == INVOKE_PROPERTYGET ? 
                    MAKEINTRESOURCE(IDS_EA_GETTING_PROPERTY) :
                    MAKEINTRESOURCE(IDS_EA_CALLING_METHOD)),
                bstrMemberName));
        if (hr)
            goto Error;
        
        pchAction = achBufAction;
    }
    else
    {
        pchAction = NULL;
    }

    if (_apch[EPART_ERROR])
    {
        pchError = _apch[EPART_ERROR];
    }
    else if (_invkind == INVOKE_PROPERTYPUT && 
            _hr == E_INVALIDARG)
    {
        hr = THR(GetMemberName(&bstrMemberName));
        if (hr)
            goto Error;

        hr = THR(Format(
                0, 
                achBufError, 
                ARRAY_SIZE(achBufError),
                MAKEINTRESOURCE(IDS_EE_INVALID_PROPERTY_VALUE),
                bstrMemberName));
        if (hr)
            goto Error;

        pchError = achBufError;
    }
    else
    {
        hr = THR(GetErrorText(_hr, achBufError, ARRAY_SIZE(achBufError)));
        if (hr)
            goto Error;

        pchError = achBufError;
    }

    if (pchAction)
    {
        hr = FormsAllocStringLen(
                pchAction,
                _tcslen(pchAction) + _tcslen(pchError) + 2,
                pbstrDescription);
        if (!*pbstrDescription)
            goto MemoryError;
        _tcscat(*pbstrDescription, TEXT(" "));
        _tcscat(*pbstrDescription, pchError);
    }
    else
    {
        hr = FormsAllocString(pchError,pbstrDescription);
        if (!*pbstrDescription)
            goto MemoryError;
    }

    if (_apch[EPART_SOLUTION])
    {
        pchSolution = _apch[EPART_SOLUTION];
    }
    else
    {
        hr = THR(GetSolutionText(_hr, achBufSolution, ARRAY_SIZE(achBufSolution)));
        if (hr)
            goto Error;

        pchSolution = achBufSolution[0] ? achBufSolution : NULL;
    }

    hr = THR(FormsAllocString(pchSolution, pbstrSolution));
    if (hr)
        goto Error;

Cleanup:
    SysFreeString(bstrMemberName);
    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    SysFreeString(*pbstrDescription);
    SysFreeString(*pbstrSolution);
    *pbstrDescription = NULL;
    *pbstrSolution = NULL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetErrorInfo
//
//----------------------------------------------------------------------------

CErrorInfo * 
GetErrorInfo()
{
    THREADSTATE *   pts = GetThreadState();

    if (!pts->pEI)
    {
        pts->pEI = new CErrorInfo;
    }

    return pts->pEI;
}

//+---------------------------------------------------------------------------
//
//  Function:   ClearErrorInfo
//
//----------------------------------------------------------------------------

void         
ClearErrorInfo(
    THREADSTATE *   pts)
{
    if (!pts)
        pts = GetThreadState();
    ClearInterface(&(pts->pEI));
}


//+---------------------------------------------------------------------------
//
//  Function:   CloseErrorInfo
//
//----------------------------------------------------------------------------

void
CloseErrorInfo(HRESULT hr, REFCLSID clsidSource)
{
    IErrorInfo *    pErrorInfo;

    Assert(FAILED(hr));
  
    if (GetErrorInfo() == NULL)
    {
        // There's an error, but we couldn't allocate
        // an error info object.  Release the global error
        // object so that our caller's caller will
        // not be confused.

        hr = GetErrorInfo(0, &pErrorInfo);
        if (!hr)
        {
            ReleaseInterface(pErrorInfo);
        }
    }
    else
    {
        THREADSTATE *   pts = GetThreadState();
        pts->pEI->_hr = hr;
        pts->pEI->_clsidSource = clsidSource;
        SetErrorInfo(NULL, pts->pEI);

        ClearInterface(&(pts->pEI));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   PutErrorInfoText
//
//----------------------------------------------------------------------------

void
PutErrorInfoText(EPART epart, UINT ids, ...)
{
    va_list arg;

    if (GetErrorInfo() != NULL)
    {
        va_start(arg, ids);
        TLS(pEI)->SetTextV(epart, ids, &arg);
        va_end(arg);
    }
}
