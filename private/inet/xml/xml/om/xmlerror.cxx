/*
 * @(#)XMLError.cxx 1.0 11/17/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */
#include "core.hxx"
#pragma hdrstop

#include "xml/om/document.hxx"
#include "xml/om/xmlerror.hxx"
#include "xml/om/omlock.hxx"

HRESULT STDMETHODCALLTYPE IXMLErrorWrapper::GetErrorInfo(XML_ERROR *pError)
{        
    HRESULT hr;
    if (!pError)
        return E_INVALIDARG;

    STACK_ENTRY;
    OMWRITELOCK(getWrapped());

    TRY
    {
        pError->_nLine = 0;
        pError->_pchBuf = NULL;
        pError->_cchBuf = 0;
        pError->_ich = 0;
        pError->_pszFound = NULL;
        pError->_pszExpected = NULL;

        Exception *e = getWrapped()->getErrorMsg();
        if (e)
        {
            pError->_nLine = e->_nLine;
            pError->_ich = e->_nCol;
            if (pError->_ich < 0) // sanity check.
                pError->_ich = 0;

            String* expected = String::emptyString();
            pError->_pszExpected = expected->getBSTR();
            String* msg = e->getMessage();
            if (msg != null)
                pError->_pszFound = msg->getBSTR();

            if (e->getSourceText())
                pError->_pchBuf = e->getSourceText()->getBSTR();
            else
                pError->_pchBuf = String::emptyString()->getBSTR();

            // Some client code already uses _ich as an index into _pchBuf, so
            // now we need to fix the buffer in case it is too small.
            ULONG len = _tcslen(pError->_pchBuf);
            if (pError->_ich >= len)
                pError->_ich = len-1;
            pError->_cchBuf = len;
        }
        if (pError->_pchBuf == NULL)
            pError->_pchBuf = String::emptyString()->getBSTR();
        if (pError->_pszFound == NULL)
            pError->_pszFound = String::emptyString()->getBSTR();
        if (pError->_pszExpected == NULL)
            pError->_pszExpected = String::emptyString()->getBSTR();
        hr = S_OK;
    }
    CATCH
    {
        hr = ERESULTINFO;
    }
    ENDTRY
    return hr;
}

