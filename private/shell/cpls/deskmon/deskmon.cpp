/******************************************************************************

  Source File:  deskmon.cpp

  Main code for the advanced desktop Monitor page

  Copyright (c) 1997-1998 by Microsoft Corporation

  Change History:

  12-16-97 AndreVa - Created It

******************************************************************************/


#include    "deskmon.h"


//
// The function DeviceProperties() is implemented in DevMgr.dll; Since we don't have a devmgr.h, we
// explicitly declare it here.
// 
typedef int (WINAPI  *DEVPROPERTIES)(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    BOOL ShowDeviceTree
    );


// OLE-Registry magic number
// 42071713-76d4-11d1-8b24-00a0c9068ff3
//
GUID g_CLSID_CplExt = { 0x42071713, 0x76d4, 0x11d1,
                        { 0x8b, 0x24, 0x00, 0xa0, 0xc9, 0x06, 0x8f, 0xf3}
                      };


DESK_EXTENSION_INTERFACE DeskInterface;

static const DWORD sc_MonitorHelpIds[] =
{
    IDC_MONITOR_GRP,    IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_TYPE, 
    IDI_MONITOR,        IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_TYPE, 
    IDC_MONITORDESC,    IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_TYPE, 
    IDC_MONITORS_LIST,  IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_MONITORTYPE_LISTBOX,
    IDC_PROPERTIES,     IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_PROPERTIES,

    IDC_MONSET_GRP,     IDH_NOHELP, 
    IDC_MONSET_FREQSTR, IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_REFRESH, 
    IDC_MONSET_FREQ,    IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_REFRESH, 
    IDC_MONSET_PRUNNING_MODE,       IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_HIDEMODE_CHECKBOX, 
    IDC_MONSET_PRUNNING_MODE_DESC,  IDH_DISPLAY_SETTINGS_ADVANCED_MONITOR_HIDEMODE_CHECKBOX, 

    0, 0
};

///////////////////////////////////////////////////////////////////////////////
//
// Messagebox wrapper
//
///////////////////////////////////////////////////////////////////////////////


int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    DWORD dwTitleID,
    DWORD dwTextID)
{
    TCHAR Title[256];
    TCHAR Text[1700];

    LoadString(g_hInst, dwTextID, Text, SIZEOF(Text));
    LoadString(g_hInst, dwTitleID, Title, SIZEOF(Title));

    return (MessageBox(hwnd, Text, Title, fuStyle));
}


// Constructors / destructor
CMonitorPage::CMonitorPage(HWND hDlg)
    : m_hDlg(hDlg)
    , m_lpdmPrevious(NULL)
    , m_bCanBePruned(FALSE)
    , m_bIsPruningReadOnly(TRUE)
    , m_bIsPruningOn(FALSE)
    , m_cMonitors(0)
    , m_hMonitorsList(NULL)
    , m_lpdmOnCancel(NULL)
    , m_bOnCancelIsPruningOn(FALSE)
{
    // preconditions
    //ASSERT(NULL != hDlg);
}


CMonitorPage::~CMonitorPage()
{
}


