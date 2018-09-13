/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tsmain.cpp

Abstract:

    This module implements Device Manager troubleshooting supporting classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/
//


#include "devmgr.h"
#include "proppage.h"
#include "devdrvpg.h"
#include "tsmisc.h"
#include "tswizard.h"

const CMPROBLEM_INFO ProblemInfo[NUM_CM_PROB] =
{
    // no problem
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_WORKING_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NOT_CONFIGURED
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
    // CM_PROB_DEVLOADER_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_OUT_OF_MEMORY
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_IS_WRONG_TYPE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_LACKED_ARBITRATOR
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_BOOT_CONFIG_CONFLICT
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_FILTER (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DEVLOADER_NOT_FOUND (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_INVALID_DATA
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_START
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_LIAR (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NORMAL_CONFLICT
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NOT_VERIFIED (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NEED_RESTART
    {
        TRUE,
        FIX_COMMAND_RESTARTCOMPUTER,
        IDS_INST_RESTARTCOMPUTER,
        1,
        IDS_FIXIT_RESTARTCOMPUTER
    },
    // CM_PROB_REENUMERATION
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_PARTIAL_LOG_CONF
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_UNKNOWN_RESOURCE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_REINSTALL
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
    // CM_PROB_REGISTRY (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_VXDLDR (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_WILL_BE_REMOVED (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DISABLED
    {
        TRUE,
        FIX_COMMAND_ENABLEDEVICE,
        IDS_INST_ENABLEDEVICE,
        1,
        IDS_FIXIT_ENABLEDEVICE
    },
     // CM_PROB_DEVLOADER_NOT_READY (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_DEVICE_NOT_THERE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_MOVED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_TOO_EARLY
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_NO_VALID_LOG_CONF
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_FAILED_INSTALL
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
     // CM_PROB_HARDWARE_DISABLED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_CANT_SHARE_IRQ
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_ADD
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DISABLED_SERVICE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_TRANSLATION_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NO_SOFTCONFIG
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_BIOS_TABLE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_IRQ_TRANSLATION_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    }
};

//
// CProblemAgent implementation
//
CProblemAgent::CProblemAgent(
    CDevice* pDevice,
    ULONG Problem,
    BOOL SeparateProcess
    )
{
    m_pDevice = pDevice;
    m_Problem = Problem;

    ASSERT(Problem < NUM_CM_PROB);
    ASSERT(pDevice);

    m_idInstFirst = ProblemInfo[Problem].idInstFirst;
    m_idInstCount = ProblemInfo[Problem].idInstCount;
    m_idFixit = ProblemInfo[Problem].idFixit;
    m_FixCommand = ProblemInfo[Problem].FixCommand;
    m_SeparateProcess = SeparateProcess;
}

DWORD
CProblemAgent::InstructionText(
    LPTSTR Buffer,
    DWORD  BufferSize
    )
{
    TCHAR LocalBuffer[512];
    LocalBuffer[0] = _T('\0');
    SetLastError(ERROR_SUCCESS);

    if (m_idInstFirst)
    {
        TCHAR Temp[256];

        for (int i = 0; i < m_idInstCount; i++)
        {
            LoadString(g_hInstance, m_idInstFirst + i, Temp, ARRAYLEN(Temp));
            lstrcat(LocalBuffer, Temp);
        }
    }

    DWORD Len = lstrlen(LocalBuffer);

    if (BufferSize > Len) {
        lstrcpyn(Buffer, LocalBuffer, Len + 1);
    }

    else if (Len) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return Len;
}

DWORD
CProblemAgent::FixitText(
    LPTSTR Buffer,
    DWORD BufferSize
    )
{
    if (!Buffer && BufferSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    SetLastError(ERROR_SUCCESS);

    if (m_idFixit)
    {
        return LoadResourceString(m_idFixit, Buffer, BufferSize);
    }

    return 0;
}

BOOL
CProblemAgent::FixIt(
    HWND hwndOwner
    )
/*++

    Lanuches a troubleshooter based on the m_FixCommand.
    
Arguments:

    hwndOwner - Parent window handle
    
Return Value:
    TRUE if launching the troubleshooter changed the device in some way
    so that the UI on the general tab needs to be refreshed.
        
    FALSE if launching the troubleshooter did not change the device in
    any way.

--*/
{
    BOOL Result;
    SP_TROUBLESHOOTER_PARAMS tsp;
    DWORD RequiredSize;

    tsp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    tsp.ClassInstallHeader.InstallFunction = DIF_TROUBLESHOOTER;
    tsp.ChmFile[0] = TEXT('\0');
    tsp.HtmlTroubleShooter[0] = TEXT('\0');

    m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
            (PSP_CLASSINSTALL_HEADER)&tsp,
            sizeof(tsp)
            );

    //
    // If the class installer retuns NO_ERROR (SetupDiCallClassInstaller returns TRUE)
    // then don't launch the default troubleshooters because the class installer has
    // launched it's own troubleshooter
    //
    if (m_pDevice->m_pMachine->DiCallClassInstaller(DIF_TROUBLESHOOTER, *m_pDevice)) {

        return TRUE;
    
    } else if (ERROR_DI_DO_DEFAULT == GetLastError()) {

        m_pDevice->m_pMachine->DiGetClassInstallParams(*m_pDevice,
                                                       (PSP_CLASSINSTALL_HEADER)&tsp,
                                                       sizeof(tsp),
                                                       &RequiredSize
                                                       );
    }

    switch (m_FixCommand)
    {
    case FIX_COMMAND_UPGRADEDRIVERS:
        Result = UpgradeDriver(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_REINSTALL:
        Result = Reinstall(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_ENABLEDEVICE:
        Result = EnableDevice(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_STARTDEVICE:
        Result = EnableDevice(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_RESTARTCOMPUTER:
        Result = RestartComputer(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_TROUBLESHOOTER:
        Result = StartTroubleShooter(hwndOwner, m_pDevice, tsp.ChmFile, tsp.HtmlTroubleShooter);
        break;

    case FIX_COMMAND_DONOTHING:
        Result = TRUE;
        break;

    default:
        Result = FALSE;
    }

    return Result;
}


BOOL
CProblemAgent::UpgradeDriver(
    HWND hwndOwner,
    CDevice* pDevice
    )
{

    return CDeviceDriverPage::UpdateDriver(pDevice, hwndOwner, FALSE, NULL);
}

BOOL
CProblemAgent::Reinstall(
    HWND hwndOwner,
    CDevice* pDevice
    )
{

    return UpgradeDriver(hwndOwner, pDevice);
}

BOOL
CProblemAgent::EnableDevice(
    HWND hwndOwner,
    CDevice* pDevice
    )
{
    CWizard98 theSheet(hwndOwner);

    CTSEnableDeviceIntroPage* pEnableDeviceIntroPage = new CTSEnableDeviceIntroPage;

    HPROPSHEETPAGE hIntroPage = pEnableDeviceIntroPage->Create(pDevice);
    theSheet.InsertPage(hIntroPage);

    CTSEnableDeviceFinishPage* pEnableDeviceFinishPage = new CTSEnableDeviceFinishPage;

    HPROPSHEETPAGE hFinishPage = pEnableDeviceFinishPage->Create(pDevice);
    theSheet.InsertPage(hFinishPage);

    return (BOOL)theSheet.DoSheet();
}

BOOL
CProblemAgent::RestartComputer(
    HWND hwndOwner,
    CDevice* pDevice
    )
{
    HWND hwndFocus;

    if (!pDevice || !pDevice->m_pMachine)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hwndFocus = GetFocus();

    CWizard98 theSheet(hwndOwner);

    CTSRestartComputerFinishPage* pRestartComputerFinishPage = new CTSRestartComputerFinishPage;

    HPROPSHEETPAGE hPage = pRestartComputerFinishPage->Create(pDevice);
    theSheet.InsertPage(hPage);

    theSheet.DoSheet();

    // restore focus
    if (hwndFocus) {

        SetFocus(hwndFocus);
    }

    return TRUE;
}

BOOL
ParseTroubleShooter(
    LPCTSTR TSString,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    //
    // verify parameters
    //
    if (!TSString || _T('\0') == TSString[0] || !ChmFile || !HtmlTroubleShooter)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    //
    // make a copy of the string because we have to party on it
    //
    TCHAR* psz = new TCHAR[lstrlen(TSString) + 1];

    lstrcpy(psz, TSString);

    LPTSTR ChmName;
    LPTSTR ChmNameEnd;
    LPTSTR HtmName;
    LPTSTR HtmNameEnd;
    LPTSTR p;

    p = psz;

    SetLastError(ERROR_SUCCESS);

    //
    // the format of the  string is "chmfile, htmlfile"
    //
    p = SkipBlankChars(p);

    if (_T('\0') != *p)
    {
            // looking for CHM file which could be enclosed
            // inside double quote chars.
            // NOTE: not double quote chars inside double quoted string is allowed.
            if (_T('\"') == *p)
            {
                ChmName = ++p;
        
            while (_T('\"') != *p && _T('\0') != *p) {
                
                p++;
            }

                ChmNameEnd = p;
        
            if (_T('\"') == *p) {

                        p++;
            }
        
        } else {

                ChmName = p;
        
            while (!IsBlankChar(*p) && _T(',') != *p) {

                        p++;
            }

                ChmNameEnd = p;
            }

        //
            // looking for ','
        //
            p = SkipBlankChars(p);

            if (_T('\0') != *p && _T(',') == *p)
            {
                p = SkipBlankChars(p + 1);

                if (_T('\0') != *p)
                {
                        HtmName = p++;

                        while (!IsBlankChar(*p) && _T('\0') != *p) {

                            p++;
                }

                        HtmNameEnd = p;
                }
            }
    }

    if (ChmName && HtmName)
    {
        *ChmNameEnd = _T('\0');
            *HtmNameEnd = _T('\0');

        lstrcpy(ChmFile, ChmName);
        lstrcpy(HtmlTroubleShooter, HtmName);

        return TRUE;
    }

    return FALSE;
}

//
// This function looks for CHM and HTM troubleshooter files for this device.
//
// The troubleshoooter string value has the following form:
//  "TroubleShooter-xx" = "foo.chm, bar.htm"
// where xx is the problem code for the device
//
// It first looks under the devices driver key.
// If nothing is found there it looks under the class key.
// If nothing is there it looks in the default troubleshooter location
// If nothing is there it just displays the hard-coded generic troubleshooter.
//
BOOL
CProblemAgent::GetTroubleShooter(
    CDevice* pDevice,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    BOOL Result = FALSE;
    DWORD Status, Problem = 0;
    TCHAR TroubleShooterKey[MAX_PATH];
    TCHAR TroubleShooterValue[MAX_PATH * 2];
    HKEY hKey;

    try {

        if (pDevice->GetStatus(&Status, &Problem)) {

            // if the device is a phantom device, use the CM_PROB_DEVICE_NOT_THERE
            if (pDevice->IsPhantom()) {

                Problem = CM_PROB_DEVICE_NOT_THERE;
            }

            // if the device is not started and no problem is assigned to it
            // fake the problem number to be failed start.
            if (!(Status & DN_STARTED) && !Problem && pDevice->IsRAW()) {

                Problem = CM_PROB_FAILED_START;
            }
        }

        wsprintf(TroubleShooterKey, TEXT("TroubleShooter-%d"), Problem);

        //
        // First check the devices driver key
        //
        hKey = pDevice->m_pMachine->DiOpenDevRegKey(*pDevice, DICS_FLAG_GLOBAL,
                     0, DIREG_DRV, KEY_READ);

        if (INVALID_HANDLE_VALUE != hKey)
        {
            DWORD regType;
            DWORD Len = sizeof(TroubleShooterValue);

            CSafeRegistry regDrv(hKey);

            // get the TroubleShooter value from the driver key
            if (regDrv.GetValue(TroubleShooterKey, &regType,
                        (PBYTE)TroubleShooterValue,
                        &Len))
            {
                if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                    Result = TRUE;
                }
            }
        }

        //
        // If we don't have a TroubleShooter yet then try the class key
        //
        if (!Result) {

                        CClass* pClass = pDevice->GetClass();
                        ASSERT(pClass);
                        LPGUID pClassGuid = *pClass;

            hKey = pDevice->m_pMachine->DiOpenClassRegKey(pClassGuid, KEY_READ, DIOCR_INSTALLER);

            if (INVALID_HANDLE_VALUE != hKey)
            {
                DWORD regType;
                DWORD Len = sizeof(TroubleShooterValue);

                CSafeRegistry regClass(hKey);

                // get the TroubleShooter value from the class key
                if (regClass.GetValue(TroubleShooterKey, &regType,
                            (PBYTE)TroubleShooterValue,
                            &Len))
                {
                    if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                        Result = TRUE;
                    }
                }
            }
        }

        //
        // If we still don't have a TroubleShooter then try the default TroubleShooters
        // key.
        //
        if (!Result) {

            CSafeRegistry regDevMgr;
            CSafeRegistry regTroubleShooters;

            if (regDevMgr.Open(HKEY_LOCAL_MACHINE, REG_PATH_DEVICE_MANAGER) &&
                regTroubleShooters.Open(regDevMgr, REG_STR_TROUBLESHOOTERS)) {

                DWORD regType;
                DWORD Len = sizeof(TroubleShooterValue);

                // get the TroubleShooter value from the default TroubleShooters key
                if (regTroubleShooters.GetValue(TroubleShooterKey, &regType,
                            (PBYTE)TroubleShooterValue,
                            &Len))
                {
                    if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                        Result = TRUE;
                    }
                }
            }
        }

        //
        // And finally, if still not TroubleShooter we will just use the default one
        //
        if (!Result) {

            lstrcpy(ChmFile, TEXT("tshoot.chm"));
            lstrcpy(HtmlTroubleShooter, TEXT("tshardw.htm"));
            Result = TRUE;
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();

        Result = FALSE;
    }

    return Result;
}

void
CProblemAgent::LaunchHtlmTroubleShooter(
    HWND hwndOwner,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    if (m_SeparateProcess) {

        TCHAR tszCommand[MAX_PATH];
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        wsprintf(tszCommand, TEXT("hh.exe ms-its:%s::/%s"), ChmFile, HtmlTroubleShooter);

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_NORMAL;

        if (CreateProcess(NULL, tszCommand, NULL, NULL, FALSE, 0, 0, NULL, &si, &pi)) {

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }


    } else {

        HtmlHelp(hwndOwner, ChmFile, HH_DISPLAY_TOPIC, (LPARAM)HtmlTroubleShooter);
    }
}


BOOL
CProblemAgent::StartTroubleShooter(
    HWND hwndOwner,
    CDevice* pDevice,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{

    if ((ERROR_DI_DO_DEFAULT == GetLastError()) &&
        ((ChmFile[0] != TEXT('\0')) &&
         (HtmlTroubleShooter[0] != TEXT('\0')))) {

        LaunchHtlmTroubleShooter(hwndOwner, ChmFile, HtmlTroubleShooter);

    } else {

        //
        // Get CHM file and TroubleShooter file from the registry
        //

        if (GetTroubleShooter(pDevice, ChmFile, HtmlTroubleShooter)) {

            LaunchHtlmTroubleShooter(hwndOwner, ChmFile, HtmlTroubleShooter);
        }
    }

    //
    // Return FALSE since launching the troubleshooter does not change the device
    // in any way.
    //
    return FALSE;
}

CWizard98::CWizard98(
    HWND hwndParent,
    UINT MaxPages
    )
{
    m_MaxPages = 0;

    if (MaxPages && MaxPages <= 32) {

        m_MaxPages = MaxPages;
        memset(&m_psh, 0, sizeof(m_psh));
        m_psh.hInstance = g_hInstance;
        m_psh.hwndParent = hwndParent;
        m_psh.phpage = new HPROPSHEETPAGE[MaxPages];
        m_psh.dwSize = sizeof(m_psh);
        m_psh.dwFlags = PSH_WIZARD | PSH_USEICONID | PSH_USECALLBACK | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
        m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
        m_psh.pszbmHeader = MAKEINTRESOURCE(IDB_BANNER);
            PSH_STRETCHWATERMARK;
        m_psh.pszIcon = MAKEINTRESOURCE(IDI_DEVMGR);
        m_psh.pszCaption = MAKEINTRESOURCE(IDS_TROUBLESHOOTING_NAME);
        m_psh.pfnCallback = CWizard98::WizardCallback;
    }
}

INT
CWizard98::WizardCallback(
    IN HWND             hwndDlg,
    IN UINT             uMsg,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    Call back used to remove the "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter
            is one of the following values:

            PSCB_INITIALIZED - Indicates that the property sheet is
            being initialized. The lParam value is zero for this message.

            PSCB_PRECREATE      Indicates that the property sheet is about
            to be created. The hwndDlg parameter is NULL and the lParam
            parameter is a pointer to a dialog template in memory. This
            template is in the form of a DLGTEMPLATE structure followed
            by one or more DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
            meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/
{

    switch( uMsg ) {

    case PSCB_INITIALIZED:
        break;

    case PSCB_PRECREATE:
        if( lParam ){

            DLGTEMPLATE *pDlgTemplate = (DLGTEMPLATE *)lParam;
            pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
        }
        break;
    }

    return FALSE;
}

