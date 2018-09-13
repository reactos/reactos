
/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tshooter.cpp

Abstract:

    File that implements the CTroubleshooter class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "tshooter.h"

// strings used for caller name and version.
const TCHAR* const DEVMGR_CALLERNAME = TEXT("Device Manager");
const TCHAR* const DEVMGR_CALLERVERSION = TEXT("1.0");

//
// This function tests if there are troubleshooters available
// for the given deviceid/classguid/problem combination
//
// INPUT:
//  pClassGuid  -- class guid, can be NULL
//  pDeviceID   -- device id, can be NULL
//  Problem     -- the problem number, 0 means no problem
//
// OUTPUT:
//  TRUE -- Troubleshooters are available for service
//  FALSE -- either no troubleshooters are available or an error
//       occured.
//
BOOL
CTroubleshooter::Test(
    LPGUID pClassGuid,
    LPCTSTR pDeviceID,
    ULONG Problem
    )
{
    TCHAR GuidString[MAX_GUID_STRING_LEN];
    LPTSTR pClassGuidString = NULL;
    if (pClassGuid)
    {
    if (GuidToString(pClassGuid, GuidString, ARRAYLEN(GuidString)))
    {
        pClassGuidString = GuidString;
    }
    else
        return FALSE;
    }
    DWORD TSLError;
    // do a query to see if there are any troubleshooters available
    // for the request.
    TSLError =  ::TSLLaunchDevice(m_hTSL, DEVMGR_CALLERNAME, DEVMGR_CALLERVERSION,
                pDeviceID, pClassGuidString,
                Problem ? NULL : MAKE_PROBLEM_STRING(Problem),
                FALSE);
    // TSL_WARNING_GENERAL or TSL_ERROR_GENERAL generates a series of
    // status message. We have to retreive them even though we may not
    // need them at all.
    DumpErrors(TEXT("TSLLaunchDevice"), TSLError);
    return TSL_OK == TSLError || TSL_WARNING_GENERAL == TSLError;
}

