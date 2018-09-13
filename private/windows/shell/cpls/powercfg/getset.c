/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       GETSET.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Power management UI. Control panel applet. Support for set/get to/from
*   dialog controls.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>

#include "powercfg.h"

// Private functions implemented in GETSET.C
LPTSTR  IDtoStr(UINT, PUINT);
VOID    DoSetSlider(HWND, UINT, DWORD, DWORD);
VOID    DoSetText(HWND, UINT, LPTSTR, DWORD);
BOOLEAN DoComboSet(HWND, UINT, PUINT, UINT);
BOOLEAN DoComboGet(HWND, UINT, PUINT, PUINT);
BOOLEAN DoGetCheckBox(HWND, UINT, PUINT, LPDWORD);
BOOLEAN DoGetCheckBoxEnable(HWND, UINT, PUINT, LPDWORD, LPDWORD);
BOOLEAN ValidateCopyData(LPDWORD, LPDWORD, DWORD);

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/


/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  DisableControls
*
*  DESCRIPTION:
*   Disable all specified controls. Only modify those which are visable.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID DisableControls(
    HWND            hWnd,
    UINT            uiNumControls,
    POWER_CONTROLS  pc[]
)
{
    UINT    i;
    HWND    hWndControl;

    for (i = 0; i < uiNumControls; i++) {

        switch (pc[i].uiType) {
            case CHECK_BOX:
            case CHECK_BOX_ENABLE:
            case SLIDER:
            case EDIT_UINT:
            case EDIT_TEXT:
            case EDIT_TEXT_RO:
            case PUSHBUTTON:
            case STATIC_TEXT:
            case COMBO_BOX:

                if (hWndControl = GetDlgItem(hWnd, pc[i].uiID)) {
                    if (GetWindowLong(hWndControl, GWL_STYLE) & WS_VISIBLE) {
                        EnableWindow(hWndControl, FALSE);

                        // Force state to disable.
                        if (pc[i].lpdwState) {
                            *pc[i].lpdwState = CONTROL_DISABLE;
                        }
                    }
                }
                else {
                    DebugPrint( "DisableControls, GetDlgItem failed, hWnd: 0x%X, ID: 0x%X, Index: %d", hWnd, pc[i].uiID, i);
                }
                break;

            default:
                DebugPrint( "DisableControls, unknown control type");
        } // switch (ppc[i].uType)
    }
}

/*******************************************************************************
*
*  HideControls
*
*  DESCRIPTION:
*   Hide all specified controls. Used to handle error cases.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID HideControls(
    HWND            hWnd,
    UINT            uiNumControls,
    POWER_CONTROLS  pc[]
)
{
    UINT    i;
    HWND    hWndControl;

    for (i = 0; i < uiNumControls; i++) {

        switch (pc[i].uiType) {
            case CHECK_BOX:
            case CHECK_BOX_ENABLE:
            case SLIDER:
            case EDIT_UINT:
            case EDIT_TEXT:
            case EDIT_TEXT_RO:
            case PUSHBUTTON:
            case STATIC_TEXT:
            case COMBO_BOX:
                // Force state to hide.
                if (pc[i].lpdwState) {
                    *pc[i].lpdwState = CONTROL_HIDE;
                }

                if (hWndControl = GetDlgItem(hWnd, pc[i].uiID)) {
                    ShowWindow(hWndControl, SW_HIDE);
                }
                else {
                    DebugPrint( "HideControls, GetDlgItem failed, hWnd: 0x%X, ID: 0x%X, Index: %d", hWnd, pc[i].uiID, i);
                }
                break;

            default:
                DebugPrint( "HideControls, unknown control type");
        } // switch (ppc[i].uType)
    }
}

/*******************************************************************************
*
*  SetControls
*
*  DESCRIPTION:
*   Set dialogbox controls using control description array. Walks an array of
*   POWER_CONTROLS dialog controls specifiers and sets the state of the controls
*   (specified by the uiID member) to match the data value pointed to by
*   the lpdwParam member. This function is typically called during
*   WM_INITDIALOG or after a data value has changed and the UI needs to be
*   updated. The optional lpdwState member points to a state variable which
*   controls the visible/hidden status of the control.
*
*  PARAMETERS:
*   hWnd            - Dialog window handle.
*   uiNumControls   - Number of controls to set.
*   pc[]            - Aarray of POWER_CONTROLS dialog controls specifiers.
*
*******************************************************************************/