void CMonitorPage::OnApply()
{
    long lRet = PSNRET_INVALID_NOCHANGEPAGE;
    HINSTANCE hInst;
    LPDISPLAY_SAVE_SETTINGS lpfnDisplaySaveSettings = NULL;

    hInst = LoadLibrary(TEXT("desk.cpl"));
    if (hInst)
    {
        lpfnDisplaySaveSettings = (LPDISPLAY_SAVE_SETTINGS)
                                  GetProcAddress(hInst, "DisplaySaveSettings");
        if (lpfnDisplaySaveSettings)
        {
            long lSave = lpfnDisplaySaveSettings(DeskInterface.pContext, m_hDlg);
            LPDEVMODEW lpdmCurrent = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);

            if (lSave == DISP_CHANGE_SUCCESSFUL)
            {
                //
                // Save the current mode - to restore it in case the user cancels the p. sheet
                //
                m_lpdmOnCancel = m_lpdmPrevious = lpdmCurrent;
                m_bOnCancelIsPruningOn = m_bIsPruningOn;
                lRet = PSNRET_NOERROR;
            }
            else if (lSave == DISP_CHANGE_RESTART)
            {
                //
                // User wants to reboot system.
                //
                PropSheet_RestartWindows(GetParent(m_hDlg));
                lRet = PSNRET_NOERROR;
            }
            else
            {
                //
                // Keep the apply button active
                //
                lRet = PSNRET_INVALID_NOCHANGEPAGE;
                
                RefreshFrequenciesList();
                
                BOOL bCanBePruned, bIsPruningReadOnly, bIsPruningOn;
                DeskInterface.lpfnGetPruningMode(DeskInterface.pContext, 
                                                 &bCanBePruned, 
                                                 &bIsPruningReadOnly,
                                                 &bIsPruningOn);
                if(m_bIsPruningOn != bIsPruningOn)
                    InitPruningMode();
            }
        }

        FreeLibrary(hInst);
    }

    SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, lRet);
}

    
void CMonitorPage::OnCancel()
{
    if (m_bCanBePruned && !m_bIsPruningReadOnly && 
        ((m_bOnCancelIsPruningOn != 0) != (m_bIsPruningOn != 0)))
        DeskInterface.lpfnSetPruningMode(DeskInterface.pContext, m_bOnCancelIsPruningOn);

    DeskInterface.lpfnSetSelectedMode(DeskInterface.pContext, m_lpdmOnCancel);
};


