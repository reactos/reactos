#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Static data and other helper functions
/----------------------------------------------------------------------------*/

#define STRING_SIZE     TEXT("%sLength")
#define STRING_VALUE    TEXT("%sValue")


/*-----------------------------------------------------------------------------
/ CDsPersistQuery
/----------------------------------------------------------------------------*/

CDsPersistQuery::CDsPersistQuery(LPCTSTR pFilename)
{
    TraceEnter(TRACE_IO, "CDsPersistQuery::CDsPersistQuery");
    Trace(TEXT("pFilename: %s"), pFilename);

    StrCpy(m_szFilename, pFilename);               // copy the filename

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CDsPersistQuery
#include "unknown.inc"

STDMETHODIMP CDsPersistQuery::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IPersist, (LPPERSIST)this,
        &IID_IPersistQuery, (IPersistQuery*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::GetClassID(THIS_ CLSID* pClassID)
{
    TraceEnter(TRACE_IO, "CDsPersistQuery::GetClassID");
    TraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::WriteString(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPCTSTR pValue)
{
    HRESULT hr;
    INT cchValue;
    TCHAR szBuffer[MAX_PATH];
    WCHAR szValueW[MAX_PATH];
    LPWSTR pValueW = NULL;

    TraceEnter(TRACE_IO, "CDsPersistQuery::WriteString");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "pSection, pKey or pValue == NULL");

    cchValue = 1+lstrlen(pValue);
    Trace(TEXT("pSection: %s, pKey: %s, pValue: %s (%d)"), pSection, pKey, pValue, cchValue);

    // Write the string into the stream as a UNICODE string. If we are built for UNICODE
    // then we can simply WriteProfileStruct, otherwise we must attempt to convert it
    // from a multi-byte string.

    // Write the string size

    wsprintf(szBuffer, STRING_SIZE, pKey);

    if ( !WritePrivateProfileStruct(pSection, szBuffer, &cchValue, SIZEOF(cchValue), m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to write string size to stream");

    wsprintf(szBuffer, STRING_VALUE, pKey);

    // Write the value (thunking as required to ensure its UNICODE in the stream)

#ifdef UNICODE
    if ( !WritePrivateProfileStruct(pSection, szBuffer, (LPVOID)pValue, SIZEOF(WCHAR)*cchValue, m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to write string to stream");
#else
    if ( cchValue < ARRAYSIZE(szValueW) )
    {
        MultiByteToWideChar(CP_ACP, 0, pValue, -1, szValueW, ARRAYSIZE(szValueW));

        if ( !WritePrivateProfileStruct(pSection, szBuffer, szValueW, SIZEOF(WCHAR)*cchValue, m_szFilename) )
            ExitGracefully(hr, E_FAIL, "Failed to write _small_ thunked string");
    }
    else
    {
        hr = LocalAllocStringLenW(&pValueW, cchValue);
        FailGracefully(hr, "Failed to allocate buffer to thunk string into");

        MultiByteToWideChar(CP_ACP, 0, pValue, -1, pValueW, cchValue);

        if ( !WritePrivateProfileStruct(pSection, szBuffer, pValueW, SIZEOF(WCHAR)*cchValue, m_szFilename) )
            ExitGracefully(hr, E_FAIL, "Failed tow write _large_ thunked string");       
    }
#endif

    hr = S_OK;

exit_gracefully:

#ifdef UNICODE    
    LocalFreeStringW(&pValueW);
#endif

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::ReadString(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPTSTR pBuffer, INT cchBuffer)
{
    HRESULT hr;
    TCHAR szBuffer[MAX_PATH];
    INT cchValue;
    WCHAR szBufferW[MAX_PATH];
    LPWSTR pBufferW = NULL;

    TraceEnter(TRACE_IO, "CDsPersistQuery::ReadString");

    if ( !pSection || !pKey || !pBuffer )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to read (or into)");

    pBuffer[0] = TEXT('\0');            // terminate the buffer

    Trace(TEXT("pSection: %s, pKey: %s"), pSection, pKey);

    // Read the length of the string, checking to see if its fits in the the buffer
    // we have, if it does then read and convert as requried.

    wsprintf(szBuffer, STRING_SIZE, pKey);              // <key name>Length
    Trace(TEXT("Opening key: %s"), szBuffer);
        
    if ( !GetPrivateProfileStruct(pSection, szBuffer, &cchValue, SIZEOF(cchValue), m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to read string size");

    Trace(TEXT("cchValue %d"), cchValue);

    if ( cchValue > cchBuffer )
        ExitGracefully(hr, E_FAIL, "Buffer too small for string in stream");

    if ( cchValue > 0 )
    {
        // Read the string, if we are built UNICODE then just get it from the stream,
        // otherwise read it and thunk accordingly.

        wsprintf(szBuffer, STRING_VALUE, pKey);

#ifdef UNICODE
        if ( !GetPrivateProfileStruct(pSection, szBuffer, pBuffer, SIZEOF(WCHAR)*cchValue, m_szFilename) )
            ExitGracefully(hr, E_FAIL, "Failed to read string data");    
#else
        if ( cchValue < ARRAYSIZE(szBufferW) )
        {
            if ( !GetPrivateProfileStruct(pSection, szBuffer, szBufferW, SIZEOF(WCHAR)*cchValue, m_szFilename) )
                ExitGracefully(hr, E_FAIL, "Failed to read string size");    

            WideCharToMultiByte(CP_ACP, 0, szBufferW, -1, pBuffer, cchValue, NULL, NULL);
        }
        else
        {
            hr = LocalAllocStringLenW(&pBufferW, cchValue);
            FailGracefully(hr, "Failed to allocate thunking buffer for read");

            if ( !GetPrivateProfileStruct(pSection, szBuffer, pBufferW, SIZEOF(WCHAR)*cchValue, m_szFilename) )
                ExitGracefully(hr, E_FAIL, "Failed to read string size");    

            WideCharToMultiByte(CP_ACP, 0, pBufferW, -1, pBuffer, cchValue, NULL, NULL);
        }
#endif
    }

    Trace(TEXT("Value is: %s"), pBuffer);
    hr = S_OK;

exit_gracefully:

#ifdef UNICODE
    LocalFreeStringW(&pBufferW);
#endif

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::WriteInt(THIS_ LPCTSTR pSection, LPCTSTR pKey, INT value)
{
    HRESULT hr;

    TraceEnter(TRACE_IO, "CDsPersistQuery::WriteInt");

    if ( !pSection || !pKey )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to write");

    Trace(TEXT("pSection: %s, pKey: %s, value: %d"), pSection, pKey, value);

    if ( !WritePrivateProfileStruct(pSection, pKey, &value, SIZEOF(value), m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to write value");

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::ReadInt(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPINT pValue)
{
    HRESULT hr;

    TraceEnter(TRACE_IO, "CDsPersistQuery::ReadInt");

    if ( !pSection || !pKey || !pValue )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to read");

    Trace(TEXT("pSection: %s, pKey: %s, pValue: %08x"), pSection, pKey, pValue);

    if ( !GetPrivateProfileStruct(pSection, pKey, pValue, SIZEOF(*pValue), m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to read value");

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::WriteStruct(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;

    TraceEnter(TRACE_IO, "CDsPersistQuery::WriteStruct");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to write");

    Trace(TEXT("pSection: %s, pKey: %s, pStruct: %08x, cbStruct: %d"), pSection, pKey, pStruct, cbStruct);

    if ( !WritePrivateProfileStruct(pSection, pKey, pStruct, cbStruct, m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to write struct");

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::ReadStruct(THIS_ LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    HRESULT hr;

    TraceEnter(TRACE_IO, "CDsPersistQuery::ReadStruct");

    if ( !pSection || !pKey || !pStruct )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to read");

    Trace(TEXT("pSection: %s, pKey: %s, pStruct: %08x, cbStruct: %d"), pSection, pKey, pStruct, cbStruct);

    if ( !GetPrivateProfileStruct(pSection, pKey, pStruct, cbStruct, m_szFilename) )
        ExitGracefully(hr, E_FAIL, "Failed to read struct");

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPersistQuery::Clear(THIS)
{
    TraceEnter(TRACE_IO, "CDsPersistQuery::Clear");
    TraceLeaveResult(S_OK);
}