BOOL SetControls(
    HWND            hWnd,
    UINT            uiNumControls,
    POWER_CONTROLS  pc[]
)
{
    UINT    i, uiMaxMin, uiData, uiMsg;
    PUINT   pui;
    LPTSTR  lpsz;
    UINT    uiIndex;
    HWND    hWndControl;

    for (i = 0; i < uiNumControls; i++) {

        // Validate/Copy input data.
        switch (pc[i].uiType) {
            case CHECK_BOX:
            case CHECK_BOX_ENABLE:
            case SLIDER:
            case EDIT_UINT:
                if (!pc[i].lpvData) {
                    DebugPrint( "SetControls, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                if (!ValidateCopyData(&uiData, pc[i].lpvData, pc[i].dwSize)) {
                    DebugPrint( "SetControls, validate/copy failed, index: %d", i);
                    return FALSE;
                }
                break;

            case EDIT_TEXT:
            case EDIT_TEXT_RO:
                if (!pc[i].lpvData) {
                    DebugPrint( "SetControls, edit text, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                break;

            case COMBO_BOX:
                if (!pc[i].lpvData) {
                    DebugPrint( "SetControls, combo box, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                if (!pc[i].lpdwParam) {
                    DebugPrint( "SetControls, combo box, NULL lpdwParam, index: %d", i);
                    return FALSE;
                }
                if (!ValidateCopyData(&uiData, pc[i].lpdwParam, pc[i].dwSize)) {
                    DebugPrint( "SetControls, combo box, validate/copy failed, index: %d", i);
                    return FALSE;
                }
                break;
        }

        switch (pc[i].uiType) {
            case CHECK_BOX:
                // lpdwParam points to an optional flag mask.
                if (pc[i].lpdwParam) {
                    uiData &= *pc[i].lpdwParam;
                }
                CheckDlgButton(hWnd, pc[i].uiID, uiData);
                break;

            case CHECK_BOX_ENABLE:
                // lpdwParam points to an optional flag mask.
                if (pc[i].lpdwParam) {
                    uiData &= *pc[i].lpdwParam;
                }
                if (pc[i].lpdwState) {
                    if (uiData) {
                        *pc[i].lpdwState |= CONTROL_ENABLE;
                    }
                    else {
                        *pc[i].lpdwState &= ~CONTROL_ENABLE;
                    }
                }
                CheckDlgButton(hWnd, pc[i].uiID, uiData);
                break;

            case SLIDER:
                // lpdwParam points to scale MaxMin initialization.
                if (!pc[i].lpdwParam) {
                    DebugPrint( "SetControls, NULL slider scale pointer, index: %d", i);
                    return FALSE;
                }
                DoSetSlider(hWnd, pc[i].uiID, uiData, *pc[i].lpdwParam);
                break;

            case EDIT_UINT:
                if (pc[i].lpdwParam) {
                    PSPIN_DATA psd = (PSPIN_DATA)pc[i].lpdwParam;

                    SendDlgItemMessage(hWnd, psd->uiSpinId, UDM_SETRANGE, 0,
                                       (LPARAM) *(psd->puiRange));
                }
                SetDlgItemInt(hWnd, pc[i].uiID, uiData, FALSE);
                break;


            case EDIT_TEXT:
            case EDIT_TEXT_RO:
                DoSetText(hWnd, pc[i].uiID, pc[i].lpvData, pc[i].dwSize);
                break;


            case COMBO_BOX:
                if (!DoComboSet(hWnd, pc[i].uiID, pc[i].lpvData, uiData)) {
                    DebugPrint( "SetControls, DoComboSet failed, control: %d", i);
                }
                break;

            case PUSHBUTTON:
            case STATIC_TEXT:
                // Just set visable/enable for this one.
                break;

            default:
                DebugPrint( "SetControls, unknown control type");
                return FALSE;
        } // switch (ppc[i].uType)

        // Control type CHECK_BOX_ENABLE is always visible and enabled.
        if ((pc[i].uiType != CHECK_BOX_ENABLE) && pc[i].lpdwState) {
            if (hWndControl = GetDlgItem(hWnd, pc[i].uiID)) {
                ShowWindow(hWndControl, (*pc[i].lpdwState & CONTROL_HIDE) ?  SW_HIDE:SW_SHOW);
                EnableWindow(hWndControl, (*pc[i].lpdwState & CONTROL_ENABLE) ? TRUE:FALSE);
            }
            else {
                DebugPrint( "SetControls, GetDlgItem failed, hWnd: 0x%X, ID: 0x%X, Index: %d", hWnd, pc[i].uiID, i);
            }
        }
    }
    return TRUE;
}

/*******************************************************************************
*
*  GetControls
*
*  DESCRIPTION:
*   Get dialogbox controls values using control description array. Walks an
*   array of POWER_CONTROLS dialog controls specifiers and gets the state of the
*   controls (specified by the uiID member). The control states are placed in
*   the data values pointed to by the lpdwParam members of the description array.
*   This function is typically called during WM_COMMAND processing when the
*   state of a control has changed and the cooresponding data value needs to be
*   updated. The optional lpdwState member points to a state variable which
*   controls the visible/hidden status of the control. These state values will
*   be updated by CHECK_BOX_ENABLE controls.
*
*  PARAMETERS:
*   hWnd            - Dialog window handle.
*   uiNumControls   - Number of controls to set.
*   pc[]            - Aarray of POWER_CONTROLS dialog controls specifiers.
*
*******************************************************************************/

BOOL GetControls(
    HWND            hWnd,
    UINT            uiNumControls,
    POWER_CONTROLS  pc[]
)
{
    UINT    i, uiIndex, uiDataOut, uiMsg, uiDataIn = 0;
    BOOL    bSuccess;

    for (i = 0; i < uiNumControls; i++) {

        // Validate output data pointers.
        switch (pc[i].uiType) {
            case CHECK_BOX:
            case CHECK_BOX_ENABLE:
            case SLIDER:
            case EDIT_UINT:
                if (!pc[i].lpvData) {
                    DebugPrint( "GetControls, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                if (!ValidateCopyData(&uiDataIn, pc[i].lpvData, pc[i].dwSize)) {
                    DebugPrint( "GetControls, validate/copy input data failed, index: %d", i);
                    return FALSE;
                }
                uiDataOut = uiDataIn;
                break;

            case COMBO_BOX:
                if (!pc[i].lpvData) {
                    DebugPrint( "GetControls, combo box, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                if (!pc[i].lpdwParam) {
                    DebugPrint( "SetControls, COMBO_BOX, NULL lpdwParam, index: %d", i);
                    return FALSE;
                }
                if (!ValidateCopyData(&uiDataIn, pc[i].lpdwParam, pc[i].dwSize)) {
                    DebugPrint( "GetControls, combo box, validate/copy input data failed, index: %d", i);
                    return FALSE;
                }
                uiDataOut = uiDataIn;
                break;

            case EDIT_TEXT:
                if (!pc[i].lpvData) {
                    DebugPrint( "GetControls, edit text, NULL lpvData, index: %d", i);
                    return FALSE;
                }
                break;
        }

        switch (pc[i].uiType) {
            case CHECK_BOX:
                // lpdwParam points to a flag mask.
                DoGetCheckBox(hWnd, pc[i].uiID, &uiDataOut, pc[i].lpdwParam);
                break;

            case CHECK_BOX_ENABLE:
                // lpdwParam points to a flag mask.
                DoGetCheckBoxEnable(hWnd, pc[i].uiID, &uiDataOut,
                                    pc[i].lpdwParam, pc[i].lpdwState);
                break;

            case EDIT_UINT:
                uiDataOut = GetDlgItemInt(hWnd, pc[i].uiID, &bSuccess, FALSE);
                if (!bSuccess) {
                    DebugPrint( "GetControls, GetDlgItemInt failed, index: %d", i);
                    return FALSE;
                }
                break;

            case EDIT_TEXT:
                GetDlgItemText(hWnd, pc[i].uiID, pc[i].lpvData,
                               (pc[i].dwSize / sizeof(TCHAR)) - 1);
                break;

            case SLIDER:
                uiDataOut = (UINT) SendDlgItemMessage(hWnd, pc[i].uiID, TBM_GETPOS, 0, 0);
                break;

            case COMBO_BOX:
                if (!DoComboGet(hWnd, pc[i].uiID, pc[i].lpvData, &uiDataOut)) {
                    DebugPrint( "GetControls, DoComboGet failed, control: %d", i);
                }
                break;

            case EDIT_TEXT_RO:
            case PUSHBUTTON:
            case STATIC_TEXT:
                // Do nothing, these control types can only be set.
                break;

            default:
                DebugPrint( "GetControls, unknown control type");
                return FALSE;
        } // switch (pc[i].uType)

        // Copy output data.
        switch (pc[i].uiType) {
            case CHECK_BOX:
            case CHECK_BOX_ENABLE:
            case SLIDER:
            case EDIT_UINT:
                if (!ValidateCopyData(pc[i].lpvData, &uiDataOut, pc[i].dwSize)) {
                    DebugPrint( "GetControls, validate/copy output data failed, index: %d", i);
                    return FALSE;
                }
                break;
            case COMBO_BOX:
                if (!ValidateCopyData(pc[i].lpdwParam, &uiDataOut, pc[i].dwSize)) {
                    DebugPrint( "GetControls, combo box, validate/copy output data failed, index: %d", i);
                    return FALSE;
                }
        }
    }
    return TRUE;
}

/*******************************************************************************
*
*  RangeLimitIDarray
*
*  DESCRIPTION:
*   Limit the range of values in a ID array based on the passed in Min and Max
*   values.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID RangeLimitIDarray(UINT uiID[], UINT uiMin, UINT uiMax)
{
    UINT i, j;

    // Find the min value and slide the other entries down so that
    // it is the first entry in the array.
    if (uiMin != (UINT) -1) {
        i = 0;
        while (uiID[i++]) {
            if (uiID[i++] >= uiMin) {
                break;
            }
        }
        if (i > 1) {
            i -= 2;
            j = 0;
            while (uiID[i]) {
                uiID[j++] = uiID[i++];
                uiID[j++] = uiID[i++];
            }
        }
        uiID[j++] = 0; uiID[j] = 0;
    }

    // Find the max value and terminate the array so that it's the last entry.
    if (uiMax != (UINT) -1) {
        i = 0;
        while (uiID[i++]) {
            // Don't mess with timeouts of value zero ("Never").
            if (uiID[i] == 0) {
                break;
            }
            if (uiID[i++] > uiMax) {
                uiID[--i] = 0; uiID[--i] = 0;
                break;
            }
        }
    }
}

/*******************************************************************************
*
*                 P R I V A T E   F U N C T I O N S
*
*******************************************************************************/

/*******************************************************************************
*
*  IDtoStr
*
*  DESCRIPTION:
*   Builds a string from an array of string resource ID's.
*
*  PARAMETERS:
*
*******************************************************************************/

LPTSTR IDtoStr(
    UINT uiCount,
    UINT uiStrID[]
)
{
    TCHAR   szTmp[MAX_UI_STR_LEN];
    LPTSTR  lpsz = NULL;

    szTmp[0] = '\0';
    while (uiCount) {
        lpsz = LoadDynamicString(uiStrID[--uiCount]);
        if (lpsz) {
            if ((lstrlen(lpsz) + lstrlen(szTmp)) < (MAX_UI_STR_LEN - 3)) {
                if (szTmp[0]) {
                    lstrcat(szTmp, TEXT(", "));
                }
                lstrcat(szTmp, lpsz);
            }
            LocalFree(lpsz);
            lpsz = LocalAlloc(0, STRSIZE(szTmp));
            if (lpsz) {
                lstrcpy(lpsz, szTmp);
            }
        }
    }
    return lpsz;
}

/*******************************************************************************
*
*  DoSetSlider
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID DoSetSlider(HWND hWnd, UINT uiID, DWORD dwData, DWORD dwMaxMin)
{
    if (dwMaxMin) {
        SendDlgItemMessage(hWnd, uiID, TBM_SETRANGE, FALSE, (LPARAM) dwMaxMin);
    }

    SendDlgItemMessage(hWnd, uiID, TBM_SETPOS, TRUE, (LPARAM) dwData);
}

/*******************************************************************************
*
*  DoSetText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID DoSetText(HWND hWnd, UINT uiID, LPTSTR lpsz, DWORD dwSize)
{
    if (dwSize) {
        SendDlgItemMessage(hWnd, uiID, EM_SETLIMITTEXT,
        dwSize - sizeof(TCHAR), 0);
    }
    SetDlgItemText(hWnd, uiID, lpsz);
}

/*******************************************************************************
*
*  DoComboSet
*
*  DESCRIPTION:
*   Reset and populate a combo box with the data pointed to by uiId. Select the
*   item pointed to by uiData.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoComboSet(
    HWND    hWnd,
    UINT    uiID,
    UINT    uiId[],
    UINT    uiData
)
{
    UINT    uiSelIndex, uiIndex = 0, i = 0;
    BOOL    bFoundSel = FALSE;
    LPTSTR  lpsz;

    SendDlgItemMessage(hWnd, uiID, CB_RESETCONTENT, 0, 0);

    // Populate the combo list box.
    while (uiId[i]) {
        lpsz = LoadDynamicString(uiId[i++]);
        if (lpsz) {
            if (uiIndex != (UINT) SendDlgItemMessage(hWnd, uiID, CB_ADDSTRING,
                                                     0, (LPARAM)lpsz)) {
                DebugPrint( "DoComboSet, CB_ADDSTRING failed: %s", lpsz);
                LocalFree(lpsz);
                return FALSE;
            }
            LocalFree(lpsz);

            if (uiId[i] == uiData) {
                bFoundSel = TRUE;
                uiSelIndex = uiIndex;
            }
            if (SendDlgItemMessage(hWnd, uiID, CB_SETITEMDATA,
                                   uiIndex++, (LPARAM)uiId[i++]) == CB_ERR) {
                DebugPrint( "DoComboSet, CB_SETITEMDATA failed, index: %d", --uiIndex);
                return FALSE;
            }
        }
        else {
            DebugPrint( "DoComboSet, unable to load string, index: %d", --i);
            return FALSE;
        }
    }

    if (bFoundSel) {
        if ((UINT)SendDlgItemMessage(hWnd, uiID, CB_SETCURSEL,
                                     (WPARAM)uiSelIndex, 0) != uiSelIndex) {
            DebugPrint( "DoComboSet, CB_SETCURSEL failed, index: %d", uiSelIndex);
            return FALSE;
        }
    }
    else {
        DebugPrint( "DoComboSet unable to find data: 0x%X", uiData);
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
*
*  DoComboGet
*
*  DESCRIPTION:
*   Get data for currently selected combo box item.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoComboGet(
    HWND    hWnd,
    UINT    uiID,
    UINT    uiId[],
    PUINT   puiData
)
{
    UINT    uiIndex, uiData;

    uiIndex = (UINT) SendDlgItemMessage(hWnd, uiID, CB_GETCURSEL,0, 0);
    if (uiIndex == CB_ERR) {
         DebugPrint( "DoComboGet, CB_GETCURSEL failed");
         return FALSE;
    }
    uiData = (UINT) SendDlgItemMessage(hWnd, uiID, CB_GETITEMDATA, uiIndex, 0);
    if (uiData == CB_ERR) {
         DebugPrint( "DoComboGet, CB_GETITEMDATA failed, index: %d", uiIndex);
         return FALSE;
    }
    *puiData = uiData;
    return TRUE;
}

/*******************************************************************************
*
*  DoGetCheckBox
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoGetCheckBox(
    HWND    hWnd,
    UINT    uiID,
    PUINT   puiData,
    LPDWORD lpdwMask
)
{
    UINT    uiButtonState;
    BOOLEAN bRet;

    uiButtonState = IsDlgButtonChecked(hWnd, uiID);
    if (lpdwMask) {
        if (uiButtonState == BST_CHECKED) {
            bRet = TRUE;
            *puiData |= *lpdwMask;
        }
        else {
            bRet = FALSE;
            *puiData &= ~(*lpdwMask);
        }
    }
    else {
        if (uiButtonState == BST_CHECKED) {
            bRet = *puiData = TRUE;
        }
        else {
            bRet = *puiData = FALSE;
        }
    }
    return bRet;
}

/*******************************************************************************
*
*  DoGetCheckBoxEnable
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN DoGetCheckBoxEnable(
    HWND    hWnd,
    UINT    uiID,
    PUINT   puiData,
    LPDWORD lpdwMask,
    LPDWORD lpdwEnableState
)
{
    UINT    uiData;

    if (DoGetCheckBox(hWnd, uiID, puiData, lpdwMask)) {
        if (lpdwEnableState) {
            *lpdwEnableState |= CONTROL_ENABLE;
        }
        return TRUE;
    }
    else {
        if (lpdwEnableState) {
            *lpdwEnableState &= ~CONTROL_ENABLE;
        }
        return FALSE;
    }
}

/*******************************************************************************
*
*  ValidateCopyData
*
*  DESCRIPTION:
*   Data size must be BYTE, SHORT or DWORD.
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN ValidateCopyData(LPDWORD lpDst, LPDWORD lpSrc, DWORD dwSize)
{
    switch (dwSize) {
        case sizeof(BYTE):
        case sizeof(SHORT):
        case sizeof(DWORD):
            *lpDst = 0;
            memcpy(lpDst, lpSrc, dwSize);
            return TRUE;

        default:
            DebugPrint( "ValidateCopyData, invalid variable size: %d", dwSize);
    }
    return FALSE;
}
