/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devpopg.cpp

Abstract:

    This module implements CDevicePowerMgmtPage -- device power management
    property page

Author:

    William Hsieh (williamh) created

Revision History:


--*/
// devdrvpg.cpp : implementation file
//

#include "devmgr.h"
#include "devpopg.h"

extern "C" {
#include <initguid.h>
#include <wdmguid.h>
}


//
// help topic ids
//
const DWORD g_a15HelpIDs[]=
{
    IDC_DEVPOWER_DESC,  IDH_DISABLEHELP,
    IDC_DEVPOWER_ICON,  IDH_DISABLEHELP,
    IDC_DEVPOWER_WAKEENABLE,    IDH_DEVMGR_PWRMGR_WAKEENABLE,
    IDC_DEVPOWER_SHUTDOWNENABLE, IDH_DEVMGR_PWRMGR_SHUTDOWNENABLE,
    IDC_DEVPOWER_MESSAGE, IDH_DISABLEHELP,
    0,0

};

BOOL
CDevicePowerMgmtPage::OnInitDialog(
    LPPROPSHEETPAGE ppsp
    )
{
    //
    // Notify CPropSheetData about the page creation
    // the controls will be initialize in UpdateControls virtual function.
    //
    m_pDevice->m_psd.PageCreateNotify(m_hDlg);
    
    BOOLEAN Enabled;

    //
    // First see if the device is able to wake the system
    //
    if (m_poWakeEnable.Open(m_pDevice->GetDeviceID()))
    {
        m_poWakeEnable.Get(Enabled);
        ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE), BM_SETCHECK,
                      Enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    else
    {
        EnableWindow(GetControl(IDC_DEVPOWER_WAKEENABLE), FALSE);
    }

    //
    // See if the device can be turned off to save power
    //
    if (m_poShutdownEnable.Open(m_pDevice->GetDeviceID()))
    {
        m_poShutdownEnable.Get(Enabled);
        ::SendMessage(GetControl(IDC_DEVPOWER_SHUTDOWNENABLE), BM_SETCHECK,
                     Enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    else
    {
        EnableWindow(GetControl(IDC_DEVPOWER_SHUTDOWNENABLE), FALSE);
    }
    
    return CPropSheetPage::OnInitDialog(ppsp);
}

#if ENABLE_POWER_TIMEOUTS
BOOL
CDevicePowerMgmtPage::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    BOOLEAN Enabled;
    // watch out for the IDC_DEVPOWER_SHUTDOWNENABLE
    if (BN_CLICKED == HIWORD(wParam) && IDC_DEVPOWER_SHUTDOWNENABLE == LOWORD(wParam))
    {
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_SHUTDOWNENABLE),
                                                BM_GETCHECK, 0, 0);
        if (Enabled)
        {
            // enable the controls
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_SPIN), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_EDIT), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_SPIN), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_EDIT), TRUE);
            DWORD ConservationIdleTime, PerformanceIdleTime;
            m_poTimeout.Get(ConservationIdleTime, PerformanceIdleTime);
            if (ConservationIdleTime > UD_MAXVAL)
                ConservationIdleTime = UD_MAXVAL;
            if (PerformanceIdleTime > UD_MAXVAL)
                PerformanceIdleTime = UD_MAXVAL;
            // this device support timeout, initialize the value
            ::SendMessage(GetControl(IDC_DEVPOWER_CON_SPIN), UDM_SETPOS, 0,
                                     (LPARAM)ConservationIdleTime);

            ::SendMessage(GetControl(IDC_DEVPOWER_PER_SPIN), UDM_SETPOS, 0,
                                     (LPARAM)PerformanceIdleTime);
        }
        else
        {
            // disable the controls
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_SPIN), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_EDIT), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_SPIN), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_EDIT), FALSE);
        }
    }
    return FALSE;
}
#endif

