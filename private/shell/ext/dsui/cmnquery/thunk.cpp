#include "pch.h"
#pragma hdrstop

#ifdef UNICODE

/*-----------------------------------------------------------------------------
/ CPersistQueryA2W
/----------------------------------------------------------------------------*/

CPersistQueryA2W::CPersistQueryA2W(IPersistQueryW* pPersistQuery)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::CPersistQueryA2W");
    TraceAssert(pPersistQuery);

    pPersistQuery->AddRef();
    m_pPersistQuery = pPersistQuery;

    TraceLeave();
}

CPersistQueryA2W::~CPersistQueryA2W()
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::~CPersistQueryA2W");

    DoRelease(m_pPersistQuery);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CPersistQueryA2W
#include "unknown.inc"

STDMETHODIMP CPersistQueryA2W::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IPersist, (LPPERSIST)this,
        &IID_IPersistQueryA, (IPersistQueryA*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::GetClassID(THIS_ CLSID* pClassID)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::GetClassID");
    HRESULT hr = m_pPersistQuery->GetClassID(pClassID);
    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::WriteString(THIS_ LPCSTR pSection, LPCSTR pKey, LPCSTR pValue)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::WriteString");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pValue == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("pValue %s"), A2T(pValue));

    hr = m_pPersistQuery->WriteString(A2W(pSection), A2W(pKey), A2W(pValue));
    FailGracefully(hr, "Failed when calling IPersistQueryW::WriteString");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::ReadString(THIS_ LPCSTR pSection, LPCSTR pKey, LPSTR pBuffer, INT cchBuffer)
{
    HRESULT hr;
    WCHAR szBufferW[MAX_PATH];
    LPWSTR pBufferW = NULL;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::WriteString");

    if ( !pSection || !pKey || !pBuffer )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pBuffer == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("pBuffer %s, cchBuffer %d"), pBuffer, cchBuffer);
    
    if ( cchBuffer > ARRAYSIZE(szBufferW) )
    {   
        // allocate a buffer lager than our stack based on, to read into before
        // doing the conversion back to the caller supplied one.

        pBufferW = (LPWSTR)LocalAlloc(LPTR, cchBuffer*SIZEOF(WCHAR));
        TraceAssert(pBufferW);
        
        if ( !pBufferW )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate string");

        hr = m_pPersistQuery->ReadString(A2W(pSection), A2W(pKey), pBufferW, cchBuffer);
        FailGracefully(hr, "Failed when read into stack buffer via IPersistQueryW::ReadString");
        
        WideCharToMultiByte(CP_ACP, 0, pBufferW, -1, pBuffer, cchBuffer, NULL, NULL);
    }
    else
    {
        // for performance we optimise the case of the buffer specified < MAX_PATH,
        // as this should be quite common, instead of jumping through hoops to allocate
        // a buffer we just have a stack based one.

        hr = m_pPersistQuery->ReadString(A2W(pSection), A2W(pKey), szBufferW, ARRAYSIZE(szBufferW));
        FailGracefully(hr, "Failed when read into stack buffer via IPersistQueryW::ReadString");

        WideCharToMultiByte(CP_ACP, 0, szBufferW, -1, pBuffer, cchBuffer, NULL, NULL);
    }

    hr = S_OK;                      // success

exit_gracefully:
    
    if ( pBufferW )
        LocalFree(pBufferW);

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::WriteInt(THIS_ LPCSTR pSection, LPCSTR pKey, INT value)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::WriteInt");

    if ( !pSection || !pKey )
        ExitGracefully(hr, E_INVALIDARG, "pSection or pKey == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("value %d"), value);

    hr = m_pPersistQuery->WriteInt(A2W(pSection), A2W(pKey), value);
    FailGracefully(hr, "Failed when calling IPersistQueryW::WriteInt");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::ReadInt(THIS_ LPCSTR pSection, LPCSTR pKey, LPINT pValue)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::ReadInt");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pValue == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("pValue %08x"), pValue);

    hr = m_pPersistQuery->ReadInt(A2W(pSection), A2W(pKey), pValue);
    FailGracefully(hr, "Failed when calling IPersistQueryW::ReadInt");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::WriteStruct(THIS_ LPCSTR pSection, LPCSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::WriteStruct");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pStruct == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("pStruct %s, cbStruct %08x"), pStruct, cbStruct);

    hr = m_pPersistQuery->WriteStruct(A2W(pSection), A2W(pKey), pStruct, cbStruct);
    FailGracefully(hr, "Failed when calling IPersistQueryW::WriteStruct");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::ReadStruct(THIS_ LPCSTR pSection, LPCSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::ReadStrcut");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pStruct == NULL");

    Trace(TEXT("pSection %s, pKey %s"), A2T(pSection), A2T(pKey));
    Trace(TEXT("pStruct %s, cbStruct %08x"), pStruct, cbStruct);

    hr = m_pPersistQuery->ReadStruct(A2W(pSection), A2W(pKey), pStruct, cbStruct);
    FailGracefully(hr, "Failed when calling IPersistQueryW::ReadStruct");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryA2W::Clear(THIS)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::Clear");
    HRESULT hr = m_pPersistQuery->Clear();
    TraceLeaveResult(hr);    
}


/*-----------------------------------------------------------------------------
/ CPersistQueryW2A
/----------------------------------------------------------------------------*/

CPersistQueryW2A::CPersistQueryW2A(IPersistQueryA* pPersistQuery)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::CPersistQueryW2A");
    TraceAssert(pPersistQuery);

    pPersistQuery->AddRef();
    m_pPersistQuery = pPersistQuery;

    TraceLeave();
}

CPersistQueryW2A::~CPersistQueryW2A()
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::~CPersistQueryW2A");

    DoRelease(m_pPersistQuery);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CPersistQueryW2A
#include "unknown.inc"

STDMETHODIMP CPersistQueryW2A::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IPersist, (LPPERSIST)this,
        &IID_IPersistQueryW, (IPersistQueryW*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::GetClassID(THIS_ CLSID* pClassID)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryA2W::GetClassID");
    HRESULT hr = m_pPersistQuery->GetClassID(pClassID);
    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::WriteString(THIS_ LPCWSTR pSection, LPCWSTR pKey, LPCWSTR pValue)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::WriteString");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pValue == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("pValue %s"), pValue);

    hr = m_pPersistQuery->WriteString(W2A(pSection), W2A(pKey), W2A(pValue));
    FailGracefully(hr, "Failed when calling IPersistQueryA::WriteString");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::ReadString(THIS_ LPCWSTR pSection, LPCWSTR pKey, LPWSTR pBuffer, INT cchBuffer)
{
    HRESULT hr;
    CHAR szBufferA[MAX_PATH];
    LPSTR pBufferA = NULL;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::WriteString");

    if ( !pSection || !pKey || !pBuffer )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pBuffer == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("pBuffer %s, cchBuffer %d"), pBuffer, cchBuffer);
    
    if ( cchBuffer > ARRAYSIZE(szBufferA) )
    {   
        // allocate a buffer lager than our stack based on, to read into before
        // doing the conversion back to the caller supplied one.

        pBufferA = (LPSTR)LocalAlloc(LPTR, SIZEOF(CHAR)*cchBuffer);
        TraceAssert(pBufferA);

        if ( !pBufferA )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to free string");

        hr = m_pPersistQuery->ReadString(W2A(pSection), W2A(pKey), pBufferA, cchBuffer);
        FailGracefully(hr, "Failed when read into stack buffer via IPersistQueryA::ReadString");
        
        MultiByteToWideChar(CP_ACP, 0, pBufferA, -1, pBuffer, cchBuffer); 
    }
    else
    {
        // for performance we optimise the case of the buffer specified < MAX_PATH,
        // as this should be quite common, instead of jumping through hoops to allocate
        // a buffer we just have a stack based one.

        hr = m_pPersistQuery->ReadString(W2A(pSection), W2A(pKey), szBufferA, ARRAYSIZE(szBufferA));
        FailGracefully(hr, "Failed when read into stack buffer via IPersistQueryA::ReadString");

        MultiByteToWideChar(CP_ACP, 0, szBufferA, -1, pBuffer, cchBuffer); 
    }

    hr = S_OK;                      // success

exit_gracefully:
    
    if ( pBufferA )
        LocalFree(pBufferA);

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::WriteInt(THIS_ LPCWSTR pSection, LPCWSTR pKey, INT value)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::WriteInt");

    if ( !pSection || !pKey )
        ExitGracefully(hr, E_INVALIDARG, "pSection or pKey == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("value %d"), value);

    hr = m_pPersistQuery->WriteInt(W2A(pSection), W2A(pKey), value);
    FailGracefully(hr, "Failed when calling IPersistQueryA::WriteInt");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::ReadInt(THIS_ LPCWSTR pSection, LPCWSTR pKey, LPINT pValue)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::ReadInt");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pValue == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("pValue %08x"), pValue);

    hr = m_pPersistQuery->ReadInt(W2A(pSection), W2A(pKey), pValue);
    FailGracefully(hr, "Failed when calling IPersistQueryA::ReadInt");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::WriteStruct(THIS_ LPCWSTR pSection, LPCWSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::WriteStruct");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pStruct == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("pStruct %s, cbStruct %08x"), pStruct, cbStruct);

    hr = m_pPersistQuery->WriteStruct(W2A(pSection), W2A(pKey), pStruct, cbStruct);
    FailGracefully(hr, "Failed when calling IPersistQueryA::WriteStruct");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::ReadStruct(THIS_ LPCWSTR pSection, LPCWSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;
    USES_CONVERSION;
    
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::ReadStrcut");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pStruct == NULL");

    Trace(TEXT("pSection %s, pKey %s"), pSection, pKey);
    Trace(TEXT("pStruct %s, cbStruct %08x"), pStruct, cbStruct);

    hr = m_pPersistQuery->ReadStruct(W2A(pSection), W2A(pKey), pStruct, cbStruct);
    FailGracefully(hr, "Failed when calling IPersistQueryA::ReadStruct");

    // hr = S_OK;                      // success

exit_gracefully:
    
    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CPersistQueryW2A::Clear(THIS)
{
    TraceEnter(TRACE_PERSIST, "CPersistQueryW2A::Clear");
    HRESULT hr = m_pPersistQuery->Clear();
    TraceLeaveResult(hr);    
}


#endif