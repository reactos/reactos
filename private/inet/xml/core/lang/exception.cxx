/*
 * @(#)Exception.cxx 1.0 6/3/97
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifndef _RESOURCES_HXX
#include "core/util/resources.hxx"
#endif
#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#endif

DEFINE_CLASS_MEMBERS_CLASS(Exception, _T("Exception"), Base);

static Exception * s_pOutOfMemException;
#if DBG == 1
static Mutex * s_pOutputMutex;
#endif

void
Exception::classInit()
{
    String::classInit();

    if ( !s_pOutOfMemException)
    {
#if DBG == 1
        extern unsigned long ulMemAllocFailDisable;
        ulMemAllocFailDisable = 1;
#endif
        String * s = Resources::FormatSystemMessage(E_OUTOFMEMORY);
        s_pOutOfMemException = Exception::newException(E_OUTOFMEMORY, s);
        s_pOutOfMemException->AddRef();
#if DBG == 1
        ulMemAllocFailDisable = 0;
#endif
    }
#if DBG == 1
    if (!s_pOutputMutex)
    {
        extern unsigned long ulMemAllocFailDisable;
        ulMemAllocFailDisable = 1;
        s_pOutputMutex = CSMutex::newCSMutex();
        // already addref-d
        ulMemAllocFailDisable = 0;
    }
#endif
}

void 
Exception::classExit()
{
    ::release(&s_pOutOfMemException);
#if DBG == 1
    ::release(&s_pOutputMutex);
#endif
}


void *
Exception::operator new(size_t cb, NewNoException)
{
    void * pv = MemAllocNe(cb);
    if (pv)
    {
        ((Base *)pv)->spinLock();
#if DBG == 1
        // we don't have access to this info... is there any other way to set it?
        //((Base *)pv)->_dwTID = GetTlsData()->_dwTID;
#endif
    }
    return pv;
}


const DWORD EXCEPTION_RAISED = 0xE0000001;

Exception * 
Exception::getException()
{
    return GetTlsData()->_pException;
}

int 
Exception::fillException(EXCEPTION_POINTERS * ep)
{
    EXCEPTION_RECORD * er = ep->ExceptionRecord;

// #if DBG == 1
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        // Return special code so it breaks back into the debugger.
        return EXCEPTION_CONTINUE_SEARCH;
    }
// #endif

    TLSDATA * ptlsdata = GetTlsData();
    HRESULT ehr = er->ExceptionCode;
    if (ehr == EXCEPTION_RAISED)
    {
        // check in case allocating the exception object failed...
        if (ptlsdata->_pException)
            ehr = ptlsdata->_pException->getHRESULT();
        else
            ehr = E_FAIL;
    }
    else
    {
        Assert(FALSE && "Exception from system, check EXCEPTION_RECORD for address");
        Exception * e = new(NewNoExceptionEnum) Exception(String::emptyString());
        assign(&ptlsdata->_pException, e);
        if (e)
            e->hr = ehr;
    }
    ptlsdata->_hrException = ehr;
    return EXCEPTION_EXECUTE_HANDLER;
}

void 
Exception::raiseException(Exception * e)
{
    assign(&GetTlsData()->_pException, e);
    RaiseException(EXCEPTION_RAISED, 0, 0, 0);
}

void
Exception::throwAgain()
{
    raiseException(GetTlsData()->_pException);
}

void 
Exception::throwE()
{
    raiseException(this);
}

Exception::Exception(String * s)
{
    detailMessage = s;
//    _nLine = _nCol = _nFilePosition = 0; // GC'd objects are initialized to zero.
}

void Exception::finalize()
{    
    detailMessage = null;
    _pUrl = null;
    _pSourceText = null;
    super::finalize();
}

Exception *
Exception::newException(String * s)
{
    return new Exception(s);
}

Exception *
Exception::newException(HRESULT hr, String * s)
{
    Exception * e = new Exception(s);
    e->hr = hr;
    return e;
}


/**
 * Constructor with detailed message
 */
void
Exception::throwE(String * s, HRESULT hr)
{    
    Exception * e = Exception::newException(hr, s);
    e->throwE();
}

/**
 * Constructor with detailed message
 */
void
Exception::throwE(const TCHAR * c)
{
    Exception * e = Exception::newException(String::newString(c));
    e->throwE();
}

void 
Exception::throwE(HRESULT hr)
{
    Exception * e = Exception::newException(hr);
    e->throwE();
}

