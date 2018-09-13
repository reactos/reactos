/*
 * @(#)IDOMError.cxx 1.0
 * 
 *  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#include "idomerror.hxx"
#include "xmldomdid.h"

CRITICAL_SECTION cs2;
bool cs_init2 = false;

class CSLazyLock
{
public:

    CSLazyLock(LPCRITICAL_SECTION p) 
    { 
        if (! cs_init2) {
            InitializeCriticalSection(p);
            cs_init2 = true;
        }
        EnterCriticalSection(p);
        pcs = p;
    }

    ~CSLazyLock()
    {
        LeaveCriticalSection(pcs);
    }

private:

    LPCRITICAL_SECTION pcs;
};

#define CRITICALSECTIONLOCK CSLazyLock lock(&cs2);

////////////////////////////////////////////////////////////////////////////////
// IDispatch interface customized implementation
//

static INVOKE_METHOD
s_rgDOMErrorMethods[] =
{
    // name         dispid                   argument number  argument table  argument guid id  return type    invoke kind
    { L"errorCode", DISPID_VALUE,             0,            NULL,               NULL,           VT_I4,      DISPATCH_PROPERTYGET},
    { L"filepos",   DISPID_DOM_ERROR_FILEPOS, 0,            NULL,               NULL,           VT_I4,      DISPATCH_PROPERTYGET},
    { L"line",      DISPID_DOM_ERROR_LINE,    0,            NULL,               NULL,           VT_I4,      DISPATCH_PROPERTYGET},
    { L"linepos",   DISPID_DOM_ERROR_LINEPOS, 0,            NULL,               NULL,           VT_I4,      DISPATCH_PROPERTYGET},
    { L"reason",    DISPID_DOM_ERROR_REASON,  0,            NULL,               NULL,           VT_BSTR,    DISPATCH_PROPERTYGET},
    { L"srcText",   DISPID_DOM_ERROR_SRCTEXT, 0,            NULL,               NULL,           VT_BSTR,    DISPATCH_PROPERTYGET},
    { L"url",       DISPID_DOM_ERROR_URL,     0,            NULL,               NULL,           VT_BSTR,    DISPATCH_PROPERTYGET}
};

static DISPIDTOINDEX 
s_DOMError_DispIdMap[] =
{
  { DISPID_VALUE,               0}, // IXMLDOMParseError::errorCode
  { DISPID_DOM_ERROR_URL,       6}, // IXMLDOMParseError::url
  { DISPID_DOM_ERROR_REASON,    4}, // IXMLDOMParseError::reason
  { DISPID_DOM_ERROR_SRCTEXT,   5}, // IXMLDOMParseError::srcText
  { DISPID_DOM_ERROR_LINE,      2}, // IXMLDOMParseError::line
  { DISPID_DOM_ERROR_LINEPOS,   3}, // IXMLDOMParseError::linepos
  { DISPID_DOM_ERROR_FILEPOS,   1}  // IXMLDOMParseError::filepos
};

DISPATCHINFO _dispatch<IXMLDOMParseError, &LIBID_MSXML, &IID_IXMLDOMParseError>::s_dispatchinfo =
{
    NULL, &IID_IXMLDOMParseError, &LIBID_MSXML, ORD_MSXML, s_rgDOMErrorMethods, NUMELEM(s_rgDOMErrorMethods), s_DOMError_DispIdMap, NUMELEM(s_DOMError_DispIdMap), DOMError::_invoke
};  


////////////////////////////////////////////////////////////////////////////////
// IXMLDOMParseError Interface
//

DOMError::DOMError(Exception* e) : _pException(e)
{
    // this is here so the static initialization of s_dispatchinfo will occur when this is constructed
};


HRESULT
DOMError::_invoke(
    void * pTarget,
    DISPID dispIdMember,
    INVOKE_ARG *rgArgs, 
    WORD wInvokeType,
    VARIANT *pVarResult,
    UINT cArgs)
{
    HRESULT hr;
    IXMLDOMParseError * pObj = (IXMLDOMParseError*)pTarget;

    TEST_METHOD_TABLE(s_rgDOMErrorMethods, NUMELEM(s_rgDOMErrorMethods), s_DOMError_DispIdMap, NUMELEM(s_DOMError_DispIdMap));

    switch( dispIdMember )
    {
        case DISPID_VALUE:
            hr = pObj->get_errorCode((long*) &pVarResult->lVal);
            break;

        case DISPID_DOM_ERROR_URL:
            hr = pObj->get_url(&pVarResult->bstrVal);
            break;

        case DISPID_DOM_ERROR_REASON:
            hr = pObj->get_reason(&pVarResult->bstrVal);
            break;

        case DISPID_DOM_ERROR_SRCTEXT:
            hr = pObj->get_srcText(&pVarResult->bstrVal);
            break;

        case DISPID_DOM_ERROR_LINE:
            hr = pObj->get_line((long*) &pVarResult->lVal);
            break;

        case DISPID_DOM_ERROR_LINEPOS:
            hr = pObj->get_linepos((long*) &pVarResult->lVal);
            break;

        case DISPID_DOM_ERROR_FILEPOS:
            hr = pObj->get_filepos((long*) &pVarResult->lVal);
            break;

        default:
            hr = DISP_E_MEMBERNOTFOUND;
            break;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE 
DOMError::get_errorCode( 
         /* [out][retval] */ LONG __RPC_FAR *plErrorCode)
{
    CHECK_ARG_INIT(plErrorCode);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
            *plErrorCode = _pException->getHRESULT();
        else
        {
            *plErrorCode = 0;
            hr = S_FALSE;
        }
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY


    return hr;
}