//
// This function sets the PNP Device ID to the opened TSL handle.
// Class GUID, Device ID and Problem number are cached for the handle so that
// the member function Run does not carry any parameters.
//
// INPUT:
//  DeviceID    -- the device ID. Can not be NULL
// OUTPUT:
//  TRUE -- the device ID is set successfully.
//  FALSE -- function failed.
BOOL
CTroubleshooter::SetDeviceID(
    LPCTSTR DeviceID
    )
{
    // the device id can not be set twice!
    ASSERT(!m_pDeviceID);
    ASSERT(DeviceID);
    if (!m_pDeviceID && DeviceID)
    {
    try
    {
        int len = lstrlen(DeviceID);
        m_pDeviceID = new TCHAR[len + 1];
        lstrcpyn(m_pDeviceID, DeviceID, len + 1);
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    return NULL != m_pDeviceID;
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

//
// This function sets the class GUID to the opened TSL handle.
// Class GUID, Device ID and Problem number are cached for the handle so that
// the member function Run does not carry any parameters.
//
// INPUT:
//  pClassGuid -- the class GUID. Can not be NULL
// OUTPUT:
//  TRUE -- the class GUID is set successfully.
//  FALSE -- function failed.
BOOL
CTroubleshooter::SetClassGuid(
    LPGUID pClassGuid
    )
{
    ASSERT(!m_pClassGuidString);
    ASSERT(pClassGuid);
    if (!m_pClassGuidString && pClassGuid)
    {
    try
    {
        m_pClassGuidString = new TCHAR[MAX_GUID_STRING_LEN];
        if (!GuidToString(pClassGuid, m_pClassGuidString, MAX_GUID_STRING_LEN))
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        delete [] m_pClassGuidString;
        m_pClassGuidString = NULL;
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    return NULL != m_pClassGuidString;
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

//
// This function sets the problem number to the opened TSL handle.
// Class GUID, Device ID and Problem number are cached for the handle so that
// the member function Run does not carry any parameters.
//
// INPUT:
//  Problem -- the problem number. Valid range is 0 <= Problem < CM_NUM_PROB
// OUTPUT:
//  TRUE -- the problem number is set successfully.
//  FALSE -- function failed.
BOOL
CTroubleshooter::SetProblem(
    ULONG Problem
    )
{
    if (Problem >= NUM_CM_PROB)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
    }
    m_Problem = Problem;
    return TRUE;
}

//
// This function loops through all the provided device ids and see
// if any id is accepted by the troubleshooters.
// INPUT:
//  DeviceIDs -- device ids in properly ranking order. With the
//           first one has the highest priority.
//  ClassGuid -- the class guid of the device
//  Problem   -- the problem number of the device
//
LPCTSTR
CTroubleshooter::TestIDs(
    LPCTSTR DeviceIDs,
    LPGUID ClassGuid,
    ULONG Problem
    )
{
    if (!DeviceIDs)
    return NULL;
    while (_T('\0') != DeviceIDs[0] && !Test(ClassGuid, DeviceIDs, Problem))
    {
    // advanced to next id
    DeviceIDs += lstrlen(DeviceIDs) + 1;
    }
    return (_T('\0') != DeviceIDs[0]) ? DeviceIDs : NULL;
}

//
// This function translates the given TSL error code to Window32 API error code
// INPUT:
//  TSLError -- TSL error code
// OUTPUT:
//  Window32 API error code
//
// Note: This function really translates a sub-set(or, necessary) of the
//   entire TSL error code inventory.
DWORD
CTroubleshooter::TSLError2WinError(
    DWORD TSLError
    )
{
    DWORD WinError;
    switch (TSLError)
    {
    case TSL_OK:
        WinError = ERROR_SUCCESS;
        break;
    case TSL_ERROR_BAD_HANDLE:
        WinError = ERROR_INVALID_HANDLE;
        break;
    case TSL_ERROR_OUT_OF_MEMORY:
        WinError = ERROR_NOT_ENOUGH_MEMORY;
        break;
    case TSL_ERROR_ILLFORMED_MACHINE_ID:
        WinError = ERROR_INVALID_COMPUTERNAME;
        break;
    case TSL_ERROR_BAD_MACHINE_ID:
        WinError = ERROR_REM_NOT_LIST;
        break;
    case TSL_ERROR_ILLFORMED_DEVINST_ID:
        WinError = SPAPI_E_INVALID_DEVINST_NAME;
        break;
    case TSL_ERROR_BAD_DEVINST_ID:
        WinError = SPAPI_E_NO_SUCH_DEVINST;
        break;
    defaul:
        WinError = ERROR_GEN_FAILURE;

    }
    return WinError;
}

//
// Class public functions
//


//
// This function starts the selected troubleshooter.
BOOL
CTroubleshooter::Run()
{
    if (!m_hTSL)
    {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
    }
    DWORD TSLError;
    TSLError = ::TSLLaunchDevice(m_hTSL,
                   DEVMGR_CALLERNAME,
                   DEVMGR_CALLERVERSION,
                   m_pDeviceID,
                   m_pClassGuidString,
                   MAKE_PROBLEM_STRING(m_Problem),
                   TRUE);
    // TSL_WARNING_GENERAL or TSL_ERROR_GENERAL generates a series of
    // status message. We have to retreive them even though we may not
    // need them at all.
    DumpErrors(TEXT("TSLLaunchDevice"), TSLError);
    return TSL_OK == TSLError || TSL_WARNING_GENERAL == TSLError;
}


// This function opens a handle to the general troubleshooter.
//
// INPUT:
//  MachineName  -- the full qualified machine name. NULL for local machine
// OUTPUT:
//  TRUE    -- function succeeded;
//  FLASE   -- function failed.
BOOL
CTroubleshooter::Open(
    LPCTSTR MachineName
    )
{
    if (!m_hTSL)
    {
    m_hTSL = ::TSLOpen();
    if (m_hTSL)
    {
        DWORD TSLError;
        if (MachineName && _T('\0') != MachineName[0])
        {
        // The name is a fully qualified computer name -- led
        // by the network signature : TEXT("\\\\").
        TSLError = ::TSLMachineID(m_hTSL, MachineName);
        if (TSL_OK != TSLError)
        {
            DumpErrors(TEXT("TSLMachineID"), TSLError);
            SetLastError(TSLError2WinError(TSLError));
            Close();
        }
        }
    }
    return NULL != m_hTSL;
    }
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

//
// This function opens a handle to the troubleshooters capable of
// general troubleshooting.
//
// INPUT:
//  pMachine -- The machine object
// OUTPUT:
//  TRUE -- if the handle is obtained and the function succeeded.
//  FALSE -- failed to obtain a handle to the troubleshooters
BOOL
CTroubleshooter::Open(
    CMachine* pMachine
    )
{
    if (pMachine)
    {
    return Open(pMachine->GetRemoteMachineFullName());
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

// This function opens a handle to the class troubleshooter. If it is
// not available, a handle to the general troubleshooter is obtained.
// If both failed, this function fails.
//
// INPUT:
//  MachineName -- fully qualified machine name. NULL for local machine.
//  pClassGuid  -- the class guid. Must present.
// OUTPUT:
//  TRUE        -- Troubleshooter opened.
//  FALSE       -- function failed, no troubleshooters are opened.
BOOL
CTroubleshooter::Open(
    LPCTSTR MachineName,
    LPGUID  pClassGuid
    )
{
    if (!pClassGuid)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
    }
    // first try the general troubleshooter. If it fails, then we fail

    if (Open(MachineName))
    {
    // general troubleshooter is found. Try if the class specific
    // troubleshooter can be found. If a troubleshooter can be
    // found for the class, we set the class guid for the the Run
    // member function.
    if (Test(pClassGuid) && !SetClassGuid(pClassGuid))
    {
        // troubleshooter can be found but we failed to set the
        // class guid, fail the function.
        Close();
    }
    }
    return NULL != m_hTSL;
}


//
// This function opens a handle to the troubleshooters capable of
// class troubleshooting. If no class troubleshooting is available,
// this function opens a handle to the general troubleshooting.
//
// INPUT:
//  pClass -- The class object
// OUTPUT:
//  TRUE -- if the handle is obtained and the function succeeded.
//  FALSE -- failed to obtain a handle to the troubleshooters
BOOL
CTroubleshooter::Open(
    CClass* pClass
    )
{
    if (!pClass)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
    }
    return Open(pClass->m_pMachine->GetRemoteMachineFullName(), *pClass);

}

//
// This function opens a handle to the troubleshooters capable of
// device and problem specific troubleshooting. If the specified
// troubleshooting is not available, this function searches for
// (1). the problem troubleshooting
// (2). the class troubleshooting
// (3). the general troubleshooting.
//
// INPUT:
//  pDevice -- The Device object
// OUTPUT:
//  TRUE -- if the handle is obtained and the function succeeded.
//  FALSE -- failed to obtain a handle to the troubleshooters
BOOL
CTroubleshooter::Open(
    CDevice* pDevice
    )
{
    DWORD Status, Problem;
    if (pDevice && pDevice->GetStatus(&Status, &Problem) &&
    Open(pDevice->m_pMachine))
    {
    DWORD TSLError;
    GUID ClassGuid;
    // get the device class guid
    pDevice->ClassGuid(ClassGuid);
    // Set the device instance id for TSL sniffing.
    TSLError =  ::TSLDeviceInstanceID(m_hTSL, pDevice->GetDeviceID());
    if (TSL_OK == TSLError)
    {
        TCHAR Buffer[REGSTR_VAL_MAX_HCID_LEN];
        ULONG BufferLen;
        BufferLen = sizeof(Buffer);
        // presume that there are no hardware ids for this device.
        LPCTSTR pDeviceID = NULL;
        // Hardware ids are listed in the correct ranking order with the
        // first one has the highest rank.
        // Compatible IDs have lower ranks than any hardware ids.
        // In order to find the id recognized by troubleshooters,
        // we test every hardware id in ranking order and then, every
        // comptaible id.
        //
        if (pDevice->m_pMachine->CmGetHardwareIDs(pDevice->GetDevNode(), Buffer, &BufferLen) && BufferLen)
        {
        pDeviceID = TestIDs(Buffer, &ClassGuid, Problem);
        if (!pDeviceID)
        {
            // All the hardware ids failed, try compatible ids
            BufferLen = sizeof(Buffer);
            if (pDevice->m_pMachine->CmGetCompatibleIDs(pDevice->GetDevNode(), Buffer, &BufferLen) &&
            BufferLen)
            {
            pDeviceID = TestIDs(Buffer, &ClassGuid, Problem);
            }
        }
        }
        if (pDeviceID)
        {
        // we did find a device id that troubleshooters are
        // happy with. Set it.
        if (!SetClassGuid(&ClassGuid) || !SetDeviceID(pDeviceID) ||
            !SetProblem(Problem))
        {
            Close();
        }
        }
        // no device ids accepted, try the problem
        else if (Test(NULL, NULL, Problem))
        {
        if (!SetProblem(Problem))
            Close();
        }
        // no device id or problem are accepted, try the class
        else if (Test(&ClassGuid))
        {
        if (!SetClassGuid(&ClassGuid))
            Close();
        }
        //
        // Nothing is acceptable except general troubleshooting

    }
    else
    {
        DumpErrors(TEXT("TSLDeviceInstanceID"), TSLError);
        // failed to set the device instance id.
        // invalidate the handle
        SetLastError(TSLError2WinError(TSLError));
        Close();
    }
    }
    return NULL != m_hTSL;

}


void
CTroubleshooter::DumpErrors(
    LPCTSTR APIName,
    DWORD TslError
    )
{
    if (m_hTSL && APIName  &&
    (TSL_WARNING_GENERAL == TslError || TSL_ERROR_GENERAL == TslError))
    {
    TCHAR TslMsg[255];
    TCHAR OutMsg[255];
    int Count = 0;
    DWORD Size = ARRAYLEN(TslMsg);
    if (TSL_WARNING_GENERAL == TslError)
        wsprintf(OutMsg, TEXT("%s:: Warning\n\n"), APIName);
    else
        wsprintf(OutMsg, TEXT("%s:: Error\n\n"), APIName);

    OutputDebugString(OutMsg);
    // retreive TSL error message and dump it to the debugger
    // When TSLStatus returns TSL_OK, it means we have no more
    // message.
    while (TSL_OK != ::TSLStatus(m_hTSL, Size, TslMsg))
    {
        // one more code
        Count++;
        wsprintf(OutMsg, TEXT("Msg number %d -- %s\n"), Count, TslMsg);
        OutputDebugString(OutMsg);
    }
    }
}