void CMonitorPage::OnInitDialog()
{
    m_hMonitorsList = GetDlgItem(m_hDlg, IDC_MONITORS_LIST); 
    HWND hSingleMonitor = GetDlgItem(m_hDlg, IDC_MONITORDESC); 
    //ASSERT((NULL != m_hMonitorsList) && (NULL != hSingleMonitor));
    ListBox_ResetContent(m_hMonitorsList);

    //
    // Get the CPL extension interfaces from IDataObject.
    //
    STGMEDIUM stgmExt;
    FORMATETC fmteExt = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_INTERFACE),
                         (DVTARGETDEVICE FAR *) NULL,
                         DVASPECT_CONTENT,
                         -1,
                         TYMED_HGLOBAL};

    HRESULT hres = g_lpdoTarget->GetData(&fmteExt, &stgmExt);

    if (SUCCEEDED(hres) && stgmExt.hGlobal)
    {
        //
        // The storage now contains Display device path (\\.\DisplayX) in UNICODE.
        //
        PDESK_EXTENSION_INTERFACE pInterface =
            (PDESK_EXTENSION_INTERFACE) GlobalLock(stgmExt.hGlobal);

        RtlCopyMemory(&DeskInterface,
                      pInterface,
                      min(pInterface->cbSize,
                      sizeof(DESK_EXTENSION_INTERFACE)));

        GlobalUnlock(stgmExt.hGlobal);
        ReleaseStgMedium(&stgmExt);
    }

    //
    // Get the adapter devnode.
    // The adapter is the parent of all monitors in the device tree.
    //
    DEVINST devInstAdapter;
    BOOL bDevInstAdapter = FALSE;

    STGMEDIUM stgmAdpId;
    FORMATETC fmteAdpId = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_ID),
                           (DVTARGETDEVICE FAR *) NULL,
                           DVASPECT_CONTENT,
                           -1,
                           TYMED_HGLOBAL};

    hres = g_lpdoTarget->GetData(&fmteAdpId, &stgmAdpId);

    if(SUCCEEDED(hres) && stgmAdpId.hGlobal)
    {
        LPWSTR pwDeviceID = (LPWSTR) GlobalLock(stgmAdpId.hGlobal);

        #ifdef UNICODE
            bDevInstAdapter = (CM_Locate_DevNodeW(&devInstAdapter, pwDeviceID, 0) == CR_SUCCESS);
        #else // UNICODE
    	    CHAR szBuffer[MAX_PATH];
            bDevInstAdapter = (BOOL)WideCharToMultiByte(CP_ACP, 0, pwDeviceID, lstrlenW(pwDeviceID) + 1,
                                                        szBuffer, MAX_PATH, NULL, NULL);
            if(bDevInstAdapter)
            {
                bDevInstAdapter = (CM_Locate_DevNodeA(&devInstAdapter, szBuffer, 0) == CR_SUCCESS);
            }
        #endif // UNICODE

        GlobalUnlock(stgmAdpId.hGlobal);
        ReleaseStgMedium(&stgmAdpId);
    }

    //
    // Get the adapter device and enum all monitors
    //
    STGMEDIUM stgmAdpDev;
    FORMATETC fmteAdpDev = {(CLIPFORMAT)RegisterClipboardFormat(DESKCPLEXT_DISPLAY_DEVICE),
                           (DVTARGETDEVICE FAR *) NULL,
                           DVASPECT_CONTENT,
                           -1,
                           TYMED_HGLOBAL};
    hres = g_lpdoTarget->GetData(&fmteAdpDev, &stgmAdpDev);

    if (SUCCEEDED(hres) && stgmAdpDev.hGlobal)
    {
        LPWSTR pwDisplayDevice = (LPWSTR)GlobalLock(stgmAdpDev.hGlobal);
        LPTSTR pBuffer = NULL;

        #ifdef UNICODE
            pBuffer = pwDisplayDevice;
        #else // UNICODE
    	    CHAR szBuffer[MAX_PATH];
            if(WideCharToMultiByte(CP_ACP, 0, pwDisplayDevice, lstrlenW(pwDisplayDevice) + 1,
                                   szBuffer, MAX_PATH, NULL, NULL))
                pBuffer = szBuffer;
        #endif // UNICODE

        if(NULL != pBuffer)
        {
            DISPLAY_DEVICE ddMon;
            BOOL bSuccess = FALSE;
            int cMonitors = 0;

            do 
            {
                ZeroMemory(&ddMon, sizeof(ddMon));
                ddMon.cb = sizeof(DISPLAY_DEVICE);

                bSuccess = EnumDisplayDevices(pBuffer, cMonitors, &ddMon, 0);
                if(bSuccess)
                {
                    ++cMonitors;

                    if(0 == m_cMonitors)
                        SendDlgItemMessage(m_hDlg, IDC_MONITORDESC, WM_SETTEXT, 0, (LPARAM)ddMon.DeviceString);

                    int nNewItem = ListBox_AddString(m_hMonitorsList, (LPTSTR)ddMon.DeviceString);
                    if(nNewItem >= 0)
                    {
                        ++m_cMonitors;

                        ListBox_SetItemData(m_hMonitorsList, nNewItem, NULL);
                        if(bDevInstAdapter)
                            SaveMonitorInstancePath(devInstAdapter, ddMon.DeviceID, nNewItem);
                    }
                }
            }
            while (bSuccess);
        }
        
        GlobalUnlock(stgmAdpDev.hGlobal);
        ReleaseStgMedium(&stgmAdpDev);
    }

    if(m_cMonitors <= 0)
    {
        TCHAR szDefaultMonitor[MAX_PATH];
        LoadString(g_hInst, IDS_DEFAULT_MONITOR, szDefaultMonitor, SIZEOF(szDefaultMonitor));
        SendDlgItemMessage(m_hDlg, IDC_MONITORDESC, WM_SETTEXT, 0, (LPARAM)szDefaultMonitor);
        EnableWindow(GetDlgItem(m_hDlg, IDC_PROPERTIES), FALSE);
    }
    else if(m_cMonitors == 1)
    {
        BOOL bEnable = ((ListBox_GetCount(m_hMonitorsList) >= 1) &&
                        (NULL != (LPTSTR)ListBox_GetItemData(m_hMonitorsList, 0)));
        EnableWindow(GetDlgItem(m_hDlg, IDC_PROPERTIES), bEnable);
    }
    else
    {
        ListBox_SetCurSel(m_hMonitorsList, 0);
        OnSelMonitorChanged();
    }

    ShowWindow(((m_cMonitors <= 1) ? m_hMonitorsList : hSingleMonitor), SW_HIDE);

    //
    // Init the pruning mode check box
    //
    InitPruningMode();
    m_bOnCancelIsPruningOn = m_bIsPruningOn;

    //
    // Save the current mode - in case the user cancels the p. sheet    
    //
    m_lpdmOnCancel = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
}


