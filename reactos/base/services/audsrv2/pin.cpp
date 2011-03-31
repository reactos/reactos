//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    pin.cpp
//
// Abstract:
//      This is the implementation file for C++ classes that expose
//      functionality of KS pin objects
//

#include "kssample.h"

LPCSTR gpstrKsPinType[] = { "render", "capture" };
LPCSTR gpstrKsState[] = { "KSSTATE_STOP", "KSSTATE_ACQUIRE", "KSSTATE_PAUSE", "KSSTATE_RUN" };

typedef
KSDDKAPI
DWORD
WINAPI
KSCREATEPIN(
    IN HANDLE FilterHandle,
    IN PKSPIN_CONNECT Connect,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle
    ) ;

class CKSUSER
{
public:
    HMODULE         hmodKsuser;
    KSCREATEPIN*    DllKsCreatePin;

    CKSUSER(void)
    {
        hmodKsuser = NULL;
        DllKsCreatePin = NULL;
    }

    DWORD KsCreatePin
    (
        IN HANDLE           FilterHandle,
        IN PKSPIN_CONNECT   Connect,
        IN ACCESS_MASK      DesiredAccess,
        OUT PHANDLE         ConnectionHandle
    )
    {
        if (NULL == DllKsCreatePin)
        {
            if (NULL == hmodKsuser)
            {
                hmodKsuser = LoadLibrary(TEXT("ksuser.dll"));
                if (NULL == hmodKsuser) {
                    return ERROR_FILE_NOT_FOUND;
                }
            }

            DllKsCreatePin = (KSCREATEPIN*)GetProcAddress(hmodKsuser, "KsCreatePin");
            if (NULL == DllKsCreatePin) {
                return ERROR_FILE_NOT_FOUND;
            }
        }
        
        return DllKsCreatePin(FilterHandle,
                            Connect,
                            DesiredAccess,
                            ConnectionHandle);
    }
};

CKSUSER  KSUSER;


// ============================================================================
//
//   CKsPin
//
//=============================================================================



////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::CKsPin()
//
//  Routine Description:
//      Copy constructor for CKsPin
//
//  Arguments:
//      A parent filter,
//      A Pin to copy from,
//      An HRESULT to indicate success or failure
//
//
//  Return Value:
//
//     S_OK on success
//


CKsPin::CKsPin(
    IN  CKsFilter*    pFilter,
    IN  ULONG           PinId,
    OUT HRESULT*        phr)
    : CKsIrpTarget(INVALID_HANDLE_VALUE),
    m_pFilter(pFilter),
    m_nId(PinId),
    m_dwAlignment(0),
    m_pLinkPin(NULL),
    m_fCompleteOnlyOnRunState(TRUE),
    m_fLooped(FALSE),
    m_eType(eUnknown),
    m_ksState(KSSTATE_STOP),
    m_pksPinCreate(NULL),
    m_cbPinCreateSize(0),
    m_guidCategory(GUID_NULL)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    // non-atomic initializations
    ZeroMemory(&m_Descriptor, sizeof m_Descriptor);
    ZeroMemory(m_szFriendlyName, sizeof m_szFriendlyName);

    //Initialize
    if (SUCCEEDED(hr))
    {
        hr = CKsPin::Init();
    }

     TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::~CKsPin()
//
//  Routine Description:
//      Destructor
//
//  Arguments:
//      None
//
//
//  Return Value:
//      None
//


CKsPin::~CKsPin()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ClosePin();

    // Need to cast to BYTE *, because that's how CKsAudRenPin::CKsAudRenPin
    // and CKsAudRenPin::SetFormat allocate the memory
    delete[] (BYTE *)m_pksPinCreate;

    // Need to cast to BYTE *, because that's how CKsFilter::GetPinPropertyMulti allocates the memory
    delete[] (BYTE *)m_Descriptor.pmiDataRanges;
    delete[] (BYTE *)m_Descriptor.pmiInterfaces;
    delete[] (BYTE *)m_Descriptor.pmiMediums;

    TRACE_LEAVE();
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::ClosePin()
//
//  Routine Description:
//      Closes the pin
//
//  Arguments:
//      None
//
//
//  Return Value:
//
//     S_OK on success
//