HRESULT STDMETHODCALLTYPE 
DOMError::get_url( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrUrl)
{
    CHECK_ARG_INIT(pbstrUrl);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException && _pException->getUrl() != NULL)
            *pbstrUrl = _pException->getUrl()->getBSTR();
        else
        {
            *pbstrUrl = null;
            hr = S_FALSE;
        }
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY


    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMError::get_reason( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrReason)
{
    CHECK_ARG_INIT(pbstrReason);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
        {
            String* msg = _pException->getMessage();
            if (msg != null)
            {
                *pbstrReason = msg->getBSTR();
                goto Cleanup;
            }
        }

        *pbstrReason = null;
        hr = S_FALSE;
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY


Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMError::get_srcText( 
        /* [out][retval] */ BSTR __RPC_FAR *pbstrText)
{
    CHECK_ARG_INIT(pbstrText);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
        {
            String * s = _pException->getSourceText();
            if (s)
            {
                *pbstrText = s->getBSTR();
                goto Cleanup;
            }
        }

        *pbstrText = null;
        hr = S_FALSE;
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY

Cleanup:
    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMError::get_line( 
        /* [out][retval] */ long __RPC_FAR *plLine)
{
    CHECK_ARG(plLine);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
            *plLine = _pException->_nLine;
        else
        {
            *plLine = 0;
            hr = S_FALSE;
        }
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY


    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMError::get_linepos( 
        /* [out][retval] */ long __RPC_FAR *plLinePos)
{
    CHECK_ARG(plLinePos);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
            *plLinePos = _pException->_nCol;
        else
        {
            *plLinePos = 0;
            hr = S_FALSE;
        }
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY

    return hr;
}
    
HRESULT STDMETHODCALLTYPE 
DOMError::get_filepos( 
        /* [out][retval] */ long __RPC_FAR *plFilePos)
{
    CHECK_ARG(plFilePos);
    STACK_ENTRY_IUNKNOWN(this);
    CRITICALSECTIONLOCK;
    HRESULT hr = S_OK;

    TRY {
        if (_pException)
            *plFilePos = _pException->_nFilePosition;
        else
        {
            *plFilePos = 0;
            hr = S_FALSE;
        }
    } 
    CATCH 
    { 
        hr = ERESULTINFO; 
    }
    ENDTRY


    return hr;
}