void CMonitorPage::OnDestroy()
{
    //ASSERT(NULL != m_hMonitorsList);
    int cMonitors = ListBox_GetCount(m_hMonitorsList);
    for(int nMonitor = 0; nMonitor < cMonitors; ++nMonitor)
    {
        LPTSTR pMonitorInstancePath = (LPTSTR)ListBox_GetItemData(m_hMonitorsList, nMonitor);
        if(NULL != pMonitorInstancePath)
            LocalFree(pMonitorInstancePath);
    }
}


void CMonitorPage::SaveMonitorInstancePath(DEVINST devInstAdapter, LPCTSTR pMonitorID, int nNewItem)
{
    //ASSERT(NULL != m_hMonitorsList);

    DEVINST devInstChild, devInstPrevChild;
    TCHAR szBuff[256]; // buffer used to concatenate: HARDWAREID, "\" and DRIVER
                       // this is what EnumDisplayDevice returns in DeviceID in case of a monitor
    ULONG lenBuff; // size of the buffer, in bytes

    if (CM_Get_Child(&devInstChild, devInstAdapter, 0) != CR_SUCCESS) 
        return;

    do 
    {

        //CM_DRP_HARDWAREID
        lenBuff = sizeof(szBuff) - 2 * sizeof(TCHAR); // make sure we have place to append "\"
        if (CM_Get_DevNode_Registry_Property(devInstChild,
                                             CM_DRP_HARDWAREID,
                                             NULL,
                                             szBuff,
                                             &lenBuff,
                                             0) != CR_SUCCESS)
            continue;

        // "\"
        lstrcat(szBuff, TEXT("\\"));

        //CM_DRP_DRIVER
        lenBuff = sizeof(szBuff) - lstrlen(szBuff) * sizeof(TCHAR);
        if (CM_Get_DevNode_Registry_Property(devInstChild,
                                             CM_DRP_DRIVER,
                                             NULL,
                                             szBuff + lstrlen(szBuff),
                                             &lenBuff,
                                             0) != CR_SUCCESS)
            continue;

        if (lstrcmp(szBuff, pMonitorID) == 0) 
        {
            LPTSTR pMonitorInstancePath = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof(TCHAR));
            if((NULL != pMonitorInstancePath) &&
               (CM_Get_Device_ID(devInstChild, pMonitorInstancePath, MAX_PATH, 0) == CR_SUCCESS))
                ListBox_SetItemData(m_hMonitorsList, nNewItem, (LPARAM)pMonitorInstancePath);
            break;
        }

        devInstPrevChild = devInstChild;
    } 
    while(CM_Get_Sibling(&devInstChild, devInstPrevChild, 0) == CR_SUCCESS);
}


void CMonitorPage::OnSelMonitorChanged()
{
    //ASSERT(NULL != m_hMonitorsList);
    //ASSERT(NULL != m_hDlg);
   
    //
    // Enable / Disable the Properties button
    //
    BOOL bEnable = FALSE;
    if(ListBox_GetCount(m_hMonitorsList) >= 1)
    {
        int nCurSel = ListBox_GetCurSel(m_hMonitorsList);
        if(nCurSel >= 0)
            bEnable = (NULL != (LPTSTR)ListBox_GetItemData(m_hMonitorsList, nCurSel));
    }
    EnableWindow(GetDlgItem(m_hDlg, IDC_PROPERTIES), bEnable);
}


void CMonitorPage::OnProperties()
{
    int nSelMonitor;

    if(m_cMonitors <= 0)
        nSelMonitor = -1;
    else if(m_cMonitors == 1)
        nSelMonitor = ((ListBox_GetCount(m_hMonitorsList) >= 1) ? 0 : -1);
    else
        nSelMonitor = ListBox_GetCurSel(m_hMonitorsList);

    if(nSelMonitor < 0)
        return;

    //ASSERT(nSelMonitor < ListBox_GetCount(m_hMonitorsList));

    LPTSTR pMonitorInstancePath = (LPTSTR)ListBox_GetItemData(m_hMonitorsList, nSelMonitor);
    if(NULL != pMonitorInstancePath)
    {
        HINSTANCE hinstDevMgr = LoadLibrary(TEXT("DEVMGR.DLL"));

        if (hinstDevMgr)
        {
            DEVPROPERTIES pfnDevProp = NULL;
            #ifdef UNICODE
                pfnDevProp = (DEVPROPERTIES)GetProcAddress(hinstDevMgr, "DevicePropertiesW");
            #else // UNICODE
                pfnDevProp = (DEVPROPERTIES)GetProcAddress(hinstDevMgr, "DevicePropertiesA");
            #endif // UNICODE

            if (pfnDevProp)
            {
                //Display the property sheets for this device.
                (*pfnDevProp)(m_hDlg, NULL, pMonitorInstancePath, FALSE);
            }

            FreeLibrary(hinstDevMgr);
        }
    }
}


