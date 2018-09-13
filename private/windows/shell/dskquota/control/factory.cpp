///////////////////////////////////////////////////////////////////////////////
/*  File: factory.cpp

    Description: Contains the member function definitions for class
        DiskQuotaControlClassFactory.  The class factory object generates
        new instances of DiskQuotaControl objects.  The object implements
        IClassFactory.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h" // PCH
#pragma hdrstop

#include "dskquota.h"
#include "control.h"
#include "factory.h"

//
// Verify that build is UNICODE.
//
#if !defined(UNICODE)
#   error This module must be compiled UNICODE.
#endif


extern LONG g_cLockThisDll;  // Supports LockServer().


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControlClassFactory::QueryInterface

    Description: Retrieves a pointer to the IUnknown or IClassFactory 
        interface.  Recoginizes the IID_IUnknown and IID_IClassFactory
        interface IDs.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NOERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added IShellPropSheetExt                             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControlClassFactory::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlClassFactory::QueryInterface")));
    DBGPRINTIID(DM_CONTROL, DL_MID, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IUnknown == riid || IID_IClassFactory == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControlClassFactory::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaControlClassFactory::AddRef(
   VOID
   )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControlClassFactory::AddRef")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
                     this, m_cRef, m_cRef + 1));

    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControlClassFactory::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
DiskQuotaControlClassFactory::Release(
    VOID
    )
{
    DBGTRACE((DM_CONTROL, DL_LOW, TEXT("DiskQuotaControlClassFactory::Release")));
    DBGPRINT((DM_CONTROL, DL_LOW, TEXT("\t0x%08X  %d -> %d"),
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
/*  Function: DiskQuotaControlClassFactory::CreateInstance

    Description: Creates a new instance of a DiskQuotaControl object, returning
        a pointer to its IDiskQuotaControl interface.

    Arguments:
        pUnkOuter - Pointer to outer object's IUnknown interface for IUnknown
            delegation in support of aggregation.  Aggregation is not supported
            by IDiskQuotaControl.

        riid - Reference to interface ID being requested.

        ppvOut - Address of interface pointer variable to accept interface
            pointer.

    Returns:
        NOERROR               - Success.
        E_OUTOFMEMORY         - Insufficient memory to create new object.
        E_NOINTERFACE         - Requested interface not supported.
        E_INVALIDARG          - ppvOut arg was NULL.
        CLASS_E_NOAGGREGATION - Aggregation was requested but is not supported.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    08/15/96    Added shell extension support.                       BrianAu
    08/20/97    Added IDispatch support.                             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControlClassFactory::CreateInstance(
    LPUNKNOWN pUnkOuter, 
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlClassFactory::CreateInstance")));
    DBGPRINTIID(DM_CONTROL, DL_HIGH, riid);

    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    try
    {
        if (NULL != pUnkOuter && IID_IUnknown != riid)
        {
            hr = CLASS_E_NOAGGREGATION;
        }
        else if (IID_IClassFactory == riid)
        {
            *ppvOut = this;
            ((LPUNKNOWN)*ppvOut)->AddRef();
            hr = NOERROR;
        }
        else if (IID_IDiskQuotaControl == riid ||
                 IID_IDispatch == riid ||
                 IID_IUnknown == riid)
        {
            hr = Create_IDiskQuotaControl(riid, ppvOut);
        }
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

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControlClassFactory::LockServer

    Description: Places/removes a lock on the DLL server.  See OLE 2
        documentation of IClassFactory for details.
        
    Arguments:
        fLock - TRUE = Increment lock count, FALSE = Decrement lock count.

    Returns:
        S_OK    - Success.
        S_FALSE - Lock count is already 0.  Can't be decremented.
        

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
DiskQuotaControlClassFactory::LockServer(
    BOOL fLock
    )
{
    DBGTRACE((DM_CONTROL, DL_HIGH, TEXT("DiskQuotaControlClassFactory::LockServer")));
    HRESULT hr = S_OK;

    if (fLock)
    {
        //
        // Increment the lock count.
        //
        InterlockedIncrement(&g_cLockThisDll);
    }
    else
    {
        //
        // Decrement only if lock count is > 0.
        // Otherwise, it's an error.
        //
        LONG lLock = g_cLockThisDll - 1;
        if (0 <= lLock)
        {
            InterlockedDecrement(&g_cLockThisDll);
            CoFreeUnusedLibraries();
        }
        else
            hr = S_FALSE;  // Lock count already at 0.
    }

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: DiskQuotaControlClassFactory::Create_IDiskQuotaControl

    Description: Creates a DiskQuotaControl object and returns a pointer
        to it's IDiskQuotaControl interface.

    Arguments:
        ppvOut - Address of interface pointer variable to receive the interface
            pointer.

        riid - Reference to interface ID requested.

    Returns:
        NOERROR        - Success.

    Exceptions: OutOfMemory.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    08/15/96    Initial creation.                                    BrianAu
                Broke code out of CreateInstance().
    08/20/97    Added riid argument.                                 BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
HRESULT
DiskQuotaControlClassFactory::Create_IDiskQuotaControl(
    REFIID riid,
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_CONTROL, DL_MID, TEXT("DiskQuotaControlClassFactory::Create_IDiskQuotaControl")));
    DBGASSERT((NULL != ppvOut));

    HRESULT hr = NOERROR;
    DiskQuotaControl *pController = new DiskQuotaControl;

    hr = pController->QueryInterface(riid, ppvOut);

    if (FAILED(hr))
    {
        delete pController;
    }

    return hr;
}