//
// This function saves the settings(if any)
//
BOOL
CDevicePowerMgmtPage::OnApply()
{
    BOOLEAN Enabled;
    if (m_poWakeEnable.IsOpened() && IsWindowEnabled(GetControl(IDC_DEVPOWER_WAKEENABLE)))
    {
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_WAKEENABLE),
                                               BM_GETCHECK, 0, 0);
        m_poWakeEnable.Set(Enabled);
    }
    if (m_poShutdownEnable.IsOpened() && IsWindowEnabled(GetControl(IDC_DEVPOWER_SHUTDOWNENABLE)))
    {
        Enabled = BST_CHECKED == ::SendMessage(GetControl(IDC_DEVPOWER_SHUTDOWNENABLE),
                                              BM_GETCHECK, 0, 0);
        m_poShutdownEnable.Set(Enabled);

#if ENABLE_POWER_TIMEOUTS
        // Timeout is valid only if shutdown is enabled.
        if (Enabled && m_poTimeout.IsOpened())
        {
            LRESULT lResult;
            ULONG ConservationIdleTime;
            ULONG PerformanceIdleTime;
            lResult = ::SendMessage(GetControl(IDC_DEVPOWER_CON_SPIN),
                                    UDM_GETPOS, 0, 0);
            ConservationIdleTime = LOWORD(lResult);

            lResult = ::SendMessage(GetControl(IDC_DEVPOWER_PER_SPIN),
                                    UDM_GETPOS, 0, 0);
            PerformanceIdleTime = LOWORD(lResult);
            m_poTimeout.Set(ConservationIdleTime, PerformanceIdleTime);
        }
#endif  // #if ENABLE_POWER_TIMEOUTS
    }
    return FALSE;
}

//
// This function refreshes every control in the dialog. It may be called
// when the dialog is being initialized
//
void
CDevicePowerMgmtPage::UpdateControls(
    LPARAM lParam
    )
{
    if (lParam)
    {
        m_pDevice = (CDevice*) lParam;
    }

    try
    {

        HICON hIconOld;
        m_IDCicon = IDC_DEVPOWER_ICON;  // Save for cleanup in OnDestroy.
        hIconOld = (HICON)SendDlgItemMessage(m_hDlg, IDC_DEVPOWER_ICON, STM_SETICON,
                                      (WPARAM)(m_pDevice->LoadClassIcon()),
                                      0
                                      );
        if (hIconOld)
        {
            DestroyIcon(hIconOld);
        }

        SetDlgItemText(m_hDlg, IDC_DEVPOWER_DESC, m_pDevice->GetDisplayName());

        //
        // Get any power message that the class installer might want to display
        //
        SP_POWERMESSAGEWAKE_PARAMS pmp;
        DWORD RequiredSize;

        pmp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        pmp.ClassInstallHeader.InstallFunction = DIF_POWERMESSAGEWAKE;
        pmp.PowerMessageWake[0] = TEXT('\0');

        m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
                (PSP_CLASSINSTALL_HEADER)&pmp,
                sizeof(pmp)
                );

        //
        // If the class installer returns NO_ERROR and there is text to display in the 
        // PowerMessageWake field of the SP_POWERMESSAGEWAKE_PARAMS structure then display
        // the text.
        //
        if ((m_pDevice->m_pMachine->DiCallClassInstaller(DIF_POWERMESSAGEWAKE, *m_pDevice)) &&
            (m_pDevice->m_pMachine->DiGetClassInstallParams(*m_pDevice,
                                                            (PSP_CLASSINSTALL_HEADER)&pmp,
                                                            sizeof(pmp),
                                                            &RequiredSize)) &&
            (pmp.PowerMessageWake[0] != TEXT('\0'))) {

            SetDlgItemText(m_hDlg, IDC_DEVPOWER_MESSAGE, pmp.PowerMessageWake);
        }


#if ENABLE_POWER_TIMEOUTS
        // idle time is valid only if turning off the device is enabled
        BOOLEAN ShutdownEnabled;
        // not negative values are allowed
        ::SendMessage(GetControl(IDC_DEVPOWER_CON_SPIN), UDM_SETRANGE, 0,
                                 (LPARAM)MAKELONG(UD_MAXVAL, 0));
        ::SendMessage(GetControl(IDC_DEVPOWER_PER_SPIN), UDM_SETRANGE, 0,
                                 (LPARAM)MAKELONG(UD_MAXVAL, 0));
        if (m_poShutdownEnable.Get(ShutdownEnabled) &&
            m_poTimeout.Open(m_pDevice->GetDeviceID()) && ShutdownEnabled)
        {
            ULONG ConservationIdleTime, PerformanceIdleTime;
            // enable the controls
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_SPIN), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_EDIT), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_SPIN), TRUE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_EDIT), TRUE);
            m_poTimeout.Get(ConservationIdleTime, PerformanceIdleTime);
            if (ConservationIdleTime > UD_MAXVAL)
                ConservationIdleTime = UD_MAXVAL;
            if (PerformanceIdleTime > UD_MAXVAL)
                PerformanceIdleTime = UD_MAXVAL;
            // this device support timeout, initialize the value
            ::SendMessage(GetControl(IDC_DEVPOWER_CON_SPIN), UDM_SETPOS, 0,
                                     (LPARAM)ConservationIdleTime);

            ::SendMessage(GetControl(IDC_DEVPOWER_PER_SPIN), UDM_SETPOS, 0,
                                     (LPARAM)PerformanceIdleTime);
        }
        else
        {
            // either device can not be turned off or it is currently
            // disabled. Disable the time out
            ::SendMessage(GetControl(IDC_DEVPOWER_CON_SPIN), UDM_SETPOS,0,
                          (LPARAM)0);

            ::SendMessage(GetControl(IDC_DEVPOWER_PER_SPIN), UDM_SETPOS,0,
                          (LPARAM)0);
            // disable the controls
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_SPIN), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_CON_EDIT), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_SPIN), FALSE);
            ::EnableWindow(GetControl(IDC_DEVPOWER_PER_EDIT), FALSE);
        }
