//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    filter.cpp
//
// Abstract:    
//      This is the implementation file for C++ classes that expose
//      functionality of KS filter objects
//

#include "kssample.h"

// ============================================================================
//
//   CKsFilter
//
//=============================================================================


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::InternalInit()
//
//  Routine Description:
//      Internal Initialization Function.  It zero's out the strings and calls
//      initialzie on all of the lists.
//  
//  Arguments: 
//      None  
//
//  Return Value:
//  
//     S_OK on success.  Propigates all failures.
//


HRESULT CKsFilter::InternalInit()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    // Zero out the strings
    ZeroMemory(m_szFilterName, sizeof m_szFilterName);

    // Initialize the Lists
    hr = m_listCaptureSinkPins.Initialize(1);
    if SUCCEEDED(hr)
    {
        hr = m_listCaptureSourcePins.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listCaptureSourcePins"));
    }
        

    if (SUCCEEDED(hr))
    {
        hr = m_listNoCommPins.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listNoCommPins"));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_listPins.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listPins"));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_listRenderSinkPins.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listRenderSinkPins"));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_listRenderSourcePins.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listRenderSourcePins"));
    }

    if (SUCCEEDED(hr))
    {
        hr = m_listNodes.Initialize(1);
        if (FAILED(hr))
            DebugPrintf(TRACE_ERROR,TEXT("Failed to Initialize m_listNodes"));
    }

    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::CKsFilter()
//
//  Routine Description:
//      Constructor
//  
//  Arguments: 
//      Strings and more Strings!
//
//
//  Return Value:
//  
//     S_OK on success
//


