
#ifndef __API_H__
#define __API_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    api.h

Abstract:

    header file for api.cpp

    Note that no other file can #include "api.h" because the file contains
    definitions, not just declarations.

Author:

    William Hsieh (williamh) created

Revision History:


--*/



// Exported APIs

// This function launches Device Manager as a separate process.
// This function is provided for any application who wants to
// launch Device Manager but do not want to know the detail about
// MMC.
//
// INPUT: HWND hwndParent     -- the Window Handle of the caller.
//    HINSTANCE hInstance -- The caller's module instance handle
//    LPCTSTR lpMachineName -- optional machine name. NULL for local machine.
//    int nCmdShow      -- controlling how the Device Manager window
//                 to be shown. Refer to Windows API
//                 reference manual for detail.
//  OUTPUT: TRUE if the process is created successfully.
//      FALSE if the process is not created. Use GetLastError()
//        API to retreive error code.

BOOL WINAPI
DeviceManager_ExecuteA(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCSTR   lpMachineName,
    int       nCmdShow
    );

BOOL WINAPI
DeviceManager_ExecuteW(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    );


// This API parses the given command line, creates a property sheet
// for the device ID specified in the command line and
// displays the property sheet.
//
// The command line syntax is:
// /deviceid <DeviceID> [/machinename <Machine Name>] [/showdevicetree]
//
// <DeviceID> specifiec the target device
// <MachineName> specifies the optional computer name.
// ShowDeviceTree, if specifies indicates that device tree should be displayed.
//
void WINAPI
DeviceProperties_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    );
void WINAPI
DeviceProperties_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int   nCmdShow
    );

// This function creates and display a property sheet for the given device
// if a valid device id is given. It can also display the device tree
// in the same time(with or without property sheet display).
//
// INPUT: HWND hwndParent  -- the Window Handle of the caller.
//    LPCTSTR MachineName -- optional machine name. If not specified
//               local machine is used.
//    LPCTSTR DeviceId    -- the device ID. Must be provided if
//               ShowDeviceTree is FALSE, otherwise,
//               this function returns 0 and the last
//               error is ERROR_INVALID_PARAMETER
//    BOOL ShowDeviceTree -- TRUE if to shwo device tree
//  OUTPUT:
//  0 if error, call GetLastError API to retrieve the error code.
//  1 if succeeded.
//  This function will not return to the caller until the property
//  sheet(or the device tree, if requested) dismissed.

int WINAPI
DevicePropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    BOOL ShowDeviceTree
    );

int  WINAPI
DevicePropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    BOOL ShowDeviceTree
    );


// This function returns the appropriate problem text based on the given
// device and problem number.
//
// INPUT: HMACHINE hMachine   -- Machine handle, NULL for local machine
//    DEVNODE DevNode     -- The device. This paramter is required.
//    ULONG ProblemNumber -- The CM problem number
//    LPTSTR Buffer       -- Buffer to receive the text.
//    UINT   BufferSize   -- Buffer size in chars
//  OUTPUT:
//  0 if the function failed. Call GetLastError API to retreive the
//                error code.
//  <> 0 The required buffer size in chars to receive the text.
//     not including the NULL termianted char.
//     The caller should check this value to decide it has provided
//     enough buffer. ERROR_INSUFFICIENT_BUFFER is not set
//     in this function


UINT WINAPI
DeviceProblemTextA(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPSTR Buffer,
    UINT   BufferSize
    );

UINT WINAPI
DeviceProblemTextW(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPWSTR Buffer,
    UINT   BufferSize
    );



// This function prints the machine's hardware report
//
// INPUT: LPCTSTR MachineName   -- The machine name. NULL for local machine
//    LPCTSTR FileName  -- The file name to which the report
//                 will be written to. If the file already
//                 exist, this function return FALSE.
//                 The caller should delete the file and
//                 call the function again.
//    int     ReportType    -- 0 -- print system summary
//                 1 -- print selected classes
//                 2 -- print all
//    DWORD   ClassGuids    -- number of class guid are in ClassGuidList
//                 Only required if ReportType is 1
//    LPGUID  ClassGuidList -- class guid list. Only required if
//                 ReportType is 1.
//  OUTPUT:
//  TRUE if the function succeeded.
//  FLASE if the function failed. GetLastError() returns the error code
//
//
//
//                error code.
//  <> 0 The required buffer size in chars to receive the text.
//     not including the NULL termianted char.
//     The caller should check this value to decide it has provided
//     enough buffer. ERROR_INSUFFICIENT_BUFFER is not set
//     in this function


BOOL WINAPI
DeviceManagerPrintA(
    LPCSTR MachineName,
    LPCSTR FileName,
    int    ReportType,
    DWORD   ClassGuids,
    LPGUID  ClassGuidList
    );

BOOL WINAPI
DeviceManagerPrintW(
    LPCWSTR MachineName,
    LPCWSTR FileName,
    int     ReportType,
    DWORD   ClassGuids,
    LPGUID  ClassGuidList
    );

BOOL
DeviceManagerDoPrint(
    LPCTSTR MachineName,
    LPCTSTR FileName,
    int     ReportType,
    DWORD   ClassGuids,
    LPGUID  ClassGuidList
    );
//////////////////////////////////////////////////////////////////////////
////////
////////
///////
const TCHAR*    MMC_FILE = TEXT("mmc.exe");
const TCHAR*    DEVMGR_MSC_FILE = TEXT("devmgmt.msc");
const TCHAR*    MMC_COMMAND_LINE = TEXT(" /s ");
const TCHAR*    DEVMGR_MACHINENAME_OPTION = TEXT(" /dmmachinename %s");
const TCHAR*    DEVMGR_DEVICEID_OPTION = TEXT(" /dmdeviceid %s");
const TCHAR*    DEVMGR_COMMAND_OPTION = TEXT(" /dmcommand %s");
const TCHAR*    DEVMGR_CMD_PROPERTY  = TEXT("property");

