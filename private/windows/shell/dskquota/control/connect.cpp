///////////////////////////////////////////////////////////////////////////////
/*  File: connect.cpp

    Description: Contains class definitions for classes associated with
        OLE connection points.  These are:

            ConnectionPoint         ( IConnectionPoint )
            ConnectionPointEnum     ( IEnumConnectionPoints )
            ConnectionEnum          ( IEnumConnections )


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"  // PCH
#pragma hdrstop

#include "connect.h"
#include "guidsp.h"

//
// Constants for connection point-related objects.
//
const UINT CONNECTION_FIRST_COOKIE = 100; // 1st cookie value given out.

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::ConnectionPoint

    Description: Constructor,

    Arguments:
        pUnkContainer - Pointer to containing DiskQuotaController object.

        riid - Reference to IID that this connection point object supports.

    Returns: Nothing.

    Exceptions: CAllocException, CSyncException

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
    09/06/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionPoint::ConnectionPoint(
    LPUNKNOWN pUnkContainer,
    REFIID riid
    ) : m_cRef(0),
        m_cConnections(0),
        m_dwCookieNext(CONNECTION_FIRST_COOKIE),
        m_pUnkContainer(pUnkContainer),
        m_riid(riid),
        m_hMutex(NULL)
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPoint::ConnectionPoint")));
    DBGASSERT((NULL != pUnkContainer));

    if (NULL == (m_hMutex = CreateMutex(NULL, FALSE, NULL)))
        throw CSyncException(CSyncException::mutex, CSyncException::create);

    m_Dispatch.Initialize(static_cast<IDispatch *>(this),
                          LIBID_DiskQuotaTypeLibrary,
                          IID_DIDiskQuotaControlEvents,
                          L"DSKQUOTA.DLL");
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::~ConnectionPoint

    Description: Destructor.  

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionPoint::~ConnectionPoint(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPoint::~ConnectionPoint")));

    Lock();
    UINT cConnections = m_ConnectionList.Count();
    for (UINT i = 0; i < cConnections; i++)
    {
        if (NULL != m_ConnectionList[i].pUnk)
        {
            try
            {
                m_ConnectionList[i].pUnk->Release();
            }
            catch(...)
            {
                DBGERROR((TEXT("C++ exception while calling Release in ConnectionPoint::~ConnectionPoint")));
            }
        }
    }
    ReleaseLock();
        
    if (NULL != m_hMutex)
        CloseHandle(m_hMutex);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or 
        IConnectionPoint interface.  Only IID_IUnknown and 
        IID_IConnectionPoint are recognized.  The object referenced by the 
        returned interface pointer is uninitialized.  The recipient of the 
        pointer must call Initialize() before the object is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR        - Success.
        E_NOINTERFACE  - Requested interface not supported.
        E_INVALIDARG   - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionPoint::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPoint::QueryInterface")));
    DBGPRINTIID(DM_CONNPT, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IConnectionPoint == riid)
    {
        *ppvOut = static_cast<IConnectionPoint *>(this);
    }
    else if (IID_IDispatch == riid)
    {
        *ppvOut = static_cast<IDispatch *>(this);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionPoint::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPoint::AddRef")));
    DBGPRINT((DM_CONNPT, DL_LOW, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);

    //
    // NOTE:  We maintain a pointer to the quota controller (m_pUnkContainer) but
    //        we DO NOT AddRef it.  The controller calls AddRef for connection
    //        point objects so this would create a circular reference.
    //

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionPoint::Release(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPoint::Release")));
    DBGPRINT((DM_COM, DL_HIGH, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::GetConnectionInterface

    Description: Retrieves a connection point's interface ID.

    Arguments:
        pIID - Address of IID variable to receive the IID.

    Returns:
        NOERROR      - Success.
        E_INVALIDARG - pIID is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ConnectionPoint::GetConnectionInterface(
    LPIID pIID
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::GetConnectionInterface")));
    HRESULT hr = E_INVALIDARG;

    if (NULL != pIID)
    {
        *pIID = m_riid;
        hr = NOERROR;
    }

    return hr;
}

   
///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::GetConnectionPointContainer

    Description: Retrieves an interface pointer for the point's parent
        container.

    Arguments:
        ppCPC - Address of variable to receive container's interface pointer
            value.

    Returns:
        NOERROR     - Success.
        E_INVALIDARG - ppCPC argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ConnectionPoint::GetConnectionPointContainer(
    PCONNECTIONPOINTCONTAINER *ppCPC
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::GetConnectionPointContainer")));
    return m_pUnkContainer->QueryInterface(IID_IConnectionPointContainer, 
                                       (LPVOID *)ppCPC);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::Advise

    Description: Notifies the connection point of an outgoing interface.

    Arguments:
        pUnkSink - Pointer to outgoing interface provided by Sink.

        pdwCookie - Address of variable to receive the "cookie" returned
            for this connection.  The client uses this "cookie" value to
            refer to the connection.

    Returns:
        NOERROR                 - Success.
        E_INVALIDARG            - pUnkSink or pdwCookie were NULL.
        CONNECT_E_CANNOTCONNECT - Sink doesn't support our event interface.
        E_UNEXPECTED            - Exception caught while calling client code.
        E_OUTOFMEMORY           - Insufficient memory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/21/96    Initial creation.                                    BrianAu
    09/06/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ConnectionPoint::Advise(
    LPUNKNOWN pUnkSink,
    LPDWORD pdwCookie
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::Advise")));
    DBGPRINT((DM_CONNPT, DL_MID, TEXT("\tAdvising connpt 0x%08X of sink 0x%08X"), 
             this, pUnkSink));

    HRESULT hr = NOERROR;
    if (NULL == pUnkSink || NULL == pdwCookie)
        return E_INVALIDARG;

    LPUNKNOWN pSink = NULL;
    AutoLockMutex lock(m_hMutex);

    //
    // Does the sink support our conn pt interface?
    //
    try
    {
        //
        // QueryInterface() is client code.  Must handle exceptions.
        //
        hr = pUnkSink->QueryInterface(m_riid, (LPVOID *)&pSink);

        if (SUCCEEDED(hr))
        {
            CONNECTDATA cd;

            //
            // See if there is an unused entry in the list.
            // If not, we'll have to extend the list.
            //
            UINT index = m_cConnections;
            for (UINT i = 0; i < m_cConnections; i++)
            {
                if (NULL == m_ConnectionList[i].pUnk)
                {
                    index = i;
                    break;
                }
            }

            //
            // Fill in the connection info and add to connection list.
            //
            cd.pUnk    = pSink;
            *pdwCookie = cd.dwCookie = m_dwCookieNext++;

            if (index < m_cConnections)
                m_ConnectionList[index] = cd;
            else
                m_ConnectionList.Append(cd); // This can throw OutOfMemory.

            if (SUCCEEDED(hr))
            {
                m_cConnections++;  // Another connection.
                DBGPRINT((DM_CONNPT, DL_HIGH, 
                         TEXT("CONNPT - Connection complete.  Cookie = %d.  %d total connections."),
                         *pdwCookie, m_cConnections));
            }
            else
            {
                DBGERROR((TEXT("ConnPt connection failed with error 0x%08X."), hr));
            }
        }
        else
            hr = CONNECT_E_CANNOTCONNECT;  // Interface not supported.
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }
    
    if (FAILED(hr) && NULL != pSink)
    {
        //
        // Something failed after QueryInterface.  Release sink pointer.
        //
        try
        {
            //
            // IUnknown::Release() is client code.  Must handle exceptions.
            //
            pSink->Release();
        }
        catch(...)
        {
            DBGERROR((TEXT("C++ exception while calling Release() in ConnectionPoint::Advise")));
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::Unadvise

    Description: Disconntinues an outgoing communication channel with the 
        connection point object.

    Arguments:
        dwCookie - The "channel" identifier returned from Advise().

    Returns:
        NOERROR                 - Success.
        CONNECT_E_NOCONNECTION  - No connection found for this cookie.
        E_UNEXPECTED            - Exception caught while calling client code.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/21/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ConnectionPoint::Unadvise(
    DWORD dwCookie
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::Unadvise")));
    DBGPRINT((DM_CONNPT, DL_MID, TEXT("\tUnadvising connpt 0x%08X of cookie %d"), 
             this, dwCookie));

    HRESULT hr = CONNECT_E_NOCONNECTION;
    
    if (0 != dwCookie)
    {
        AutoLockMutex lock(m_hMutex);
        for (UINT i = 0; i < m_cConnections; i++)
        {
            if (m_ConnectionList[i].dwCookie == dwCookie)
            {
                //
                // Matching cookie found.  Release interface, mark connection
                // list entry as unused.
                //
                hr = NOERROR;

                try
                {
                    //
                    // IUnknown::Release() is client code.  Handle exceptions.
                    //
                    m_ConnectionList[i].pUnk->Release();
                }
                catch(...)
                {
                    DBGERROR((TEXT("C++ exception while calling Release() in ConnectionPoint::Uadvise")));
                    hr = E_UNEXPECTED;
                }

                m_ConnectionList[i].pUnk     = NULL;
                m_ConnectionList[i].dwCookie = 0;
                m_cConnections--;
                DBGPRINT((DM_CONNPT, DL_HIGH, TEXT("CONNPT - Connection terminated for cookie %d.  %d total connections"),
                         dwCookie, m_cConnections));        
                break;
            }   
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPoint::EnumConnections

    Description: Retrieves in interface pointer to a connection enumerator
        which enumerates all connections associated with this connection 
        point.

    Arguments:
        ppEnum - Address of interface pointer variable to received address of
            the enumerator's IEnumConnection interface.

    Returns:
        NOERROR        - Success.
        E_INVALIDARG   - ppEnum was NULL.
        E_OUTOFMEMORY  - Insufficient memory to create enumerator.
        E_UNEXPECTED   - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/21/96    Initial creation.                                    BrianAu
    09/06/96    Added exception handling.                            BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
ConnectionPoint::EnumConnections(
    PENUMCONNECTIONS *ppEnum
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::EnumConnections")));

    HRESULT hr = NOERROR;

    if (NULL == ppEnum)
        return E_INVALIDARG;

    ConnectionEnum *pEnum = NULL;
    AutoLockMutex lock(m_hMutex);

    try
    {
        array_autoptr<CONNECTDATA> ptrCD;
        PCONNECTDATA pcd = NULL;                
        //
        // Build temp array of connection data to initialize enumerator.
        // Note: If m_cConnections == 0, we still return an enumerator
        //       but it's uninitialized.  Calls to Next and Skip will always
        //       return S_FALSE so the enumerator is just viewed as "empty".
        //
        if (0 != m_cConnections)
        {
            ptrCD = new CONNECTDATA[m_cConnections];
            pcd = ptrCD.get();

            //
            // Transfer connection info to temp array for initializting
            // the enumerator object.
            // Remember, the connection list can have unused entries so
            // cConnListEntries can be greater than m_cConnections.
            //
            UINT cConnListEntries = m_ConnectionList.Count();
            for (UINT i = 0, j = 0; i < cConnListEntries; i++)
            {
                DBGASSERT((j < m_cConnections));
                *(pcd + j) = m_ConnectionList[i];  
                if (NULL != pcd[j].pUnk)
                    j++;
            }
        }

        //
        // Create the enumerator object.
        // The enumerator keeps a copy of the connection's
        // IUnknown pointer.  Note that we still create an 
        // enumerator even if m_cConnections is 0.  It's just an 
        // empty enumerator.  If m_cConnections is 0, pcd can be NULL.
        //
        DBGASSERT((m_cConnections ? NULL != pcd : TRUE));
        pEnum = new ConnectionEnum(static_cast<IConnectionPoint *>(this), m_cConnections, pcd);

        hr = pEnum->QueryInterface(IID_IEnumConnections, 
                                       (LPVOID *)ppEnum);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        delete pEnum;
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        //
        // Catch any exceptions than might have been thrown by user
        // code during the call to AddRef() in ConnectionEnum ctor.
        //
        DBGERROR((TEXT("Unexpected C++ exception")));
        delete pEnum;
        hr = E_UNEXPECTED;
    }

    return hr;
}


//
// IDispatch::GetIDsOfNames
//
STDMETHODIMP
ConnectionPoint::GetIDsOfNames(
    REFIID riid,  
    OLECHAR **rgszNames,  
    UINT cNames,  
    LCID lcid,  
    DISPID *rgDispId
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::GetIDsOfNames")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetIDsOfNames(riid,
                                    rgszNames,
                                    cNames,
                                    lcid,
                                    rgDispId);
}


//
// IDispatch::GetTypeInfo
//
STDMETHODIMP
ConnectionPoint::GetTypeInfo(
    UINT iTInfo,  
    LCID lcid,  
    ITypeInfo **ppTypeInfo
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::GetTypeInfo")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfo(iTInfo, lcid, ppTypeInfo);
}


//
// IDispatch::GetTypeInfoCount
//
STDMETHODIMP
ConnectionPoint::GetTypeInfoCount(
    UINT *pctinfo
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::GetTypeInfoCount")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.GetTypeInfoCount(pctinfo);
}


//
// IDispatch::Invoke
//
STDMETHODIMP
ConnectionPoint::Invoke(
    DISPID dispIdMember,  
    REFIID riid,  
    LCID lcid,  
    WORD wFlags,  
    DISPPARAMS *pDispParams,  
    VARIANT *pVarResult,  
    EXCEPINFO *pExcepInfo,  
    UINT *puArgErr
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionPoint::Invoke")));
    //
    // Let our dispatch object handle this.
    //
    return m_Dispatch.Invoke(dispIdMember,
                             riid,
                             lcid,
                             wFlags,
                             pDispParams,
                             pVarResult,
                             pExcepInfo,
                             puArgErr);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::ConnectionEnum

    Description: Constructor,

    Arguments:
        pUnkContainer - Pointer to the IUnknown interface of the containing
            object.

        cConnections - Number of connections in array pointed to by rgConnections.

        rgConnections - Array of connection information used to
            initialize the enumerator.

    Returns: Nothing.

    Exceptions: CAllocException.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
    09/06/06    Added copy constructor.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionEnum::ConnectionEnum(
    LPUNKNOWN pUnkContainer,
    UINT cConnections, 
    PCONNECTDATA rgConnections
    ) : m_cRef(0),
        m_iCurrent(0),
        m_cConnections(0),
        m_rgConnections(NULL),
        m_pUnkContainer(pUnkContainer)
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionEnum::ConnectionEnum")));
    DBGASSERT((NULL != pUnkContainer));

    if (0 != cConnections)
    {
        m_rgConnections = new CONNECTDATA[cConnections];

        DBGASSERT((NULL != rgConnections));

        for (UINT i = 0; i < cConnections; i++)
        {
            //
            // IUnknown::AddRef() is client code. It can generate an exception.
            // Caller must catch and handle it.
            //
            rgConnections[i].pUnk->AddRef();
            m_rgConnections[i].pUnk     = rgConnections[i].pUnk;
            m_rgConnections[i].dwCookie = rgConnections[i].dwCookie;
            m_cConnections++;
        }
    }
}


ConnectionEnum::ConnectionEnum(const ConnectionEnum& refEnum)
    : m_cRef(0),
      m_iCurrent(0),
      m_cConnections(0),
      m_rgConnections(NULL),
      m_pUnkContainer(m_pUnkContainer)
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionEnum::ConnectionEnum [copy]")));

    if (0 != m_cConnections)
    {
        m_rgConnections = new CONNECTDATA[m_cConnections];

        DBGASSERT((NULL != refEnum.m_rgConnections));
        for (UINT i = 0; i < m_cConnections; i++)
        {
            //
            // IUnknown::AddRef() is client code. It can generate an exception.
            // Caller must catch and handle it.
            //
            refEnum.m_rgConnections[i].pUnk->AddRef();
            m_rgConnections[i].pUnk     = refEnum.m_rgConnections[i].pUnk;
            m_rgConnections[i].dwCookie = refEnum.m_rgConnections[i].dwCookie;
            m_cConnections++;
        }
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::~ConnectionEnum

    Description: Destructor.  Releases all connection sink interface pointers
        held in enumerator's array.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionEnum::~ConnectionEnum(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionEnum::~ConnectionEnum")));

    if (NULL != m_rgConnections)
    {
        for (UINT i = 0; i < m_cConnections; i++)
        {
            if (NULL != m_rgConnections[i].pUnk)
            {
                try
                {
                    //
                    // Release() is client code.  Handle exceptions.
                    //
                    m_rgConnections[i].pUnk->Release();
                }
                catch(...)
                {
                    DBGERROR((TEXT("C++ exception while calling Release() in ConnectionPoint::~ConnectionEnum")));
                }
                    
                m_rgConnections[i].pUnk = NULL;
            }
        }
        delete[] m_rgConnections;
    }
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or 
        IEnumConnections interface.  Only IID_IUnknown and 
        IID_IEnumConnections are recognized.  The object referenced by the 
        returned interface pointer is uninitialized.  The recipient of the 
        pointer must call Initialize() before the object is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR       - Success.
        E_NOINTERFACE - Requested interface not supported.
        E_INVALIDARG  - ppvOut arg is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionEnum::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionEnum::QueryInterface")));
    DBGPRINTIID(DM_CONNPT, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IEnumConnections == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionEnum::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionEnum::AddRef")));
    DBGPRINT((DM_CONNPT, DL_LOW, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;


    InterlockedIncrement(&m_cRef);

    //
    // Increment ref count of connection point so that it stays around
    // while the connection enumerator is alive.
    //
    m_pUnkContainer->AddRef();

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionEnum::Release(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionEnum::Release")));
    DBGPRINT((DM_CONNPT, DL_LOW, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;

    //
    // Decrement ref count of connection point.  We AddRef'd it
    // above.
    //
    m_pUnkContainer->Release();

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::Next

    Description: Retrieve the next cConnections connections supported by
        the enumerator.

    Arguments:
        cConnections - Number of elements in pConnections array.

        pConnections - Array to receive CONNECTDATA data records.

        pcCreated [optional] - Address of DWORD to accept the count of records 
            returned in pConnections. Note that any array locations equal to 
            or beyond the value returned in pcCreated are invalid and set to 
            NULL.

    Returns:
        S_OK          - Success.  Enumerated number of requested connections.
        S_FALSE       - End of enumeration encountered.  Returning less than
                          cConnections records.
        E_INVALIDARG  - pConnections arg is NULL.
        E_UNEXPECTED  - Exception caught.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
ConnectionEnum::Next(
    DWORD cConnections,         // Number of elements in array.
    PCONNECTDATA pConnections,  // Dest array for connection info.
    DWORD *pcCreated            // Return number created.
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionEnum::Next")));

    if (NULL == pConnections)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    DWORD cCreated = 0;

    //
    // Transfer data to caller's array.
    // Stop when at the end of the enumeration or we've
    // returned all that the caller asked for.
    //
    while(m_iCurrent < m_cConnections && cConnections > 0)
    {
        DBGASSERT((NULL != m_rgConnections));
        *pConnections = m_rgConnections[m_iCurrent++];
        if (NULL != pConnections->pUnk)
        {
            try
            {
                //
                // IUnknown::AddRef() is client code.  Handle exceptions.
                //
                pConnections->pUnk->AddRef();
                pConnections++;
                cCreated++;
                cConnections--;
            }
            catch(...)
            {
                DBGERROR((TEXT("C++ exception while calling AddRef() in ConnectionEnum::Next")));
                hr = E_UNEXPECTED;
                break;
            }
        }
    }

    //
    // If requested, return the count of items enumerated.
    //
    if (NULL != pcCreated)
        *pcCreated = cCreated;

    if (cConnections > 0)
    {
        //
        // Less than requested number of connections were retrieved.
        // 
        hr = S_FALSE;
        while(cConnections > 0)
        {
            //
            // Set any un-filled array elements to NULL.
            //
            pConnections->pUnk     = NULL;
            pConnections->dwCookie = 0;
            pConnections++;
            cConnections--;
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::Skip

    Description: Skips a specified number of connections in the enumeration.

    Arguments:
        cConnections - Number of connections to skip.

    Returns:
        S_OK            - Success.  Skipped number of requested items.
        S_FALSE         - End of enumeration encountered.  Skipped less than
                          cConnections items.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionEnum::Skip(
    DWORD cConnections
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionEnum::Skip")));

    while(m_iCurrent < m_cConnections && cConnections > 0)
    {
        m_iCurrent++;
        cConnections--;
    }

    return cConnections == 0 ? S_OK : S_FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::Reset

    Description: Resets the enumerator object so that the next call to Next()
        starts enumerating at the beginning of the enumeration.

    Arguments: None.

    Returns:
        S_OK    - Success.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionEnum::Reset(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionEnum::Reset")));

    m_iCurrent = 0;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionEnum::Clone

    Description: Creates a duplicate of the enumerator object and returns
        a pointer to the new object's IEnumConnections interface.

    Arguments:
        ppEnum - Address of interface pointer variable to accept the pointer
            to the new object's IEnumConnections interface.

    Returns:
        NOERROR        - Success.
        E_OUTOFMEMORY   - Insufficient memory to create new enumerator.
        E_INVALIDARG    - ppEnum arg was NULL.
        E_UNEXPECTED    - Exception caught while calling client code.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionEnum::Clone(
    PENUMCONNECTIONS *ppEnum
    )
{
    DBGTRACE((DM_CONNPT, DL_HIGH, TEXT("ConnectionEnum::Clone")));

    if (NULL == ppEnum)
        return E_INVALIDARG;

    HRESULT hr            = NOERROR;
    ConnectionEnum *pEnum = NULL;

    *ppEnum = NULL;

    try
    {        
        pEnum = new ConnectionEnum((const ConnectionEnum&)*this);

        hr = pEnum->QueryInterface(IID_IEnumConnections, (LPVOID *)ppEnum);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr) && NULL != pEnum)
    {
        delete pEnum;
        *ppEnum = NULL;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::ConnectionPointEnum

    Description: Constructor,

    Arguments:
        pUnkContainer - Pointer to IUnknown of containing object.

        cConnPts - Number of connection points in array pointed to by rgConnPts.

        rgConnPts - Array of connection point object pointers used to
            initialize the enumerator.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
    09/06/96    Added copy constructor.                              BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionPointEnum::ConnectionPointEnum(
    LPUNKNOWN pUnkContainer,
    UINT cConnPts, 
    PCONNECTIONPOINT *rgConnPts
    ) : m_cRef(0),
        m_iCurrent(0),
        m_cConnPts(0),
        m_rgConnPts(NULL),
        m_pUnkContainer(pUnkContainer)
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPointEnum::ConnectionPointEnum")));
    DBGASSERT((NULL != pUnkContainer));

    if (0 != cConnPts)
    {
        m_rgConnPts = new PCONNECTIONPOINT[cConnPts];

        m_cConnPts = cConnPts;
        for (UINT i = 0; i < m_cConnPts; i++)
        {
            m_rgConnPts[i] = rgConnPts[i];
            m_rgConnPts[i]->AddRef();
        }
    }
}

ConnectionPointEnum::ConnectionPointEnum(
    const ConnectionPointEnum& refEnum
    ) : m_cRef(0),
        m_iCurrent(0),
        m_cConnPts(0),
        m_rgConnPts(NULL),
        m_pUnkContainer(refEnum.m_pUnkContainer)
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPointEnum::ConnectionPointEnum [copy]")));

    if (0 != refEnum.m_cConnPts)
    {
        m_rgConnPts = new PCONNECTIONPOINT[refEnum.m_cConnPts];

        m_cConnPts = refEnum.m_cConnPts;
        for (UINT i = 0; i < m_cConnPts; i++)
        {
            m_rgConnPts[i] = refEnum.m_rgConnPts[i];
            m_rgConnPts[i]->AddRef();
        }
    }
}

    

///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::~ConnectionPointEnum

    Description: Destructor.  Releases all connection point object pointers
        held in enumerator's array.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ConnectionPointEnum::~ConnectionPointEnum(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPointEnum::~ConnectionPointEnum")));

    if (NULL != m_rgConnPts)
    {
        for (UINT i = 0; i < m_cConnPts; i++)
        {
            if (NULL != m_rgConnPts[i])
            {
                m_rgConnPts[i]->Release();
                m_rgConnPts[i] = NULL;
            }
        }
        delete[] m_rgConnPts;
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or 
        IEnumConnectionPoints interface.  Only IID_IUnknown and 
        IID_IEnumConnectionPoints are recognized.  The object referenced by the 
        returned interface pointer is uninitialized.  The recipient of the 
        pointer must call Initialize() before the object is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR        - Success.
        E_NOINTERFACE  - Requested interface not supported.
        E_INVALIDARG   - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionPointEnum::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONNPT, DL_MID, TEXT("ConnectionPointEnum::QueryInterface")));
    DBGPRINTIID(DM_CONNPT, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IEnumConnectionPoints == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionPointEnum::AddRef(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::AddRef")));
    DBGPRINT((DM_CONNPT, DL_LOW, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;


    //
    // Increment ref count of QuotaController so that it stays around
    // while the enumerator is alive.
    //
    m_pUnkContainer->AddRef();

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/18/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
ConnectionPointEnum::Release(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::Release")));
    DBGPRINT((DM_COM, DL_HIGH, TEXT("\t0x%08X  %d -> %d\n"),
             this, m_cRef, m_cRef - 1));

    ULONG ulReturn = m_cRef - 1;

    //
    // Decrement ref count of QuotaController.  We AddRef'd it above.
    //
    m_pUnkContainer->Release();

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::Next

    Description: Retrieve the next cConnPts connections supported by
        the enumerator.

    Arguments:
        cConnPts - Number of elements in pConnPts array.

        pConnPts - Array to receive PCONNECTIONPOINT pointers.
         
        pcCreated [optional] - Address of DWORD to accept the count of records 
            returned in pConnPts. Note that any array locations equal to or 
            beyond the value returned in pcCreated are invalid and set to NULL.

    Returns:
        S_OK            - Success.  Enumerated number of requested connection pts.
        S_FALSE         - End of enumeration encountered.  Returning less than
                          cConnPts records.
        E_INVALIDARG    - pConnPts arg is NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT 
ConnectionPointEnum::Next(
    DWORD cConnPts,                 // Number of elements in array.
    PCONNECTIONPOINT *rgpConnPts,   // Dest array for connection point ptrs.
    DWORD *pcCreated                // Return number created.
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::Next")));

    if (NULL == rgpConnPts)
        return E_INVALIDARG;

    HRESULT hr     = S_OK;
    DWORD cCreated = 0;

    //
    // Transfer data to caller's array.
    // Stop when at the end of the enumeration or we've
    // returned all that the caller asked for.
    //
    while(m_iCurrent < m_cConnPts && cConnPts > 0)
    {
        *rgpConnPts = m_rgConnPts[m_iCurrent++];
        if (NULL != *rgpConnPts)
        {
            (*rgpConnPts)->AddRef();
            rgpConnPts++;
            cCreated++;
            cConnPts--;
        }
        else
            DBGASSERT((FALSE));  // Shouldn't hit this.
    }

    //
    // If requested, return the count of items enumerated.
    //
    if (NULL != pcCreated)
        *pcCreated = cCreated;

    if (cConnPts > 0)
    {
        //
        // Less than requested number of connections were retrieved.
        // 
        hr = S_FALSE;
        while(cConnPts > 0)
        {
            //
            // Set any un-filled array elements to NULL.
            //
            *rgpConnPts = NULL;
            rgpConnPts++;
            cConnPts--;
        }
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::Skip

    Description: Skips a specified number of connection pts in the enumeration.

    Arguments:
        cConnPts - Number of connection points to skip.

    Returns:
        S_OK            - Success.  Skipped number of requested items.
        S_FALSE         - End of enumeration encountered.  Skipped less than
                          cConnPts items.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionPointEnum::Skip(
    DWORD cConnPts
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::Skip")));

    while(m_iCurrent < m_cConnPts && cConnPts > 0)
    {
        m_iCurrent++;
        cConnPts--;
    }

    return cConnPts == 0 ? S_OK : S_FALSE;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::Reset

    Description: Resets the enumerator object so that the next call to Next()
        starts enumerating at the start of the enumeration.

    Arguments: None.

    Returns:
        S_OK    - Success.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
ConnectionPointEnum::Reset(
    VOID
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::Reset")));

    m_iCurrent = 0;
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ConnectionPointEnum::Clone

    Description: Creates a duplicate of the enumerator object and returns
        a pointer to the new object's IEnumConnectionPoints interface.

    Arguments:
        ppEnum - Address of interface pointer variable to accept the pointer
            to the new object's IEnumConnectionPoints interface.

    Returns:
        NOERROR         - Success.
        E_OUTOFMEMORY   - Insufficient memory to create new enumerator.
        E_INVALIDARG    - ppEnum arg was NULL.
        E_UNEXPECTED    - Unexpected exception.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/19/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
 ConnectionPointEnum::Clone(
    PENUMCONNECTIONPOINTS *ppEnum
    )
{
    DBGTRACE((DM_CONNPT, DL_LOW, TEXT("ConnectionPointEnum::Clone")));

    if (NULL == ppEnum)
        return E_INVALIDARG;

    HRESULT hr                 = NOERROR;
    ConnectionPointEnum *pEnum = NULL;

    try
    {
        *ppEnum = NULL;
        pEnum = new ConnectionPointEnum((const ConnectionPointEnum&)*this);

        hr = pEnum->QueryInterface(IID_IEnumConnectionPoints, (LPVOID *)ppEnum);
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory exception")));
        hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        DBGERROR((TEXT("Unexpected C++ exception")));
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr) && NULL != pEnum)
    {
        delete pEnum;
        *ppEnum = NULL;
    }

    return hr;
}