CKsFilter::CKsFilter (
    LPCTSTR     pszName,
    HRESULT*   phr)
    : CKsIrpTarget(INVALID_HANDLE_VALUE),
    m_eType(eUnknown)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    hr = InternalInit();

    if (NULL == pszName)
    {
        hr = E_INVALIDARG;
        DebugPrintf(TRACE_ERROR,TEXT("pszName cannot be NULL"));
    }

    if (SUCCEEDED(hr))
    {
        _tcsncpy(m_szFilterName, pszName, MAX_PATH);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::DestroyLists()
//
//  Routine Description:
//      Dumps the contents of the lists
//  
//  Arguments: 
//      None
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::DestroyLists()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    CKsPin* pPin;
    CKsNode *pKsNode;

    // Clear the node list
    while( m_listNodes.RemoveHead(&pKsNode) )
    {
        delete pKsNode;
    }

    // clean pins
    while(m_listPins.RemoveHead(&pPin))
    {
        delete pPin;
    }

    // empty shallow copy lists
    while (m_listRenderSinkPins.RemoveHead(&pPin));
    while(m_listRenderSourcePins.RemoveHead(&pPin));
    while(m_listCaptureSinkPins.RemoveHead(&pPin));
    while(m_listCaptureSourcePins.RemoveHead(&pPin));
    while(m_listNoCommPins.RemoveHead(&pPin));
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::~CKsFilter()
//
//  Routine Description:
//      CKsFilter Destructor
//  
//  Arguments: 
//      None
//
//  Return Value:
//      None
//


CKsFilter::~CKsFilter()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    
    hr = DestroyLists();
    SafeCloseHandle(m_handle);
    
    assert(SUCCEEDED(hr));
    
    TRACE_LEAVE_HRESULT(hr);
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::Instantiate()
//
//  Routine Description:
//      Instantiates the pin 
//  
//  Arguments: 
//      None, Make sure that the object is fully initialized before calling this function  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::Instantiate(void)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    m_handle = CreateFile(
        m_szFilterName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (!IsValidHandle(m_handle))
    {
        hr = E_FAIL;
    }
    
    if (FAILED(hr))
    {
        DWORD dwError = GetLastError();
        DebugPrintf(TRACE_ERROR,TEXT("CKsFilter::Instantiate: CreateFile failed for device %s.  ErrorCode = 0x%08x"),m_szFilterName, dwError);
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::EnumeratePins()
//
//  Routine Description:
//      Enumerates Pins
//  
//  Arguments: 
//      None  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::EnumeratePins(void)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    HRESULT   hrWarn        = S_OK;
    DWORD   fViableFilter   = FALSE;
    HANDLE  hPin            = NULL;
    ULONG   cPins, nPinId;
    DWORD   dwKsRet         = ERROR_NOT_SUPPORTED;

    hr = GetPinPropertySimple(
        0,
        KSPROPSETID_Pin,
        KSPROPERTY_PIN_CTYPES,
        &cPins,
        sizeof cPins );
        
    if (SUCCEEDED(hr))
    {
        // Loop through the pins
        for (nPinId = 0; nPinId < cPins; nPinId++)
        {
            // create a new CKsPin
            hr = S_OK;
            CKsPin* pNewPin = new CKsPin(this,nPinId, &hr);

            if (FAILED(hr))
            {
                DebugPrintf(TRACE_ERROR, TEXT("Failed to construct pin"));
                goto break_loop;
            }
            else if (NULL == pNewPin)
            {
                hr = E_OUTOFMEMORY;
                DebugPrintf(TRACE_ERROR, TEXT("Failed to create pin"));
                goto break_loop;
            }

            if (NULL==m_listPins.AddTail(pNewPin))
            {
                DebugPrintf(TRACE_ERROR, TEXT("Failed to add pin to list"));
                goto break_loop;
            }
            continue;

       break_loop:

            delete pNewPin;
            pNewPin = NULL;
        }
    }

    hr = ClassifyPins();

    fViableFilter = (
        m_listCaptureSinkPins.GetCount()    ||
        m_listCaptureSourcePins.GetCount()  ||
        m_listRenderSinkPins.GetCount()     ||
        m_listRenderSourcePins.GetCount()   ||
        m_listNoCommPins.GetCount() );

    if (!fViableFilter)
    {
        hr = E_FAIL;
        DebugPrintf(TRACE_ERROR, TEXT("Filter is not viable. It has no pins."));
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::ClassifyPins()
//
//  Routine Description:
//      Classify Pins
//  
//  Arguments: 
//      None -- It operates on the internal m_listPins 
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::ClassifyPins()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    LISTPOS pos;
    CKsPin* pPin = NULL;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;
    

    pos = m_listPins.GetHeadPosition();

    while(SUCCEEDED(hr) && m_listPins.GetNext(pos, &pPin))
    {
        hr = pPin->GetCommunication(&Communication);
        if (FAILED(hr))
        {
            break;
        }
        
        hr = pPin->GetDataFlow(&DataFlow);
        if (FAILED(hr))
        {
            break;
        }

        
        if (KSPIN_DATAFLOW_IN == DataFlow)
        {
            switch(Communication)
            {
                case KSPIN_COMMUNICATION_SINK:
                    if (NULL == m_listRenderSinkPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_SOURCE:
                    if (NULL == m_listCaptureSourcePins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_BOTH:
                    if (NULL == m_listRenderSinkPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if (NULL == m_listCaptureSourcePins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_NONE:
                    if (NULL == m_listNoCommPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                default:
                    DebugPrintf(TRACE_LOW,TEXT("Pin communication type not recognized"));
                    break;
            }
        }
        else
        {
            switch(Communication)
            {
                case KSPIN_COMMUNICATION_SINK:
                    if (NULL == m_listCaptureSinkPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_SOURCE:
                    if (NULL == m_listRenderSourcePins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_BOTH:
                    if (NULL == m_listCaptureSinkPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if (NULL == m_listRenderSourcePins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                case KSPIN_COMMUNICATION_NONE:
                    if (NULL == m_listNoCommPins.AddTail(pPin))
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    break;

                default:
                    DebugPrintf(TRACE_LOW,TEXT("Pin communication type not recognized"));
                    break;
            }
        }
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::GetPinPropertySimple()
//
//  Routine Description:
//      Gets simple pin properties
//  
//  Arguments: 
//      Pin property stuff
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::GetPinPropertySimple(
    IN  ULONG   nPinId,
    IN  REFGUID guidPropertySet,
    IN  ULONG   nProperty,
    OUT PVOID   pvValue,
    OUT ULONG   cbValue)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    ULONG ulReturned = 0;
    KSP_PIN ksPProp;

    ksPProp.Property.Set = guidPropertySet;
    ksPProp.Property.Id = nProperty;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = nPinId;
    ksPProp.Reserved = 0;

    hr = SyncIoctl(
        m_handle,
        IOCTL_KS_PROPERTY,
        &ksPProp,
        sizeof(KSP_PIN),
        pvValue,
        cbValue,
        &ulReturned);
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::GetPinPropertyMulti()
//
//  Routine Description:
//      Gets multiple pin properties
//  
//  Arguments: 
//      Pin property stuff  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::GetPinPropertyMulti(
    IN  ULONG               nPinId,
    IN  REFGUID             guidPropertySet,
    IN  ULONG               nProperty,
    OUT PKSMULTIPLE_ITEM*   ppKsMultipleItem)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    ULONG   ulReturned = 0;
    ULONG   cbMultipleItem = 0;
    KSP_PIN ksPProp;

    ksPProp.Property.Set = guidPropertySet;
    ksPProp.Property.Id = nProperty;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = nPinId;
    ksPProp.Reserved = 0;

    hr = SyncIoctl(
        m_handle,
        IOCTL_KS_PROPERTY,
        &ksPProp.Property,
        sizeof(KSP_PIN),
        NULL,
        0,
        &cbMultipleItem);

    if (SUCCEEDED(hr))
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM) new BYTE[cbMultipleItem];
        if (NULL == ppKsMultipleItem)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = SyncIoctl(
                m_handle,
                IOCTL_KS_PROPERTY,
               &ksPProp,
               sizeof(KSP_PIN),
               (PVOID)*ppKsMultipleItem,
               cbMultipleItem,
               &ulReturned);
        }
    }
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}



////////////////////////////////////////////////////////////////////////////////
//
//  CKsFilter::EnumerateNodes()
//
//  Routine Description:
//      Enumerates nodes
//  
//  Arguments: 
//      None  
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsFilter::EnumerateNodes(void)
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    // Clear the existing node list
    CKsNode *pKsNode;
    while( m_listNodes.RemoveHead(&pKsNode) )
    {
        delete pKsNode;
    }

    PKSMULTIPLE_ITEM    pkmiTopoNodes = NULL;

    hr = GetPropertyMulti(
            KSPROPSETID_Topology,
            KSPROPERTY_TOPOLOGY_NODES,
            &pkmiTopoNodes 
        );

    if (FAILED(hr))
    {
        DebugPrintf(TRACE_NORMAL,TEXT( "Failed to get property KSPROPSETID_Topology.KSPROPERTY_TOPOLOGY_NODES"));
    }
    else
    {
        if (pkmiTopoNodes)
        {
            ULONG cNodes = pkmiTopoNodes->Count;

            // Point to immediately following pkmiTopoNodes
            GUID* argguidNodes = (GUID*)(pkmiTopoNodes + 1);

            // Iterate through all the nodes
            for(ULONG nNode = 0; nNode < cNodes; nNode++)
            {
                CKsNode* pnewNode = new CKsNode(nNode, argguidNodes[nNode], &hr);
                if (pnewNode)
                {
                    if (SUCCEEDED(hr))
                    {
                        // Add the node to the list
                        if (NULL==m_listNodes.AddTail(pnewNode))
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if (FAILED(hr))
                    {
                        delete pnewNode;
                    }
                }
                // Don't need to set hr if we got a NULL result, as the check below will fail
                // and set hr to E_FAIL.
            }

            if (m_listNodes.GetCount() != cNodes)
            {
                hr = E_FAIL;

                while( m_listNodes.RemoveHead(&pKsNode) )
                {
                    delete pKsNode;
                }
            }
        }
    }

    delete[] (BYTE*)pkmiTopoNodes;

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}