#endif
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        // report memory error
        MsgBoxParam(m_hDlg, 0, 0, 0);
    }
}

BOOL
CDevicePowerMgmtPage::OnHelp(
    LPHELPINFO pHelpInfo
    )
{
    WinHelp((HWND)pHelpInfo->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)g_a15HelpIDs);
    return FALSE;
}


BOOL
CDevicePowerMgmtPage::OnContextMenu(
    HWND hWnd,
    WORD xPos,
    WORD yPos
    )
{
    WinHelp(hWnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)g_a15HelpIDs);
    return FALSE;
}

//
// This function enables/disables the device power capability
// INPUT:
//      fEnable -- TRUE  to enable
//              -- FALSE to disable
// OUTPUT:
//      TRUE if the state is set
//      FALSE if the state is not set.
BOOL
CPowerEnable::Set(
    BOOLEAN fEnable
    )
{
    if (IsOpened())
    {
        DWORD Error;
        BOOLEAN fNewValue = fEnable;
        Error = WmiSetSingleInstance(m_hWmiBlock, m_DevInstId, m_Version,
                                     sizeof(fNewValue), &fNewValue);

        // get the value back to see if the change is really succeeded.
        if (ERROR_SUCCESS == Error && Get(fNewValue) && fNewValue == fEnable)
            return TRUE;
    }
    return FALSE;
}

BOOL
CPowerEnable::Get(
    BOOLEAN& fEnable
    )
{
    if (IsOpened())
    {
        ULONG Size = m_WmiInstDataSize;
        DWORD Error;

        Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &Size, m_pWmiInstData);
        if (ERROR_SUCCESS == Error && Size == m_WmiInstDataSize &&
            m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock &&
            m_Version == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->WnodeHeader.Version)
        {
            fEnable = *((BOOLEAN*)(m_pWmiInstData + ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->DataBlockOffset));
            return TRUE;
        }
    }
    return FALSE;
}
//
// Function to open the wmi block.
// INPUT:
//      DeviceId -- the device id
// OUTPUT:
//      TRUE  if the device can be turned off
//      FALSE if the device can not be turned off.
BOOL
CPowerEnable::Open(
    LPCTSTR DeviceId
    )
{
    if (!DeviceId)
        return FALSE;
    // do nothing if already opened
    if (IsOpened())
        return TRUE;
    // WMI requires an instance number. We simply
    // append a '_0' here because we are only interested in
    // the device as a whole. A mouse can have one instance for
    // left button, one for the left button and one for
    // the movement although this arrangement is not pratical
    // at all. If the device plans to do so, the instance 0
    // should be reserved to control the device as a whole.
    //
    int len = lstrlen(DeviceId);
    if (len >= ARRAYLEN(m_DevInstId) - 2)
        return FALSE;
    lstrcpyn(m_DevInstId, DeviceId, len + 1);
    m_DevInstId[len] = _T('_');
    m_DevInstId[len + 1] = _T('0');
    m_DevInstId[len + 2] = _T('\0');

    ULONG Error;
    Error = WmiOpenBlock(&m_wmiGuid, 0, &m_hWmiBlock);
    if (ERROR_SUCCESS == Error)
    {

        // get the required block size.
        ULONG BufferSize = 0;
        Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &BufferSize, NULL);
        if (BufferSize && Error == ERROR_INSUFFICIENT_BUFFER)
        {
            // the device does support the GUID, remember the size
            // and allocate a buffer to the data block.
            m_WmiInstDataSize = BufferSize;
            m_pWmiInstData = new BYTE[BufferSize];
            Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId, &BufferSize, m_pWmiInstData);
            if (ERROR_SUCCESS == Error &&
                m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock)
            {
                // remember the version
                m_Version = ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->WnodeHeader.Version;
                return TRUE;
            }

        }
        Close();
    }
    SetLastError(Error);
    return FALSE;
}


