///////////////////////////////////////////////////////////////////////////////
/*  File: format.cpp

    Description: Implementation for class EnumFORMATETC.
        Moved from original location in dataobj.cpp (deleted from project).


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "format.h"


EnumFORMATETC::EnumFORMATETC(
    UINT cFormats, 
    LPFORMATETC prgFormats
    ) : m_cRef(0),
        m_cFormats(0),
        m_iCurrent(0),
        m_prgFormats(NULL)
{
    DBGTRACE((DM_DRAGDROP, DL_HIGH, TEXT("EnumFORMATETC::EnumFORMATETC")));
    DBGPRINT((DM_DRAGDROP, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    m_prgFormats = new FORMATETC[m_cFormats = cFormats];

    for (UINT i = 0; i < m_cFormats; i++)
    {
        m_prgFormats[i] = prgFormats[i];
    }
}


EnumFORMATETC::EnumFORMATETC(
    const EnumFORMATETC& ef
    ) : m_cRef(0),
        m_cFormats(ef.m_cFormats),
        m_iCurrent(0),
        m_prgFormats(NULL)
{
    DBGTRACE((DM_DRAGDROP, DL_HIGH, TEXT("EnumFORMATETC::EnumFORMATETC (Copy)")));
    DBGPRINT((DM_DRAGDROP, DL_HIGH, TEXT("\tthis = 0x%08X"), this));

    m_prgFormats = new FORMATETC[m_cFormats];

    for (UINT i = 0; i < m_cFormats; i++)
    {
        m_prgFormats[i] = ef.m_prgFormats[i];
    }
}



EnumFORMATETC::~EnumFORMATETC(
    VOID
    )
{
    DBGTRACE((DM_DRAGDROP, DL_HIGH, TEXT("EnumFORMATETC::~EnumFORMATETC")));
    DBGPRINT((DM_DRAGDROP, DL_HIGH, TEXT("\tthis = 0x%08X"), this));
    if (NULL != m_prgFormats)
        delete[] m_prgFormats;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: EnumFORMATETC::QueryInterface

    Description: Returns an interface pointer to the object's IUnknown or 
        IEnumFORMATETC interface.  Only IID_IUnknown and 
        IID_IEnumFORMATETC are recognized.  The object referenced by the 
        returned interface pointer is uninitialized.  The recipient of the 
        pointer must call Initialize() before the object is usable.

    Arguments:
        riid - Reference to requested interface ID.

        ppvOut - Address of interface pointer variable to accept interface ptr.

    Returns:
        NO_ERROR        - Success.
        E_NOINTERFACE   - Requested interface not supported.
        E_INVALIDARG    - ppvOut argument was NULL.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/25/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
EnumFORMATETC::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    HRESULT hResult = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IEnumFORMATETC == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hResult = NOERROR;
    }

    return hResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: EnumFORMATETC::AddRef

    Description: Increments object reference count.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/25/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
EnumFORMATETC::AddRef(
    VOID
    )
{
    ULONG ulReturn = m_cRef + 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("EnumFORMATETC::AddRef, 0x%08X  %d -> %d\n"),
             this, ulReturn - 1, ulReturn));

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: EnumFORMATETC::Release

    Description: Decrements object reference count.  If count drops to 0,
        object is deleted.

    Arguments: None.

    Returns: New reference count value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    09/25/96    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) 
EnumFORMATETC::Release(
    VOID
    )
{
    ULONG ulReturn = m_cRef - 1;

    DBGPRINT((DM_COM, DL_HIGH, TEXT("EnumFORMATETC::Release, 0x%08X  %d -> %d\n"),
             this, ulReturn + 1, ulReturn));

    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


STDMETHODIMP
EnumFORMATETC::Next(
    DWORD cFormats,
    LPFORMATETC pFormats,
    LPDWORD pcReturned
    )
{
    HRESULT hResult = S_OK;

    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("EnumFORMATETC::Next %d"), cFormats));
    DWORD iFormats = 0;
    if (NULL == pFormats)
        return E_INVALIDARG;

    while(cFormats-- > 0)
    {
        if (m_iCurrent < m_cFormats)
        {
            *(pFormats + iFormats++) = m_prgFormats[m_iCurrent++];
        }
        else
        {
            hResult = S_FALSE;
            break;
        }
    }

    if (NULL != pcReturned)
        *pcReturned = iFormats;

    return hResult;
}


STDMETHODIMP
EnumFORMATETC::Skip(
    DWORD cFormats
    )
{
    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("EnumFORMATETC::Skip %d"), cFormats));
    while((cFormats-- > 0) && (m_iCurrent < m_cFormats))
        m_iCurrent++;

    return cFormats == 0 ? S_OK : S_FALSE;
}


STDMETHODIMP 
EnumFORMATETC::Reset(
    VOID
    )
{
    DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("EnumFORMATETC::Reset")));
    m_iCurrent = 0;
    return S_OK;
}


STDMETHODIMP 
EnumFORMATETC::Clone(
    IEnumFORMATETC **ppvOut
    )
{
    HRESULT hResult = NO_ERROR;
    try
    {
        EnumFORMATETC *pNew = new EnumFORMATETC(*this);

        DBGPRINT((DM_DRAGDROP, DL_MID, TEXT("EnumFORMATETC::Clone")));
        hResult = pNew->QueryInterface(IID_IEnumFORMATETC, (LPVOID *)ppvOut);
    }
    catch(CAllocException& e)
    {
        hResult = E_OUTOFMEMORY;
    }
    catch(...)
    {
        hResult = E_UNEXPECTED;
    }

    return hResult;
}

