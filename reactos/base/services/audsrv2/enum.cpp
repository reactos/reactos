//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    enum.cpp
//
// Abstract:    
//      This is the implementation file for the C++ class which encapsulates
//      the setupDi functions needed to enumerate KS Filters
//


#include "kssample.h"
#include <setupapi.h>
#include <mmsystem.h>

// ============================================================================
//
//   CKsEnumFilters
//
//=============================================================================

// Utility Function

inline BOOL
IsEqualGUIDAligned(GUID guid1, GUID guid2)
{
    return ((*(PLONGLONG)(&guid1) == *(PLONGLONG)(&guid2)) && (*((PLONGLONG)(&guid1) + 1) == *((PLONGLONG)(&guid2) + 1)));
}


///////////////////////////////////////////////////////////////////////////////
//
//  CKsEnumFilters::CKsEnumFilters()
//
//  Routine Description:
//      Constructor
//  
//  Arguments: 
//      HRESULT* phr -- The results of the construction.  Because the lists
//      allocate memory we have to be able to fail construction.  The results
//      are propigated back to the caller through this argument.  If 
//      construction fails the state of the object is undefined
//
//
//  Return Value:
//      None, but there is a side-effect parameter
//


CKsEnumFilters::CKsEnumFilters(HRESULT* phr)
{
    TRACE_ENTER();
    HRESULT hr;
    
    // Initialize the list
    hr = m_listFilters.Initialize(1);
    
    TRACE_LEAVE_HRESULT(hr);
    *phr = hr;
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsEnumFilters::~CKsEnumFilters()
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


CKsEnumFilters::~CKsEnumFilters()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;
    
    hr = DestroyLists();
    
    assert(SUCCEEDED(hr));
    
    TRACE_LEAVE_HRESULT(hr);
    return;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsEnumFilters::DestroyLists()
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


HRESULT CKsEnumFilters::DestroyLists()
{
    TRACE_ENTER();
    HRESULT hr = S_OK;

    CKsFilter* pFilter;

    // clean list of filters
    while(m_listFilters.RemoveHead(&pFilter))
    {
        delete pFilter;
    }
    
    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
//  CKsEnumFilters::EnumFilters()
//
//  Routine Description:
//      Enumerate filters that match the given parameters
//  
//  Arguments: 
//      Filter attributes
//
//
//  Return Value:
//  
//     S_OK on success
//


HRESULT CKsEnumFilters::EnumFilters(
    IN  ETechnology eFilterType,
    IN  GUID*   aguidCategories,
    IN  ULONG   cguidCategories,
    IN  BOOL    fNeedPins,          // = TRUE // Enumerates devices for sysaudio.
    IN  BOOL    fNeedNodes,         // = TRUE
    IN  BOOL    fInstantiate)       // = TRUE // Should we instantiate.
{
    TRACE_ENTER();

    HRESULT hr = DestroyLists();
    HDEVINFO hDevInfo = NULL;

    if (SUCCEEDED(hr)
    &&  aguidCategories
    &&  !IsEqualGUIDAligned(GUID_NULL, aguidCategories[0]))
    {
        // Get a handle to the device set specified by the guid
        hDevInfo = 
            SetupDiGetClassDevs
            (
                &(aguidCategories[0]), 
                NULL, 
                NULL, 
                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
            );

        if( !CKsIrpTarget::IsValidHandle(hDevInfo) )
        {
            hr = E_FAIL;
            DebugPrintf(TRACE_NORMAL, TEXT("No devices found"));
        }
        else
        {
            // Loop through members of the set and get details for each
            for(int iClassMember = 0;;iClassMember++)
            {
                DWORD                               cbInterfaceDetails;
                SP_DEVICE_INTERFACE_DATA            DID;
                SP_DEVICE_INTERFACE_DATA            DIDAlias;
                SP_DEVINFO_DATA                     DevInfoData;
                SP_DEVICE_INTERFACE_DETAIL_DATA*    pDevInterfaceDetails;

                DID.cbSize = sizeof(DID);
                DID.Reserved = 0;
                DIDAlias.cbSize = sizeof(DIDAlias);
                DIDAlias.Reserved = 0;

                DevInfoData.cbSize = sizeof(DevInfoData);
                DevInfoData.Reserved = 0;

                BOOL fRes = 
                    SetupDiEnumDeviceInterfaces
                    (
                        hDevInfo, 
                        NULL, 
                        &(aguidCategories[0]), 
                        iClassMember,
                        &DID
                    );

                if (!fRes)
                {
                    // This just means that we've enumerate all the devices - it's not a real error
                    break;
                }

                // Get details for the device registered in this class
                cbInterfaceDetails = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + MAX_PATH * sizeof(WCHAR);
                pDevInterfaceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA*)new BYTE[cbInterfaceDetails];
                if (!pDevInterfaceDetails)
                {
                    hr = E_OUTOFMEMORY;
                    DebugPrintf(TRACE_NORMAL, TEXT("Failed to allocate SP_DEVICE_INTERFACE_DETAIL_DATA structure"));
                    break;
                }

                pDevInterfaceDetails->cbSize = sizeof(*pDevInterfaceDetails);

                fRes = 
                    SetupDiGetDeviceInterfaceDetail
                    (
                        hDevInfo, 
                        &DID, 
                        pDevInterfaceDetails, 
                        cbInterfaceDetails,
                        NULL, 
                        &DevInfoData
                    );

                if (fRes)
                {
                    // 
                    // check additional category guids which may (or may not) have been supplied
                    //
                    for(ULONG nguidCategory = 1; nguidCategory < cguidCategories && fRes; nguidCategory++)
                    {
                        fRes = 
                            SetupDiGetDeviceInterfaceAlias
                            (
                                hDevInfo,
                                &DID,
                                &(aguidCategories[nguidCategory]),
                                &DIDAlias
                            );

                        if (!fRes)
                        {
                            DebugPrintf(TRACE_LOW, TEXT("Failed to get requested DeviceInterfaceAlias"));
                        }

                        //
                        // Check if the this interface alias is enabled.
                        //
                        if (fRes)
                        {
                            if (!DIDAlias.Flags || (DIDAlias.Flags & SPINT_REMOVED))
                            {
                                fRes = FALSE;
                                DebugPrintf(TRACE_LOW, TEXT("DeviceInterfaceAlias disabled."));
                            }
                        }
                    }

                    if (fRes)
                    {
                        CKsFilter* pnewFilter = NULL;
                        HRESULT hrFilter = S_OK;

                        switch(eFilterType)
                        {
                            case eUnknown:
                                pnewFilter = new CKsFilter(pDevInterfaceDetails->DevicePath,&hrFilter);
                                break;

                            case eAudRen:
                                pnewFilter = new CKsAudRenFilter(pDevInterfaceDetails->DevicePath, &hrFilter);
                                break;

                            case eAudCap:
                                pnewFilter = new CKsAudCapFilter(pDevInterfaceDetails->DevicePath, &hrFilter);
                                break;

                            default:
                                pnewFilter = new CKsFilter(pDevInterfaceDetails->DevicePath,  &hrFilter);
                                break;
                        }

                        if (NULL == pnewFilter)
                        {
                            hrFilter = E_OUTOFMEMORY;
                        }
                        else
                        {
                            if (SUCCEEDED(hrFilter) && (fNeedNodes || fNeedPins || fInstantiate))
                            {
                                hrFilter = pnewFilter->Instantiate();

                                if (fNeedNodes && SUCCEEDED(hrFilter))
                                    hrFilter = pnewFilter->EnumerateNodes();

                                // This enumerates Devices for SysAudio devices
                                // For normal devices, it just enumerates the pins
                                if (fNeedPins && SUCCEEDED(hrFilter))
                                    hrFilter = pnewFilter->EnumeratePins();
                            }

                            if (SUCCEEDED(hrFilter))
                            {
                                if (NULL == m_listFilters.AddTail(pnewFilter))
                                {
                                    hrFilter = E_OUTOFMEMORY;
                                }
                            }
                        }

                        if (FAILED(hrFilter))
                        {
                            delete pnewFilter;
                            DebugPrintf(
                                TRACE_ERROR, 
                                TEXT("Failed to create filter for %s"), 
                                pDevInterfaceDetails->DevicePath);
                        }
                    }
                }

                delete[] (BYTE*)pDevInterfaceDetails;
            } // for
        }
    }

    if (CKsIrpTarget::IsValidHandle(hDevInfo))
        SetupDiDestroyDeviceInfoList(hDevInfo);

    // If we didn't find anything, be sure to return an error code
    if (SUCCEEDED(hr)
    &&  m_listFilters.IsEmpty())
    {
        hr = E_FAIL;
    }

    TRACE_LEAVE_HRESULT(hr);
    return hr;
}