#if ENABLE_POWER_TIMEOUTS
//
// CPowerTimeout implementation
//

BOOL
CPowerTimeout::Open(
    LPCTSTR DeviceId
    )
{
    if (!DeviceId)
        return FALSE;
    // do nothing if already opened
    if (IsOpened())
        return TRUE;
    // WMI requires an instance number. We simply
    // append a '_0' here because we are only interested in
    // the device as a whole. A mouse can have one instance for
    // left button, one for the left button and one for
    // the movement although this arrangement is not pratical
    // at all. If the device plans to do so, the instance 0
    // should be reserved to control the device as a whole.
    //
    int len = lstrlen(DeviceId);
    if (len >= ARRAYLEN(m_DevInstId) - 2)
        return FALSE;
    lstrcpyn(m_DevInstId, DeviceId);
    m_DevInstId[len] = _T('_');
    m_DevInstId[len + 1] = _T('0');
    m_DevInstId[len + 2] = _T('\0');

    ULONG Error;
    GUID Guid = GUID_POWER_DEVICE_TIMEOUTS;
    Error = WmiOpenBlock(&Guid, 0, &m_hWmiBlock);
    if (ERROR_SUCCESS == Error)
    {
        ULONG BufferSize = 0;
        Error = WmiQuerySingleInstance(m_hWmiBlock,
                                       m_DevInstId,
                                       &BufferSize,
                                       NULL);
        if (BufferSize && ERROR_INSUFFICIENT_BUFFER == Error)
        {
            m_pWmiInstDataSize = BufferSize;
            m_pWmiInstData = new BYTE[BufferSize];
            Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId,
                                           &BufferSize, m_pWmiInstData);
            if (ERROR_SUCCESS == Error &&
                m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock)
            {
                // everything looks fine, remember the version
                m_Version = ((PWNODE_SINGLE_INSTANCE)m_pWinInstData)->WnodeHeader.Version;
                return TRUE;
            }
        }
        Close();
    }
    return FALSE;
}

BOOL
CPowerTimeout::Get(
    ULONG& ConservationIdleTime,
    ULONG& PerformanceIdleTime
    )
{
    if (IsOpened())
    {
        DWORD Error;
        Error = WmiQuerySingleInstance(m_hWmiBlock, m_DevInstId,
                                       m_WmiInstDataSize, m_pWmiInstData);
        if (ERROR_SUCCESS == Error &&
            m_Version == ((PWNODE_SINGLE_INSTANCE)m_pWinInstData)->WnodeHeader.Version &&
            m_DataBlockSize == ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->SizeDataBlock
            )
        {
            PDM_POWER_DEVICE_TIMEOUTS pTimeouts;
            pTimeouts = (PDM_POWER_DEVICE_TIMEOUTS)(m_pWmiInstData + ((PWNODE_SINGLE_INSTANCE)m_pWmiInstData)->DataBlockOffset);
            ConservationIdleTime = pTimeouts.ConservationIdleTime;
            PerformanceIdleTime = pTimeouts.PerformanceIdleTime;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
CPowerTimeout::Set(
    ULONG ConservationIdleTime,
    ULONG PerformanceIdleTime
    )
{
    if (IsOpened())
    {
        ULONG NewConservationIdleTime, NewPerformanceIdleTime;
        DWORD Error;

        DM_POWER_DEVICE_TIMEOUTS Timeouts;
        Timeouts.ConservationIdleTime = ConservationIdleTime;
        Timeouts.PerformanceIdleTime = PerformanceIdleTime;
        Error = WmiSetSingleInstance(m_hWmiBlock, m_DevInstId, m_Version,
                                     m_DataBlockSize, &Timeouts);
        if (ERROR_SUCCESS == Error &&
            Get(NewConservationIdleTime, NewPerformanceIdleTime) &&
            NewConservationIdleTime == ConservationIdleTime &&
            NewPerformanceIdleTime == PerformanceIdleTime)
        {
            return TRUE;

        }
    }
    return FALSE;
}

#endif