BOOL CMonitorPage::OnSetActive()
{
    LPDEVMODEW lpdm;
    DWORD      item;
    LPDEVMODEW lpdmCurrent, lpdmPrevious;
    LPDEVMODEW lpdmTmp;
    DWORD      i = 0;
    TCHAR      achFre[50];
    TCHAR      achText[80];
    DWORD      pos;
    HWND       hFreq;
    
    InitPruningMode();
    
    //
    // Build the list of refresh rates for the currently selected mode.
    //
    lpdmCurrent = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    hFreq = GetDlgItem(m_hDlg, IDC_MONSET_FREQ);
    
    if (lpdmCurrent == NULL)
        return -1;

    if (m_lpdmPrevious) 
    {
        if (lpdmCurrent->dmBitsPerPel != m_lpdmPrevious->dmBitsPerPel ||
            lpdmCurrent->dmPelsWidth  != m_lpdmPrevious->dmPelsWidth  ||
            lpdmCurrent->dmPelsHeight != m_lpdmPrevious->dmPelsHeight) 
        {
            ComboBox_ResetContent(hFreq);
        }
    }
    m_lpdmPrevious = lpdmCurrent;
    
    while (lpdm = DeskInterface.lpfnEnumAllModes(DeskInterface.pContext, i++)) {
    
        //
        // Only show refresh frequencies for current modes.
        //
        if ((lpdmCurrent->dmBitsPerPel != lpdm->dmBitsPerPel)  ||
            (lpdmCurrent->dmPelsWidth  != lpdm->dmPelsWidth)   ||
            (lpdmCurrent->dmPelsHeight != lpdm->dmPelsHeight))
            continue;
    
        //
        // convert bit count to number of colors and make it a string
        //
        // BUGBUG should this be 0 ?
        if (lpdm->dmDisplayFrequency == 1) {
            LoadString(g_hInst, IDS_DEFFREQ, achText, sizeof(achText));
        }
        else {
            DWORD  idFreq = IDS_FREQ;
    
            if (lpdm->dmDisplayFrequency < 50)
            {
                idFreq = IDS_INTERLACED;
            }
    
            LoadString(g_hInst, idFreq, achFre, sizeof(achFre));
            wsprintf(achText, TEXT("%d %s"), lpdm->dmDisplayFrequency, achFre);
        }
    
        //
        // Insert the string in the right place
        //
        pos = 0;
    
        while (lpdmTmp = (LPDEVMODEW) ComboBox_GetItemData(hFreq, pos))  {
            if ((ULONG_PTR)lpdmTmp != CB_ERR) {
                if (lpdmTmp->dmDisplayFrequency == lpdm->dmDisplayFrequency) {
                    break;
                }
    
                if (lpdmTmp->dmDisplayFrequency < lpdm->dmDisplayFrequency) {
                    pos++;
                    continue;
                }
            }
    
            //
            // Insert it here
            //
            item = ComboBox_InsertString(hFreq, pos, achText);
            ComboBox_SetItemData(hFreq, item, lpdm); 
            break;
        }
    }
    
    //
    // Finally, set the right selection
    //
    pos = 0;
    while (lpdmTmp = (LPDEVMODEW) ComboBox_GetItemData(hFreq, pos)) {
    
        if ((ULONG_PTR)lpdmTmp == CB_ERR) {
            FmtMessageBox(m_hDlg,
                          MB_OK | MB_ICONINFORMATION,
                          IDS_BAD_REFRESH,
                          IDS_BAD_REFRESH);
            return -1;
        }
    
        if (lpdmTmp->dmDisplayFrequency == lpdmCurrent->dmDisplayFrequency) {
            ComboBox_SetCurSel(hFreq, pos);
            break;
        }
    
        pos++;
    }
    
    return 0;
}