void CKsPin::ClosePin(void)
{
    TRACE_ENTER();

    if (IsValidHandle(m_handle))
    {
        SetState(KSSTATE_STOP);

        SafeCloseHandle(m_handle);
    }

    TRACE_LEAVE();
    return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::SetState()
//
//  Routine Description:
//      SetState
//
//  Arguments:
//      SetState
//
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::SetState(
    IN  KSSTATE ksState)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    hr = SetPropertySimple(
            KSPROPSETID_Connection,
            KSPROPERTY_CONNECTION_STATE,
            &ksState,
            sizeof (ksState));

    if (SUCCEEDED(hr))
    {
        m_ksState = ksState;
    }

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_ERROR,TEXT("Failed to Set Pin State"));
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::Instantiate()
//
//  Routine Description:
//      Instantiate
//
//  Arguments:
//      fLooped
//
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::Instantiate(
    IN BOOL fLooped)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    DWORD dwResult = 0;

    if (!m_pksPinCreate)
        return FALSE;

    if (m_pLinkPin)
    {
        assert( KSPIN_COMMUNICATION_SINK == m_pLinkPin->m_Descriptor.Communication ||
            KSPIN_COMMUNICATION_BOTH == m_pLinkPin->m_Descriptor.Communication);

        m_pksPinCreate->PinToHandle = m_pLinkPin->m_handle;
    }

    m_fLooped = fLooped;
    m_pksPinCreate->Interface.Id =
        m_fLooped ? KSINTERFACE_STANDARD_LOOPED_STREAMING : KSINTERFACE_STANDARD_STREAMING;


    dwResult = KSUSER.KsCreatePin(
        m_pFilter->m_handle,
        m_pksPinCreate,
        GENERIC_WRITE | GENERIC_READ,
        &m_handle
        );

    if (ERROR_SUCCESS != dwResult)
    {
         hr = HRESULT_FROM_WIN32(dwResult);
         if (SUCCEEDED(hr))
         {
                // Sometimes the error codes don't map to error HRESULTs.
                hr = E_FAIL;
         }
    }

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_ERROR,TEXT("Failed to instantiate pin. hr=0x%08x"), hr);
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::GetState()
//
//  Routine Description:
//      Gets the KSSTATE of Pin
//
//  Arguments:
//      PKSSTATE pksState -- State is returned here
//
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::GetState(
    KSSTATE* pksState)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;


    if (NULL == pksState)
    {
        hr = E_INVALIDARG;
        DebugPrintf(TRACE_ERROR,TEXT("pksState == NULL"));
    }
    else if (!IsValidHandle(m_handle))
    {
        hr = E_FAIL;
        DebugPrintf(TRACE_ERROR,TEXT("No valid pin handle"));
    }

    if (SUCCEEDED(hr))
    {
        hr = GetPropertySimple(
            KSPROPSETID_Connection,
            KSPROPERTY_CONNECTION_STATE,
            pksState,
            sizeof(pksState));
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::Reset()
//
//  Routine Description:
//      Reset the pin
//
//  Arguments:
//      None
//
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::Reset(void)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG   ulIn=0;
    ULONG   ulBytesReturned = 0;

    ulIn = KSRESET_BEGIN;

    hr = SyncIoctl(
        m_handle,
        IOCTL_KS_RESET_STATE,
        &ulIn,
        sizeof(ULONG),
        NULL,
        0,
        &ulBytesReturned);

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_ERROR,TEXT("IOCTL_KS_RESET_STATE failed"));
    }

    ulIn = KSRESET_END;
    hr = SyncIoctl(
        m_handle,
        IOCTL_KS_RESET_STATE,
        &ulIn,
        sizeof(ULONG),
        NULL,
        0,
        &ulBytesReturned);

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_ERROR,TEXT("IOCTL_KS_RESET_STATE failed"));
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::WriteData()
//
//  Routine Description:
//      Submit some data to the pin, using the provided KSSTREAM_HEADER and OVERLAPPED structures.
//      If the caller submits a valid event in the KSSTREAM_HEADER, the event must be unsignaled.
//
//  Arguments:
//      Pointer to the KSSTREAM_HEADER structure describing the data to send to the pin
//      Pointer to the OVERLAPPED structure to use when doing the asynchronous I/O
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::WriteData(
    KSSTREAM_HEADER *pKSSTREAM_HEADER,
    OVERLAPPED *pOVERLAPPED)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    DWORD dwError = 0;
    DWORD cbReturned = 0;
    BOOL fRes = TRUE;

    if (SUCCEEDED(hr))
    {
        // submit the data
        fRes = DeviceIoControl(
            m_handle,
            IOCTL_KS_WRITE_STREAM,
            NULL,
            0,
            pKSSTREAM_HEADER,
            pKSSTREAM_HEADER->Size,
            &cbReturned,
            pOVERLAPPED);

        // we're paused, we should return false!
        if (fRes)
        {
            //DebugPrintf(TRACE_HIGH,TEXT("DeviceIoControl returned TRUE even though the pin is paused"));
        }
        else
        {
            // if it did return FALSE then GetLAstError should return ERROR_IO_PENDING
            dwError = GetLastError();

            if (ERROR_IO_PENDING == dwError)
            {
                //Life is good
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
                DebugPrintf(TRACE_ERROR,TEXT("CKsPin::WriteData's DeviceIoControl Failed!  Error=0x%#08x"),dwError);
            }
        }

    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::ReadData()
//
//  Routine Description:
//      Submit some memory for the pin to read into, using the provided KSSTREAM_HEADER and OVERLAPPED structures.
//      If the caller submits a valid event in the KSSTREAM_HEADER, the event must be unsignaled.
//
//  Arguments:
//      Pointer to the KSSTREAM_HEADER structure describing where to store the data read from the pin
//      Pointer to the OVERLAPPED structure to use when doing the asynchronous I/O
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::ReadData(
    KSSTREAM_HEADER *pKSSTREAM_HEADER,
    OVERLAPPED *pOVERLAPPED)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    DWORD dwError = 0;
    DWORD cbReturned = 0;
    BOOL fRes = TRUE;

    fRes = DeviceIoControl(
        m_handle,
        IOCTL_KS_READ_STREAM,
        NULL,
        0,
        pKSSTREAM_HEADER,
        pKSSTREAM_HEADER->Size,
        &cbReturned,
        pOVERLAPPED);

    // we're paused, we should return false!
    if (fRes)
    {
        //DebugPrintf(TRACE_HIGH,TEXT("DeviceIoControl returne TRUE even though the pin is paused"));
    }
    else
    {
        // if it did return FALSE then GetLAstError should return ERROR_IO_PENDING
        dwError = GetLastError();

        if (ERROR_IO_PENDING == dwError)
        {
            //Life is good
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
            DebugPrintf(TRACE_ERROR,TEXT("DeviceIoControl Failed!  Error=0x%08x"),dwError);
        }

    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::Init()
//
//  Routine Description:
//      Does some basic data structure initialization
//
//  Arguments:
//     None
//
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::Init()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    BOOL fFailure = FALSE;

    // Get COMMUNICATION
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_COMMUNICATION,
        &m_Descriptor.Communication,
        sizeof(KSPIN_COMMUNICATION));

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_COMMUNICATION on PinID: %d"),m_nId);
        fFailure = TRUE;
    }


   //get PKSPIN_INTERFACES
    hr = m_pFilter->GetPinPropertyMulti(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_INTERFACES,
        &m_Descriptor.pmiInterfaces);

    if (SUCCEEDED(hr))
    {
        m_Descriptor.cInterfaces = m_Descriptor.pmiInterfaces->Count;
        m_Descriptor.pInterfaces = (PKSPIN_INTERFACE)(m_Descriptor.pmiInterfaces+1);
    }
    else
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPETY_PIN_INTERFACES on PinID: %d"),m_nId);
        fFailure = TRUE;
    }

    // get PKSPIN_MEDIUMS
    hr = m_pFilter->GetPinPropertyMulti(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_MEDIUMS,
        &m_Descriptor.pmiMediums);

    if (SUCCEEDED(hr))
    {
        m_Descriptor.cMediums = m_Descriptor.pmiMediums->Count;
        m_Descriptor.pMediums = (PKSPIN_MEDIUM)(m_Descriptor.pmiMediums +1);
    }
    else
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_MEDIUMS on PinID: %d"),m_nId);
        fFailure = TRUE;
    }



    // get PKSPIN_DATARANGES
    hr = m_pFilter->GetPinPropertyMulti(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_DATARANGES,
        &m_Descriptor.pmiDataRanges);

    if (SUCCEEDED(hr))
    {
        m_Descriptor.cDataRanges = m_Descriptor.pmiDataRanges->Count;
        m_Descriptor.pDataRanges = (PKSDATARANGE)(m_Descriptor.pmiDataRanges +1);
    }
    else
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_DATARANGES on PinID: %d"),m_nId);
        fFailure = TRUE;
    }



    //get dataflow information
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_DATAFLOW,
        &m_Descriptor.DataFlow,
        sizeof(KSPIN_DATAFLOW));

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_DATAFLOW on PinID: %d"),m_nId);
        fFailure = TRUE;
    }



    //get instance information
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_CINSTANCES,
        &m_Descriptor.CInstances,
        sizeof(KSPIN_CINSTANCES));

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_CINSTANCES on PinID: %d"),m_nId);
        fFailure = TRUE;
    }



    // Get Global instance information
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_GLOBALCINSTANCES,
        &m_Descriptor.CInstancesGlobal,
        sizeof(KSPIN_CINSTANCES));

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_GLOBALCINSTANCES on PinID: %d"),m_nId);
        fFailure = TRUE;
    }

    // Get Pin Category Information
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_CATEGORY,
        &m_guidCategory,
        sizeof(m_guidCategory));
    if (FAILED(hr))
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_CATEGORY on PinID: %d"),m_nId);
        fFailure = TRUE;
    }

    // Get Pin Name
    hr = m_pFilter->GetPinPropertySimple(
        m_nId,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_NAME,
        &m_szFriendlyName,
        sizeof(m_szFriendlyName)-1);

    if (SUCCEEDED(hr))
    {
        //DebugPrintf(TRACE_LOW,TEXT("KSPROPERTY_PIN_NAME: %ls"),m_szFriendlyName);
    }
    else
    {
        DebugPrintf(TRACE_HIGH,TEXT("Failed to retrieve pin property KSPROPERTY_PIN_NAME on PinID: %d"),m_nId);
        fFailure = TRUE;
    }



    // if we experienced any failures, return S_FALSE, otherwise S_OK
    hr = fFailure ? S_FALSE : S_OK;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::GetId()
