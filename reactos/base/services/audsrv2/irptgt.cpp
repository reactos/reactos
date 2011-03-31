//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    irptgt.cpp
//
// Abstract:    
//      This is the implementation file for the C++ base class which abstracts
//      common functionality in CKsFilter and CKsPin classes
//

#include "kssample.h"

// String's for logging.  These match up with the enumerations in kssample.h
LPCSTR gpstrKsTechnology[] = {"Unknown","PCM Audio Render","PCM Audio Capture"};

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::CKsIrpTarget()
//
//  Routine Description:
//
//      CKsIrpTarget Constructor.  
//  
//  Arguments:
//  
//      None
//  
//  Return Value:
//  
//     None
//  



CKsIrpTarget::CKsIrpTarget(HANDLE handle) 
    : m_handle(handle)
{ 
    TRACE_ENTER();

    TRACE_LEAVE();
    return; 
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::CKsIrpTarget()
//
//  Routine Description:
//
//      CKsIrpTarget Constructor.  
//  
//  Arguments:
//  
//      None
//  
//  Return Value:
//  
//     None
//  



HANDLE CKsIrpTarget::GetHandle()
{ 
    TRACE_ENTER();
    
    TRACE_LEAVE();
    return m_handle; 
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::IsValidHandle()
//
//  Routine Description:
//
//      Quick and Dirty check to see if the handle is good.  Only checks against
//      a small number of known bad values.
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//      Returns TRUE unless the handle is NULL or INVALID_HANDLE_VALUE
//



BOOL CKsIrpTarget::IsValidHandle(HANDLE handle)
{
    TRACE_ENTER();
    BOOL bRet;

    bRet = !((handle == NULL) || (handle == INVALID_HANDLE_VALUE));

    TRACE_LEAVE();
    return bRet;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::SafeCloseHandle()
//
//  Routine Description:
//
//      Safely closes a file handle
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//     None
//


BOOL CKsIrpTarget::SafeCloseHandle(HANDLE &handle)
{
    TRACE_ENTER();
    BOOL bRet;
    bRet = TRUE;

    if (IsValidHandle(handle))
    {
        bRet = CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
    }

    TRACE_LEAVE();
    return bRet;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::Close()
//
//  Routine Description:
//
//      Close the Irp Target
//  
//  Arguments:
//  
//      None 
//  
//  Return Value:
//  
//     None
//


BOOL CKsIrpTarget::Close()
{
    TRACE_ENTER();
    BOOL bRet = TRUE;
    
    bRet = SafeCloseHandle(m_handle);
    
    TRACE_LEAVE();
    return bRet;
}
////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::xxx()
//
//  Routine Description:
//
//      SyncIoctl
//  
//  Arguments:
//  
//      IN  HANDLE  handle -- The file handle to send the IOCTL to
//      IN  ULONG   ulIoctl -- The IOCTL to send
//      IN  PVOID   pvInBuffer -- Pointer to the input buffer
//      IN  ULONG   cbInBuffer -- Size in bytes of the input buffer
//      OUT PVOID   pvOutBuffer -- Pointer to the output buffer
//      OUT ULONG   cbOutBuffer -- Size in bytes of the output buffer
//      OUT PULONG  pulBytesReturned -- The number of bytes written to the
//          output buffer.
//  
//  Return Value:
//  
//     S_OK on success
//



HRESULT CKsIrpTarget::SyncIoctl(
        IN  HANDLE  handle,
        IN  ULONG   ulIoctl,
        IN  PVOID   pvInBuffer,
        IN  ULONG   cbInBuffer,
        OUT PVOID   pvOutBuffer,
        OUT ULONG   cbOutBuffer,
        OUT PULONG  pulBytesReturned)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    OVERLAPPED overlapped;
        BOOL fRes = TRUE;
        ULONG ulBytesReturned;

    if (!IsValidHandle(handle))
    {
        hr = E_FAIL;
        DebugPrintf(TRACE_ERROR,TEXT("CKsIrpTarget::SyncIoctl Invalid Handle"));
    }
    
    if (!pulBytesReturned)
    {
        pulBytesReturned = &ulBytesReturned;
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(&overlapped,sizeof(overlapped));
        overlapped.hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (!overlapped.hEvent)
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf(TRACE_ERROR,TEXT("CKsIrpTarget::SyncIoctl CreateEvent failed"));
        }
        else
        {
            // Flag the event by setting the low-order bit so we
            // don't get completion port notifications.
            // Really! - see the description of the lpOverlapped parameter in
            // the docs for GetQueuedCompletionStatus
            overlapped.hEvent = (HANDLE)((DWORD_PTR)overlapped.hEvent | 0x1);
        }
    }

    if (SUCCEEDED(hr))
    {
        fRes = DeviceIoControl(handle, ulIoctl, pvInBuffer, cbInBuffer, pvOutBuffer, cbOutBuffer, pulBytesReturned, &overlapped);
        if (!fRes)
        {

            DWORD dwError;
            dwError = GetLastError();
            if (ERROR_IO_PENDING == dwError)
            {
                DWORD dwWait;
                // Wait for completion
                dwWait = ::WaitForSingleObject(overlapped.hEvent,INFINITE);
                assert(WAIT_OBJECT_0 == dwWait);
                if (dwWait != WAIT_OBJECT_0)
                {
                    hr = E_FAIL;
                    DebugPrintf(TRACE_ERROR,TEXT("CKsIrpTarget::SyncIoctl WaitForSingleObject failed dwWait:0x%08x"),dwWait);
                }
            }
            else if (((ERROR_INSUFFICIENT_BUFFER == dwError) || (ERROR_MORE_DATA == dwError)) &&
                (IOCTL_KS_PROPERTY == ulIoctl) &&
                (cbOutBuffer == 0))
            {
                hr = S_OK;
                fRes = TRUE;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        if (!fRes) *pulBytesReturned = 0;
        SafeCloseHandle(overlapped.hEvent);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::PropertySetSupport()
//
//  Routine Description:
//      Query to see if a property set is supported
//  
//  Arguments:
//      The GUID of the property set 
//  
//  Return Value:
//  
//      S_OK if the Property set is supported
//      S_FALSE if the property set is unsupported
//


HRESULT CKsIrpTarget::PropertySetSupport(
    IN  REFGUID  guidPropertySet)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ULONG ulReturned;
    KSPROPERTY ksProperty;

    ZeroMemory(&ksProperty,sizeof(ksProperty));

    ksProperty.Set = guidPropertySet;
    ksProperty.Id = 0;
    ksProperty.Flags = KSPROPERTY_TYPE_SETSUPPORT;

    hr = SyncIoctl(
        m_handle, 
        IOCTL_KS_PROPERTY, 
        &ksProperty, 
        sizeof(ksProperty), 
        NULL, 
        0, 
        &ulReturned);
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::PropertyBasicSupport()
//
//  Routine Description:
//      Get the basic support information for this KSPROPERTY
//  
//  Arguments:
//      guidPropetySet -- The guid of the property set
//      nProperty   -- The property in the property set
//      pdwSupport -- The support information for the property
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::PropertyBasicSupport(
    IN  REFGUID guidPropertySet,
    IN  ULONG   nProperty,
    OUT PDWORD  pdwSupport
)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ULONG ulReturned;
    KSPROPERTY ksProperty;

    ZeroMemory(&ksProperty,sizeof(ksProperty));

    ksProperty.Set = guidPropertySet;
    ksProperty.Id = nProperty;
    ksProperty.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

    hr = SyncIoctl(
            m_handle,
            IOCTL_KS_PROPERTY,
            &ksProperty,
            sizeof(ksProperty),
            pdwSupport,
            sizeof(DWORD),
            &ulReturned);
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::GetPropertySimple()
//
//  Routine Description:
//      Gets a simple property;
//  
//  Arguments:
//      All the stuff for a property call
//  
//  Return Value:
//     S_OK on success
//


HRESULT CKsIrpTarget::GetPropertySimple(
    IN          REFGUID guidPropertySet,
    IN          ULONG   nProperty,
    OUT         PVOID   pvValue,
    OUT         ULONG   cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG ulReturned = 0;
    PKSPROPERTY pKsProperty = NULL;
    ULONG cbProperty = 0;

    cbProperty = sizeof(KSPROPERTY) + cbInstance;
    pKsProperty = (PKSPROPERTY)new BYTE[cbProperty];
    if (NULL == pKsProperty)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(pKsProperty,sizeof(*pKsProperty));
        pKsProperty->Set = guidPropertySet;
        pKsProperty->Id = nProperty;
        pKsProperty->Flags = KSPROPERTY_TYPE_GET;

        if (pvInstance)
        {
            CopyMemory(reinterpret_cast<BYTE*>(pKsProperty) + sizeof(KSPROPERTY),pvInstance,cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pKsProperty,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }    

    //cleanup memory
    delete[] (BYTE*)pKsProperty;
    pKsProperty = NULL;
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::GetPropertyMulti()
//
//  Routine Description:
//      MultipleItem request.  The function allocates memory for the 
//      caller.  It is the caller's responsiblity to free this memory
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::GetPropertyMulti(
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    OUT PKSMULTIPLE_ITEM*   ppKsMultipleItem)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ULONG cbMultipleItem = 0;
    ULONG ulReturned = 0;
    KSPROPERTY ksProperty;

    ZeroMemory(&ksProperty, sizeof(ksProperty));
    ksProperty.Set = guidPropertySet;
    ksProperty.Id = nProperty;
    ksProperty.Flags = KSPROPERTY_TYPE_GET;

    hr = SyncIoctl(
            m_handle,
            IOCTL_KS_PROPERTY,
            &ksProperty,
            sizeof(KSPROPERTY),
            NULL,
            0,
            &cbMultipleItem);
    if (SUCCEEDED(hr) && cbMultipleItem)
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM) new BYTE[cbMultipleItem];
        if (NULL == *ppKsMultipleItem)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && cbMultipleItem)
    {
        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                &ksProperty,
                sizeof(ksProperty),
                reinterpret_cast<VOID*>(*ppKsMultipleItem),
                cbMultipleItem,
                &ulReturned);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::GetPropertyMulti()
//
//  Routine Description:
//      MultipleItem request for when the input is not a property
//      The function allocates memory for the 
//      caller.  It is the caller's responsiblity to free this memory
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::GetPropertyMulti(
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    IN  PVOID               pvData,
    IN  ULONG               cbData,
    OUT PKSMULTIPLE_ITEM*   ppKsMultipleItem)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ULONG cbMultipleItem = 0;
    ULONG ulReturned = 0;
    KSPROPERTY ksProperty;

    ZeroMemory(&ksProperty, sizeof(ksProperty));
    ksProperty.Set = guidPropertySet;
    ksProperty.Id = nProperty;
    ksProperty.Flags = KSPROPERTY_TYPE_GET;

    hr = SyncIoctl(
            m_handle,
            IOCTL_KS_PROPERTY,
            &ksProperty,
            sizeof(KSPROPERTY),
            NULL,
            0,
            &cbMultipleItem);
    if (SUCCEEDED(hr) && cbMultipleItem)
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM) new BYTE[cbMultipleItem];
        if (NULL == *ppKsMultipleItem)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr) && cbMultipleItem)
    {
        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                (PKSPROPERTY) pvData,
                cbData,
                reinterpret_cast<VOID*>(*ppKsMultipleItem),
                cbMultipleItem,
                &ulReturned);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::GetNodePropertySimple()
//
//  Routine Description:
//      Gets a simple node property
//  
//  Arguments:
//      All the stuff for a property call
//  
//  Return Value:
//     S_OK on success
//


HRESULT CKsIrpTarget::GetNodePropertySimple(
    IN  ULONG               nNodeID,
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    OUT PVOID               pvValue,
    IN  ULONG               cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG ulReturned = 0;
    PKSNODEPROPERTY pKsNodeProperty = NULL;
    ULONG cbProperty = 0;

    cbProperty = sizeof(KSNODEPROPERTY) + cbInstance;
    pKsNodeProperty = (PKSNODEPROPERTY)new BYTE[cbProperty];
    if (NULL == pKsNodeProperty)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(pKsNodeProperty,sizeof(*pKsNodeProperty));
        pKsNodeProperty->Property.Set = guidPropertySet;
        pKsNodeProperty->Property.Id = nProperty;
        pKsNodeProperty->Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
        pKsNodeProperty->NodeId = nNodeID;
        pKsNodeProperty->Reserved = 0;

        if (pvInstance)
        {
            CopyMemory(reinterpret_cast<BYTE*>(pKsNodeProperty) + sizeof(KSNODEPROPERTY),pvInstance,cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pKsNodeProperty,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }    

    //cleanup memory
    delete[] (BYTE*)pKsNodeProperty;
    pKsNodeProperty = NULL;
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::GetNodePropertyChannel()
//
//  Routine Description:
//      Gets a simple node channel property
//  
//  Arguments:
//      All the stuff for a property call
//  
//  Return Value:
//     S_OK on success
//


HRESULT CKsIrpTarget::GetNodePropertyChannel(
    IN  ULONG               nNodeID,
    IN  ULONG               nChannel,
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    OUT PVOID               pvValue,
    IN  ULONG               cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG ulReturned = 0;
    PKSNODEPROPERTY_AUDIO_CHANNEL pKsNodePropertyChannel = NULL;
    ULONG cbProperty = 0;

    cbProperty = sizeof(KSNODEPROPERTY_AUDIO_CHANNEL) + cbInstance;
    pKsNodePropertyChannel = (PKSNODEPROPERTY_AUDIO_CHANNEL)new BYTE[cbProperty];
    if (NULL == pKsNodePropertyChannel)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ZeroMemory(pKsNodePropertyChannel,sizeof(*pKsNodePropertyChannel));
        pKsNodePropertyChannel->NodeProperty.Property.Set = guidPropertySet;
        pKsNodePropertyChannel->NodeProperty.Property.Id = nProperty;
        pKsNodePropertyChannel->NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;
        pKsNodePropertyChannel->NodeProperty.NodeId = nNodeID;
        pKsNodePropertyChannel->NodeProperty.Reserved = 0;
        pKsNodePropertyChannel->Channel = nChannel;
        pKsNodePropertyChannel->Reserved = 0;

        if (pvInstance)
        {
            CopyMemory(reinterpret_cast<BYTE*>(pKsNodePropertyChannel) + sizeof(KSNODEPROPERTY_AUDIO_CHANNEL),pvInstance,cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pKsNodePropertyChannel,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }    

    //cleanup memory
    delete[] (BYTE*)pKsNodePropertyChannel;
    pKsNodePropertyChannel = NULL;
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::SetPropertySimple()
//
//  Routine Description:
//
//      Set the value of a simple (non-multi) property
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::SetPropertySimple(
    IN          REFGUID guidPropertySet,
    IN          ULONG   nProperty,
    OUT         PVOID   pvValue,
    OUT         ULONG   cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG       ulReturned  = 0;
    PKSPROPERTY pksProperty = NULL;
    ULONG       cbProperty  = 0;

    cbProperty = sizeof(KSPROPERTY) + cbInstance;
    pksProperty = (PKSPROPERTY)new BYTE[cbProperty];
    if (NULL == pksProperty)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pksProperty->Set    = guidPropertySet; 
        pksProperty->Id     = nProperty;       
        pksProperty->Flags  = KSPROPERTY_TYPE_SET;

        if (pvInstance)
        {
            memcpy((PBYTE)pksProperty + sizeof(KSPROPERTY), pvInstance, cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pksProperty,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }

    //cleanup memory
    delete[] (BYTE*)pksProperty;
    pksProperty = NULL;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::SetPropertyMulti()
//
//  Routine Description:
//
//      Blah Blah Blah
//  
//  Arguments:
//  
//      HANDLE 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::SetPropertyMulti(
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    OUT PKSMULTIPLE_ITEM*   ppKsMultipleItem)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG       cbMultipleItem = 0;
    ULONG       ulReturned = 0;
    KSPROPERTY  ksProperty;

    ksProperty.Set    = guidPropertySet; 
    ksProperty.Id     = nProperty;       
    ksProperty.Flags  = KSPROPERTY_TYPE_SET;

    hr = SyncIoctl(
            m_handle,
            IOCTL_KS_PROPERTY,
            &ksProperty,
            sizeof(KSPROPERTY),
            NULL,
            0,
            &cbMultipleItem);

    if (SUCCEEDED(hr))
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM)LocalAlloc(LPTR, cbMultipleItem);
        
        hr = SyncIoctl (
                m_handle,
                IOCTL_KS_PROPERTY,
                &ksProperty,
                sizeof(KSPROPERTY),
                (PVOID)*ppKsMultipleItem,
                cbMultipleItem,
                &ulReturned);
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::SetNodePropertySimple()
//
//  Routine Description:
//
//      Set the value of a simple (non-multi) node property
//  
//  Arguments:
//  
//      All the stuff for a property call 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::SetNodePropertySimple(
    IN          ULONG   nNodeID,
    IN          REFGUID guidPropertySet,
    IN          ULONG   nProperty,
    OUT         PVOID   pvValue,
    OUT         ULONG   cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG       ulReturned  = 0;
    PKSNODEPROPERTY pKsNodeProperty = NULL;
    ULONG       cbProperty  = 0;

    cbProperty = sizeof(KSNODEPROPERTY) + cbInstance;
    pKsNodeProperty = (PKSNODEPROPERTY)new BYTE[cbProperty];
    
    if (NULL == pKsNodeProperty)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pKsNodeProperty->Property.Set    = guidPropertySet; 
        pKsNodeProperty->Property.Id     = nProperty;       
        pKsNodeProperty->Property.Flags  = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
        pKsNodeProperty->NodeId          = nNodeID;
        pKsNodeProperty->Reserved        = 0;

        if (pvInstance)
        {
            memcpy((PBYTE)pKsNodeProperty + sizeof(KSNODEPROPERTY), pvInstance, cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pKsNodeProperty,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }

    //cleanup memory
    delete[] (BYTE*)pKsNodeProperty;
    pKsNodeProperty = NULL;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsIrpTarget::SetNodePropertyChannel()
//
//  Routine Description:
//
//      Set the value of a simple (non-multi) node channel property
//  
//  Arguments:
//  
//      All the stuff for a property call 
//  
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsIrpTarget::SetNodePropertyChannel(
    IN          ULONG   nNodeID,
    IN          ULONG   nChannel,
    IN          REFGUID guidPropertySet,
    IN          ULONG   nProperty,
    OUT         PVOID   pvValue,
    OUT         ULONG   cbValue,
    OPTIONAL IN PVOID   pvInstance,
    OPTIONAL IN ULONG   cbInstance)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG       ulReturned  = 0;
    PKSNODEPROPERTY_AUDIO_CHANNEL pKsNodePropertyChannel = NULL;
    ULONG       cbProperty  = 0;

    cbProperty = sizeof(KSNODEPROPERTY_AUDIO_CHANNEL) + cbInstance;
    pKsNodePropertyChannel = (PKSNODEPROPERTY_AUDIO_CHANNEL)new BYTE[cbProperty];
    
    if (NULL == pKsNodePropertyChannel)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        pKsNodePropertyChannel->NodeProperty.Property.Set    = guidPropertySet; 
        pKsNodePropertyChannel->NodeProperty.Property.Id     = nProperty;       
        pKsNodePropertyChannel->NodeProperty.Property.Flags  = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
        pKsNodePropertyChannel->NodeProperty.NodeId          = nNodeID;
        pKsNodePropertyChannel->NodeProperty.Reserved        = 0;
        pKsNodePropertyChannel->Channel         = nChannel;
        pKsNodePropertyChannel->Reserved        = 0;

        if (pvInstance)
        {
            memcpy((PBYTE)pKsNodePropertyChannel + sizeof(KSNODEPROPERTY_AUDIO_CHANNEL), pvInstance, cbInstance);
        }

        hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
                pKsNodePropertyChannel,
                cbProperty,
                pvValue,
                cbValue,
                &ulReturned);
    }

    //cleanup memory
    delete[] (BYTE*)pKsNodePropertyChannel;
    pKsNodePropertyChannel = NULL;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}