void CMonitorPage::OnFrequencyChanged()
{
    DWORD       item;
    HWND        hFreq;
    LPDEVMODEW  lpdmSelected = NULL, lpdmCurrent = NULL;

    //
    // Save the mode back
    //
    hFreq = GetDlgItem(m_hDlg, IDC_MONSET_FREQ);
    item = ComboBox_GetCurSel(hFreq);
    if (item == LB_ERR) 
        return;

    lpdmCurrent = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    lpdmSelected = (LPDEVMODEW) ComboBox_GetItemData(hFreq, item);

    if (lpdmSelected && (lpdmSelected != lpdmCurrent))
        DeskInterface.lpfnSetSelectedMode(DeskInterface.pContext, lpdmSelected);
}


void CMonitorPage::OnPruningModeChanged()
{
    if (m_bCanBePruned && !m_bIsPruningReadOnly)
    {
        BOOL bNewIsPruningOn = (BST_UNCHECKED != IsDlgButtonChecked(m_hDlg, IDC_MONSET_PRUNNING_MODE));
        if((m_bIsPruningOn != 0) != bNewIsPruningOn)
        {
            m_bIsPruningOn = bNewIsPruningOn;
            DeskInterface.lpfnSetPruningMode(DeskInterface.pContext, m_bIsPruningOn);
            RefreshFrequenciesList();
        }
    }
}


void CMonitorPage::InitPruningMode()
    {
    m_bCanBePruned = FALSE;
    m_bIsPruningReadOnly = TRUE;
    m_bIsPruningOn = FALSE;
    
    DeskInterface.lpfnGetPruningMode(DeskInterface.pContext, 
                                     &m_bCanBePruned, 
                                     &m_bIsPruningReadOnly,
                                     &m_bIsPruningOn);
    
    BOOL bEnable = (m_bCanBePruned && !m_bIsPruningReadOnly);
    EnableWindow(GetDlgItem(m_hDlg, IDC_MONSET_PRUNNING_MODE), bEnable);
    EnableWindow(GetDlgItem(m_hDlg, IDC_MONSET_PRUNNING_MODE_DESC), bEnable);

    BOOL bChecked = (m_bCanBePruned && m_bIsPruningOn);
    CheckDlgButton(m_hDlg, IDC_MONSET_PRUNNING_MODE, bChecked);
    }


void CMonitorPage::RefreshFrequenciesList()
{
    LPDEVMODEW lpdm;
    DWORD      item;
    LPDEVMODEW lpdmCurrent, lpdmPrevious;
    LPDEVMODEW lpdmTmp;
    DWORD      i = 0;
    TCHAR      achFre[50];
    TCHAR      achText[80];
    DWORD      pos;
    HWND       hFreq;

    HWND hwndCurr = GetFocus();
    
    //
    // Build the list of refresh rates for the currently selected mode.
    //
    
    lpdmCurrent = DeskInterface.lpfnGetSelectedMode(DeskInterface.pContext);
    if (lpdmCurrent == NULL)
        return;

    hFreq = GetDlgItem(m_hDlg, IDC_MONSET_FREQ);
    ComboBox_ResetContent(hFreq);
    
    while (lpdm = DeskInterface.lpfnEnumAllModes(DeskInterface.pContext, i++))
    {

        //
        // Only show refresh frequencies for current modes.
        //
        if ((lpdmCurrent->dmBitsPerPel != lpdm->dmBitsPerPel)  ||
            (lpdmCurrent->dmPelsWidth  != lpdm->dmPelsWidth)   ||
            (lpdmCurrent->dmPelsHeight != lpdm->dmPelsHeight))
            continue;

        //
        // convert bit count to number of colors and make it a string
        //
        // BUGBUG should this be 0 ?
        if (lpdm->dmDisplayFrequency == 1)
        {
            LoadString(g_hInst, IDS_DEFFREQ, achText, sizeof(achText));
        }
        else
        {
            DWORD  idFreq = IDS_FREQ;

            if (lpdm->dmDisplayFrequency < 50)
            {
                idFreq = IDS_INTERLACED;
            }

            LoadString(g_hInst, idFreq, achFre, sizeof(achFre));
            wsprintf(achText, TEXT("%d %s"), lpdm->dmDisplayFrequency, achFre);
        }

        //
        // Insert the string in the right place
        //
        pos = 0;

        while (lpdmTmp = (LPDEVMODEW) ComboBox_GetItemData(hFreq, pos))
        {
            if ((ULONG_PTR)lpdmTmp != CB_ERR)
            {
                if (lpdmTmp->dmDisplayFrequency == lpdm->dmDisplayFrequency)
                {
                    break;
                }

                if (lpdmTmp->dmDisplayFrequency < lpdm->dmDisplayFrequency)
                {
                    pos++;
                    continue;
                }
            }

            //
            // Insert it here
            //
            item = ComboBox_InsertString(hFreq, pos, achText);
            ComboBox_SetItemData(hFreq, item, lpdm); 
            break;
        }
    }

    //
    // Finally, set the right selection
    //
    pos = 0;
    while (lpdmTmp = (LPDEVMODEW) ComboBox_GetItemData(hFreq, pos))
    {

        if ((ULONG_PTR)lpdmTmp == CB_ERR)
        {
            FmtMessageBox(m_hDlg,
                          MB_OK | MB_ICONINFORMATION,
                          IDS_BAD_REFRESH,
                          IDS_BAD_REFRESH);
            break;
        }

        if (lpdmTmp->dmDisplayFrequency == lpdmCurrent->dmDisplayFrequency)
        {
            ComboBox_SetCurSel(hFreq, pos);
            break;
        }

        pos++;
    }

    if (hwndCurr)
        SetFocus(hwndCurr);

    return;
}