//
//  Routine Description:
//      Returns the Pin ID
//
//  Arguments:
//      None
//
//
//  Return Value:
//
//     The Pin ID
//


ULONG CKsPin::GetId()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return m_nId;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::GetType()
//
//  Routine Description:
//      Returns the Pin type
//
//  Arguments:
//      None
//
//
//  Return Value:
//
//     The Pin type
//


ETechnology CKsPin::GetType()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return m_eType;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::SetPinConnect()
//
//  Routine Description:
//      Copies the pin connect structure
//
//  Arguments:
//      The structure and its size
//
//  Return Value:
//
//     S_OK on success
//


HRESULT CKsPin::SetPinConnect(PKSPIN_CONNECT pksPinConnect, ULONG cbKsPinConnect)
{
    HRESULT hr = S_OK;
    TRACE_ENTER();

    if ((NULL == pksPinConnect) || (cbKsPinConnect < sizeof (KSPIN_CONNECT)))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        // Free the existing Pin Connect structure
        if (m_pksPinCreate)
        {
            delete [] m_pksPinCreate;
            m_pksPinCreate = NULL;
        }

        // Allocate memory for the new pin connect structure
        m_pksPinCreate = reinterpret_cast<PKSPIN_CONNECT>(new BYTE[cbKsPinConnect]);
        if (NULL == m_pksPinCreate)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Copy over the KSPIN_CONNECT structure
        CopyMemory(m_pksPinCreate, pksPinConnect, cbKsPinConnect);
    }

    TRACE_LEAVE();
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsPin::GetPinDescriptor()
//
//  Routine Description:
//      Returns the Pin ID
//
//  Arguments:
//      None
//
//
//  Return Value:
//
//     The Pin ID
//