Exception * 
Exception::newException(HRESULT hresult, ResourceID resid, String* first, ...)
{
    va_list va;
    va_start(va, first);

    String * s = Resources::FormatMessageHelper( resid, first, va );

    va_end(va);

    Exception * e = new Exception(s);
    e->hr = hresult;
    return e;
}

void 
Exception::throwE(HRESULT hr, ResourceID resid, String* first, ...)
{
    va_list va;
    va_start(va, first);

    String * s = Resources::FormatMessageHelper( resid, first, va );

    va_end(va);

    Exception * e = Exception::newException(hr, s);
    e->throwE();
}

void 
Exception::throwE(HRESULT hresult, String* msg, int line, int col)
{
    Exception * e = Exception::newException(hresult, msg);
    e->_nLine = line;
    e->_nCol = col;
    e->throwE();
}

void 
Exception::throwLastError()
{
    throwE((HRESULT)HRESULT_FROM_WIN32(::GetLastError()));
}

void 
Exception::throwEOutOfMemory()
{
#if DBG == 1
    if (s_pOutputMutex)
    { // dump info about the thread which just threw an OOM exception
        s_pOutputMutex->Enter();

        char s_pcDebugOut[128];
        strcpy( s_pcDebugOut, "00000000 MSXML.DLL: Throwing OutOfMemory Exception\n");
    
        char * pDigit = s_pcDebugOut;
        DWORD num = GetCurrentThreadId();
        int pos = 8;
        // for each digit, pull the digit out of num and store it in the string buffer
        while ( pos-- > 0)
        {
            int digit = num & 0x0f;
            if (digit > 9)
                s_pcDebugOut[pos] = 'a'+digit-10;
            else
                s_pcDebugOut[pos] = '0'+digit;
            num >>= 4;
        }
        OutputDebugStringA(s_pcDebugOut);
        s_pOutputMutex->Leave();
    }
#endif
#ifdef PRFDATA
    PrfCountOOM(1);
#endif
    if (s_pOutOfMemException)
        s_pOutOfMemException->throwE();
    else
        raiseException(null);
}

/**
 * Gets exception string
 */
String * Exception::toString()
{
    // This method is used in _dispatch for returning error messages to external
    // script clients, therefore we no longer prepend the class name to the message
    // since this is useless for external customers.
    String * message = getMessage();
    if (message) return message;        

    String * s = getClass()->getName();
    return s;
}


/**
 * Gets detailed error message
 */
String * Exception::getMessage() 
{
    Model model(this->model());
    TRY
    {
        if (detailMessage == null && hr != S_OK)
        {
            detailMessage = Resources::FormatSystemMessage(hr);
        }
    }
    CATCH
    {
        model.Release();
        Exception::throwAgain();
    }
    ENDTRY
    return detailMessage;
}

void Exception::addDetail(String* moreDetail)
{
    // don't try to set it on the static out of memory exception...
    if (this == s_pOutOfMemException)
        return;
    Model model(this->model());
    TRY
    {
        if (moreDetail->model() != this->model())
        {
            // have to create a complete new copy of the string in the right model
            moreDetail = moreDetail->copyString();
        }
        detailMessage = String::add(moreDetail, (String*)detailMessage, null);
    }
    CATCH
    {
        model.Release();
        Exception::throwAgain();
    }
    ENDTRY
}

/**
 * Gets HRESULT
 */
HRESULT Exception::getHRESULT()
{
    return hr;
}


void Exception::setSourceText(String * pSourceText)
{
    // don't try to set it on the static out of memory exception...
    if (this == s_pOutOfMemException)
        return;
    if (pSourceText && pSourceText->model() != model())
    {
        Model model(model());
        TRY
        {
            // have to create a complete new copy of the string in the right model
            pSourceText = pSourceText->copyString();
        }
        CATCH
        {
            model.Release();
            Exception::throwAgain();
        }
        ENDTRY
    }
    _pSourceText = pSourceText;
}


void Exception::setUrl(String * pUrl)
{
    // don't try to set it on the static out of memory exception...
    if (this == s_pOutOfMemException)
        return;
    if (pUrl && pUrl->model() != model())
    {
        Model model(model());
        TRY
        {
            // have to create a complete new copy of the string in the right model
            pUrl = pUrl->copyString();
        }
        CATCH
        {
            model.Release();
            Exception::throwAgain();
        }
        ENDTRY
    }
    _pUrl = pUrl;
}