#ifdef DBG

void CMonitorPage::AssertValid() const
{
    //ASSERT(m_hDlg != NULL);
    //ASSERT(!m_bIsPruningOn || m_bCanBePruned);
}

#endif


//---------------------------------------------------------------------------
//
// PropertySheeDlgProc()
//
//  The dialog procedure for the "Monitor" property sheet page.
//
//---------------------------------------------------------------------------
BOOL
CALLBACK
PropertySheeDlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NMHDR FAR *lpnm;
    CMonitorPage * pMonitorPage = (CMonitorPage*)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
    case WM_INITDIALOG:

        if (!g_lpdoTarget)
        {
            return FALSE;
        }
        else
        {
            //ASSERT(!pMonitorPage);
            pMonitorPage = new CMonitorPage(hDlg);
            if(!pMonitorPage)
                return FALSE;
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pMonitorPage);
            pMonitorPage->OnInitDialog();
        }

        break;

    case WM_DESTROY:

        if (pMonitorPage)
        {
            pMonitorPage->OnDestroy();
            SetWindowLongPtr(hDlg, DWLP_USER, NULL);
            delete pMonitorPage;
        }

        break;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_MONSET_FREQ:

            switch(GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case CBN_SELCHANGE:
                PropSheet_Changed(GetParent(hDlg), hDlg);
                if(pMonitorPage)
                   pMonitorPage->OnFrequencyChanged();
                break;

            default:
                break;
            }
            break;
            
        case IDC_PROPERTIES:
            if(pMonitorPage)
                pMonitorPage->OnProperties();
            break;

        case IDC_MONSET_PRUNNING_MODE:
            PropSheet_Changed(GetParent(hDlg), hDlg);
            if(pMonitorPage)
                pMonitorPage->OnPruningModeChanged();
            break;

        case IDC_MONITORS_LIST:

            switch(GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case LBN_SELCHANGE:
                if(pMonitorPage)
                    pMonitorPage->OnSelMonitorChanged();
                break;

            default:
                return FALSE;
            }
            break;

        default:
            return FALSE;
        }

        break;

    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:
            return (pMonitorPage && pMonitorPage->OnSetActive());

        case PSN_APPLY: 
            if(pMonitorPage)
                pMonitorPage->OnApply();
            break;

        case PSN_RESET: 
            if(pMonitorPage)
                pMonitorPage->OnCancel();
            break;
        
        default:
            return FALSE;
        }

        break;


    case WM_HELP:

        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                TEXT("display.hlp"),
                HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)sc_MonitorHelpIds);

        break;


    case WM_CONTEXTMENU:

        WinHelp((HWND)wParam,
                TEXT("display.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR)sc_MonitorHelpIds);

        break;


    default:

        return FALSE;
    }

    return TRUE;
}



