/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       DETAILS.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Implements the Detailed Batery Information dialog.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>

#include <devioctl.h>
#include <ntpoapi.h>
#include <poclass.h>

#include "batmeter.h"
#include "bmresid.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

extern HINSTANCE   g_hInstance;             // Global instance handle of this DLL.
extern const DWORD g_ContextMenuHelpIDs[];  //Help ID's.

/*******************************************************************************
*
*  AppendStrID
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL AppendStrID(LPTSTR lpszDest, UINT uiID, BOOLEAN bUseComma)
{
    LPTSTR lpsz;

    if (lpszDest) {
        lpsz = LoadDynamicString(uiID);
        if (lpsz) {
            if (bUseComma) {
                lstrcat(lpszDest, TEXT(", "));
            }
            lstrcat(lpszDest, lpsz);
            LocalFree(lpsz);
            return TRUE;
        }
    }
    return FALSE;
}
/*******************************************************************************
*
*  GetBatStatusDetails
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL GetBatStatusDetails(HWND hWnd, PBATTERY_STATE pbs)
{
    BATTERY_STATUS              bs;
    BATTERY_WAIT_STATUS         bws;
    DWORD                       dwByteCount;
    BATTERY_INFORMATION         bi;
    BATTERY_QUERY_INFORMATION   bqi;
    TCHAR                       szChem[5], szStatus[128];
    CHAR                        szaChem[5];
    LPTSTR                      lpsz;
    UINT                        uiIDS;
    BOOLEAN                     bUseComma;

    bqi.BatteryTag = pbs->ulTag;
    bqi.InformationLevel = BatteryInformation;
    bqi.AtRate = 0;

    if (DeviceIoControl(pbs->hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                        &bqi, sizeof(bqi),
                        &bi,  sizeof(bi),
                        &dwByteCount, NULL)) {

        // Set chemistry.
        memcpy(szaChem, bi.Chemistry, 4);
        szaChem[4] = 0;

        if (szaChem[0]) {
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, 0, szaChem, -1, szChem, 5);
            SetDlgItemText(hWnd, IDC_CHEM, szChem);
#else
            SetDlgItemText(hWnd, IDC_CHEM, szaChem);
#endif
        }
        else {
            ShowWindow(GetDlgItem(hWnd, IDC_CHEM), SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_CHEMISTRY), SW_HIDE);
        }

        // Set up BATTERY_WAIT_STATUS for immediate return.
        memset(&bws, 0, sizeof(BATTERY_WAIT_STATUS));
        bws.BatteryTag = pbs->ulTag;

        if (DeviceIoControl(pbs->hDevice, IOCTL_BATTERY_QUERY_STATUS,
                            &bws, sizeof(BATTERY_WAIT_STATUS),
                            &bs,  sizeof(BATTERY_STATUS),
                            &dwByteCount, NULL)) {

            szStatus[0] = '\0';
            bUseComma = FALSE;
            if (bs.PowerState & BATTERY_POWER_ON_LINE) {
                AppendStrID(szStatus, IDS_BATTERY_POWER_ON_LINE, bUseComma);
                bUseComma = TRUE;
            }
            if (bs.PowerState & BATTERY_DISCHARGING) {
                AppendStrID(szStatus, IDS_BATTERY_DISCHARGING, bUseComma);
                bUseComma = TRUE;
            }
            if (bs.PowerState & BATTERY_CHARGING) {
                AppendStrID(szStatus, IDS_BATTERY_CHARGING, bUseComma);
                bUseComma = TRUE;
            }
            if (bs.PowerState & BATTERY_CRITICAL) {
                AppendStrID(szStatus, IDS_BATTERY_CRITICAL, bUseComma);
                bUseComma = TRUE;
            }
            SetDlgItemText(hWnd, IDC_STATE, szStatus);
            return TRUE;
        }
    }
    return FALSE;
}

/*******************************************************************************
*
*  GetBatQueryInfo
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL GetBatQueryInfo(
    PBATTERY_STATE              pbs,
    PBATTERY_QUERY_INFORMATION  pbqi,
    PULONG                      pulData,
    ULONG                       ulSize
)
{
    DWORD dwByteCount;

    if (DeviceIoControl(pbs->hDevice, IOCTL_BATTERY_QUERY_INFORMATION,
                        pbqi, sizeof(BATTERY_QUERY_INFORMATION),
                        pulData,  ulSize,
                        &dwByteCount, NULL)) {
        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
*
*  GetAndSetBatQueryInfoText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL GetAndSetBatQueryInfoText(
    HWND                        hWnd,
    PBATTERY_STATE              pbs,
    PBATTERY_QUERY_INFORMATION  pbqi,
    UINT                        uiIDS,
    UINT                        uiLabelID
)
{
    WCHAR szBatStr[MAX_BATTERY_STRING_SIZE];

    memset(szBatStr, 0, sizeof(szBatStr));
    if (GetBatQueryInfo(pbs, pbqi, (PULONG)szBatStr, sizeof(szBatStr))) {
#ifdef UNICODE
        if (lstrcmp(szBatStr, TEXT(""))) {
            SetDlgItemText(hWnd, uiIDS, szBatStr);
            return TRUE;
        }
#else
        CHAR szaBatStr[MAX_BATTERY_STRING_SIZE];

        szaBatStr[0] = '\0';
        WideCharToMultiByte(CP_ACP, 0, szBatStr, -1,
                            szaBatStr, MAX_BATTERY_STRING_SIZE, NULL, NULL);
        if (szaBatStr[0]) {
            SetDlgItemText(hWnd, uiIDS, szaBatStr);
            return TRUE;
        }
#endif
    }
    ShowWindow(GetDlgItem(hWnd, uiIDS), SW_HIDE);
    ShowWindow(GetDlgItem(hWnd, uiLabelID), SW_HIDE);
    return FALSE;
}

/*******************************************************************************
*
*  GetBatOptionalDetails
*
*  DESCRIPTION:
*   Get optional battery data and set the dialog control.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL GetBatOptionalDetails(HWND hWnd, PBATTERY_STATE pbs)
{
    BATTERY_QUERY_INFORMATION   bqi;
    ULONG                       ulData;
    LPTSTR                      lpsz = NULL;
    BATTERY_MANUFACTURE_DATE    bmd;
    TCHAR                       szDateBuf[128];
    SYSTEMTIME                  stDate;

    bqi.BatteryTag = pbs->ulTag;
    bqi.InformationLevel = BatteryManufactureDate;
    bqi.AtRate = 0;
    
    if (GetBatQueryInfo(pbs, &bqi, (PULONG)&bmd,
                        sizeof(BATTERY_MANUFACTURE_DATE))) {

        memset(&stDate, 0, sizeof(SYSTEMTIME));
        stDate.wYear  = (WORD) bmd.Year;
        stDate.wMonth = (WORD) bmd.Month;
        stDate.wDay   = (WORD) bmd.Day;

        GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
                      &stDate, NULL, szDateBuf, 128);
        SetDlgItemText(hWnd, IDC_BATMANDATE, szDateBuf);
    }
    else {
        ShowWindow(GetDlgItem(hWnd, IDC_BATMANDATE), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_DATEMANUFACTURED), SW_HIDE);
    }
    bqi.InformationLevel = BatteryDeviceName;
    GetAndSetBatQueryInfoText(hWnd, pbs, &bqi, IDC_DEVNAME, IDC_BATTERYNAME);

    bqi.InformationLevel = BatteryManufactureName;
    GetAndSetBatQueryInfoText(hWnd, pbs, &bqi, IDC_BATMANNAME, IDC_MANUFACTURE);

    bqi.InformationLevel = BatteryUniqueID;
    GetAndSetBatQueryInfoText(hWnd, pbs, &bqi, IDC_BATID, IDC_UNIQUEID);

    return TRUE;
}

/*******************************************************************************
*
*  InitBatDetailDlg
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL InitBatDetailDialogs(HWND hWnd, PBATTERY_STATE pbs)
{
    LPTSTR                      lpsz;
    DWORD                       dwByteCount;

    lpsz = LoadDynamicString(IDS_BATTERYNUMDETAILS, pbs->ulBatNum);
    if (lpsz) {
        SetWindowText(hWnd, lpsz);
        LocalFree(lpsz);
    }

    if (GetBatOptionalDetails(hWnd, pbs)) {
        return GetBatStatusDetails(hWnd, pbs);
    }
    return FALSE;
}

/*******************************************************************************
*
*  BatDetailDlgProc
*
*  DESCRIPTION:
*   DialogProc for the Detailed Battery Information dialog.
*
*  PARAMETERS:
*
*******************************************************************************/

LRESULT CALLBACK BatDetailDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT uiBatNum;
    static PBATTERY_STATE pbs;

    switch (uMsg) {
        case WM_INITDIALOG:
            pbs = (PBATTERY_STATE) lParam;
            return InitBatDetailDialogs(hWnd, pbs);

        case WM_COMMAND:
            switch (wParam) {
                case IDC_REFRESH:
                    GetBatStatusDetails(hWnd, pbs);
                    break;

                case IDCANCEL:
                case IDOK:
                    EndDialog(hWnd, wParam);
                    break;
            }
            break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_ContextMenuHelpIDs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_ContextMenuHelpIDs);
            break;
    }

    return FALSE;
}