PPIN_DESCRIPTOR CKsPin::GetPinDescriptor()
{
    TRACE_ENTER();
    TRACE_LEAVE();
    return &m_Descriptor;
}

////////////////////////////////////////////////////////////////////////////////
//

//  CKsPin::SetFormat()
//
//  Routine Description:
//      Sets the format on the pin
//  
//  Arguments: 
//      pFormat  The data format to use
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsPin::SetFormat(KSDATAFORMAT* pFormat)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    
    hr = SetPropertySimple(
            KSPROPSETID_Connection,
            KSPROPERTY_CONNECTION_DATAFORMAT,
            pFormat,
            pFormat->FormatSize,
            NULL,
            0);   
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

HRESULT CKsPin::GetDataFlow(KSPIN_DATAFLOW* pDataFlow)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    assert(NULL != pDataFlow);

    *pDataFlow = m_Descriptor.DataFlow;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


HRESULT CKsPin::GetCommunication(KSPIN_COMMUNICATION* pCommunication)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    assert(NULL != pCommunication);

    *pCommunication = m_Descriptor.Communication;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

BOOL CKsPin::HasDataRangeWithSpecifier(REFGUID guidFormatSpecifier)
{
    TRACE_ENTER();
    BOOL fFound = FALSE;
    KSDATARANGE* pdr;

    // Loop Through the DataRanges
    pdr = m_Descriptor.pDataRanges;
    for(ULONG i = 0; i < m_Descriptor.cDataRanges; i++)
    {
        // Check for a matching Specifier
        if (pdr->Specifier == guidFormatSpecifier)
        {
            // Found our matching Pin!
            fFound = TRUE;
            break;
        }

        // Advance to the next datarange.
        pdr = reinterpret_cast<KSDATARANGE*>(  reinterpret_cast<BYTE*>(pdr) + pdr->FormatSize );
    }

    TRACE_LEAVE();
    return fFound;
}