const TCHAR*    RUNDLL_MACHINENAME     = TEXT("machinename");
const TCHAR*    RUNDLL_DEVICEID        = TEXT("deviceid");
const TCHAR*    RUNDLL_SHOWDEVICETREE  = TEXT("showdevicetree");


void
ReportCmdLineError(
    HWND hwndParent,
    int  ErrorStringID,
    LPCTSTR Caption = NULL
    );
BOOL AddPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    );

int
PropertyRunDeviceTree(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID
    );



int
DeviceProperties(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    BOOL ShowDeviceTree
    );

void
DeviceProperties_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    );

BOOL
DeviceManager_Execute(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR   lpMachineName,
    int       nCmdShow
    );


int
WINAPI
DeviceTroubleshootingA(
    HWND hwndParent,
    LPCSTR  MachineName,
    LPGUID  pClassGuid,
    LPCSTR  DeviceId
    );
int
WINAPI
DeviceTroubleshootingW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPGUID  pClassGuid,
    LPCWSTR DeviceId
    );

int
WINAPI
DeviceTroubleshooting(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPGUID  pClassGuid,
    LPCTSTR DeviceId
    );



int
DeviceAdvancedPropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceId
    );

int
DeviceAdvancedPropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    );
int
DeviceAdvancedProperties(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    );

int
WINAPI
DeviceProblemWizardA(
    HWND      hwndParent,
    LPCSTR    MachineName,
    LPCSTR    DeviceId
    );

int
WINAPI
DeviceProblemWizardW(
    HWND    hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    );

int
DeviceProblemWizard(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    );

// Object to parse command line passed in the lpCmdLine parameter
// passed in DeviceProperties_RunDLL APIs
//
class CRunDLLCommandLine : public CCommandLine
{
public:
    CRunDLLCommandLine() : m_ShowDeviceTree(FALSE)
    {}
    virtual void ParseParam(LPCTSTR Param, BOOL bFlag, BOOL bLast)
    {
    if (bFlag)
    {
        if (!lstrcmpi(RUNDLL_MACHINENAME, Param))
        m_WaitMachineName = TRUE;
        if (!lstrcmpi(RUNDLL_DEVICEID, Param))
        m_WaitDeviceID = TRUE;
        if (!lstrcmpi(RUNDLL_SHOWDEVICETREE, Param))
        m_ShowDeviceTree = TRUE;
    }
    else
    {
        if (m_WaitMachineName)
        {
        m_strMachineName = Param;
        m_WaitMachineName = FALSE;
        }
        if (m_WaitDeviceID)
        {
        m_strDeviceID = Param;
        m_WaitDeviceID = FALSE;
        }
    }
    }
    LPCTSTR GetMachineName()
    {
     return (m_strMachineName.IsEmpty()) ? NULL : (LPCTSTR)m_strMachineName;
    }
    LPCTSTR GetDeviceID()
    {
     return (m_strDeviceID.IsEmpty()) ? NULL : (LPCTSTR)m_strDeviceID;
    }
    BOOL  ToShowDeviceTree()
    {
    return m_ShowDeviceTree;
    }
private:
    BOOL    m_WaitMachineName;
    BOOL    m_WaitDeviceID;
    String  m_strDeviceID;
    String  m_strMachineName;
    BOOL    m_ShowDeviceTree;
};

//
// Object to return the corresponding LPTSTR for the given
// string.
//

class CTString
{
public:
    CTString(LPCWSTR pWString);
    CTString(LPCSTR pString);
    ~CTString()
    {
    if (m_Allocated && m_pTString)
        delete [] m_pTString;
    }
    operator LPCTSTR()
    {
    return (LPCTSTR)m_pTString;
    }
private:
    LPTSTR  m_pTString;
    BOOL    m_Allocated;
};


CTString::CTString(
    LPCWSTR pWString
    )
{
    m_pTString = NULL;
    m_Allocated = FALSE;
#ifdef UNICODE
    m_pTString = (LPTSTR)pWString;
#else
    int wLen = pWString ? wcslen(pWString) : 0;
    if (wLen)
    {
    int tLen;
    tLen = WideCharToMultiByte(CP_ACP, 0, pWString, wLen, NULL, 0, NULL, NULL);
    if (tLen)
    {
        m_pTString = new TCHAR[tLen + 1];
        WideCharToMultiByte(CP_ACP, 0, pWString, wLen, m_pTString, tLen, NULL, NULL);
        m_pTString[tLen] = _T('\0');
    }
    m_Allocated = TRUE;
    }
#endif
}

CTString::CTString(
    LPCSTR pAString
    )
{
    m_pTString = NULL;
    m_Allocated = FALSE;
#ifndef UNICODE
    m_pTString = (LPTSTR)pAString;
#else
    int aLen = pAString ? strlen(pAString) : 0;
    if (aLen)
    {
    int tLen;
    tLen = MultiByteToWideChar(CP_ACP, 0, pAString, aLen, NULL, 0);
    if (tLen)
    {
        m_pTString = new TCHAR[tLen + 1];
        MultiByteToWideChar(CP_ACP, 0, pAString, aLen, m_pTString, tLen);
        m_pTString[tLen] = _T('\0');
    }
    m_Allocated = TRUE;
    }
#endif
}
#endif // __API_H__
