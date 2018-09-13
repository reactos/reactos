/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    devres1.c

Abstract:

    Routines for displaying resource dialogs.

Author:

    Paula Tomlinson (paulat) 7-Feb-1996

Revision History:

    Jamie Hunter (jamiehun) 19-Mar-1998
        Removed EditResource Dialog Proceedures into this file
        Resource picking functionality improved


--*/

#include "precomp.h"
#pragma hdrstop

#define Nearness(x,y) (((x)>(y))?(x)-(y):(y)-(x))

static UDACCEL udAccel[] = {{0,1},{1,16},{2,256},{3,4096},{4,16000}};

static const DWORD EditResHelpIDs[]=
{
    IDC_EDITRES_INSTRUCTIONS,   IDH_NOHELP,
    IDC_EDITRES_MFCHILDREN,     IDH_NOHELP,
    IDC_EDITRES_VALUE_LABEL,    IDH_DEVMGR_RESOURCES_EDIT_VALUE,
    IDC_EDITRES_VALUE,          IDH_DEVMGR_RESOURCES_EDIT_VALUE,
    IDC_EDITRES_CONFLICTINFO,   IDH_DEVMGR_RESOURCES_EDIT_INFO,
    IDC_EDITRES_CONFLICTTEXT,   IDH_DEVMGR_RESOURCES_EDIT_INFO,
    IDC_EDITRES_CONFLICTLIST,   IDH_DEVMGR_RESOURCES_EDIT_INFO,
    0, 0
};


void
InitEditResDlg(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei,
    ULONG               ulVal,
    ULONG               ulLen
    );

void
ClearEditResConflictList(
    HWND    hDlg,
    DWORD   dwFlags
    );

void
UpdateEditResConflictList(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei,
    ULONG               ulVal,
    ULONG               ulLen,
    ULONG               ulFlags
    );

BOOL
LocateClosestValue(
    IN LPBYTE      pData,
    IN RESOURCEID  ResType,
    IN ULONG       TestValue,
    IN ULONG       TestLen,
    IN INT         Mode,
    OUT PULONG     OutValue, OPTIONAL
    OUT PULONG     OutLen, OPTIONAL
    OUT PULONG     OutIndex OPTIONAL
    );

void
GetOtherValues(
    IN     LPBYTE      pData,
    IN     RESOURCEID  ResType,
    IN     LONG        Increment,
    OUT    PULONG      pulValue,
    OUT    PULONG      pulLen,
    OUT    PULONG      pulEnd
    );

void
UpdateEditResConflictList(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei,
    ULONG               ulVal,
    ULONG               ulLen,
    ULONG               ulFlags
    );

BOOL
bValidateResourceVal(
    HWND                hDlg,
    PULONG              pulVal,
    PULONG              pulLen,
    PULONG              pulEnd,
    PULONG              pulIndex,
    PRESOURCEEDITINFO   lprei
    );

BOOL
bConflictWarn(
    HWND                hDlg,
    ULONG               ulVal,
    ULONG               ulLen,
    ULONG               ulEnd,
    PRESOURCEEDITINFO   lprei
    );

void
ClearEditResConflictList(
    HWND    hDlg,
    DWORD   dwFlags
    );

void
UpdateMFChildList(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei
    );

//---------------------------------------------------------------------------
// Edit Resource Dialog Box
//---------------------------------------------------------------------------



INT_PTR
WINAPI
EditResourceDlgProc(
    HWND    hDlg,
    UINT    wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR   szBuffer[MAX_PATH];
    //
    // BUGBUG!!! (jamiehun) this should not exist as static!!!
    static  ULONG   ulEditedValue, ulEditedLen, ulEditedEnd;


    switch (wMsg) {

        case WM_INITDIALOG: {

            PRESOURCEEDITINFO lprei = (PRESOURCEEDITINFO)lParam;
            ULONG             ulSize = 0;

            SetWindowLongPtr(hDlg, DWLP_USER, lParam);  // save for later msgs

            lprei->dwFlags &= ~REI_FLAGS_CONFLICT;   // no conflict yet
            lprei->dwFlags |= REI_FLAG_NONUSEREDIT; // no manual edits yet

            ulEditedValue = lprei->ulCurrentVal;
            ulEditedLen = lprei->ulCurrentLen;
            ulEditedEnd = lprei->ulCurrentEnd;

            InitEditResDlg(hDlg, lprei, ulEditedValue, ulEditedLen);

            SetFocus(GetDlgItem(hDlg, IDC_EDITRES_VALUE));
            break;  // return default (FALSE) to indicate we've set focus
        }

        case WM_NOTIFY: {

            PRESOURCEEDITINFO lprei = (PRESOURCEEDITINFO)GetWindowLongPtr(hDlg, DWLP_USER);
            LPNM_UPDOWN lpnm = (LPNM_UPDOWN)lParam;

            switch (lpnm->hdr.code) {

                case UDN_DELTAPOS:
                    if (lpnm->hdr.idFrom == IDC_EDITRES_SPIN) {

                        if (lpnm->iDelta > 0) {
                            GetOtherValues(lprei->pData, lprei->ridResType, +1,
                                           &ulEditedValue,
                                           &ulEditedLen,
                                           &ulEditedEnd);
                        } else {
                            GetOtherValues(lprei->pData, lprei->ridResType, -1,
                                           &ulEditedValue,
                                           &ulEditedLen,
                                           &ulEditedEnd);
                        }

                        pFormatResString(szBuffer, ulEditedValue, ulEditedLen,
                                        lprei->ridResType);

                        lprei->dwFlags |= REI_FLAG_NONUSEREDIT;
                        SetDlgItemText(hDlg, IDC_EDITRES_VALUE, szBuffer);
                        UpdateEditResConflictList(hDlg, lprei,
                                                  ulEditedValue,
                                                  ulEditedLen,
                                                  lprei->ulCurrentFlags);
                }
                break;
            }
            break;
        }

        case WM_COMMAND: {

            switch(LOWORD(wParam)) {

                case IDOK: {

                    PRESOURCEEDITINFO  lprei = (PRESOURCEEDITINFO) GetWindowLongPtr(hDlg, DWLP_USER);
                    ULONG ulIndex;

                    //
                    // Validate the values (could have been manually edited)
                    //
                    if (bValidateResourceVal(hDlg, &ulEditedValue, &ulEditedLen,
                                             &ulEditedEnd, &ulIndex, lprei)) {
                        //
                        // Warn if there is a conflict.  If use accepts conflict
                        // end the dialog, otherwise update the
                        // edit control since it may have been changed by the
                        // Validate call.
                        //
                        //No HMACHINE
                        if(bConflictWarn(hDlg, ulEditedValue, ulEditedLen,
                                         ulEditedEnd, lprei)) {

                            lprei->ulCurrentVal = ulEditedValue;
                            lprei->ulCurrentLen = ulEditedLen;
                            lprei->ulCurrentEnd = ulEditedEnd;
                            lprei->ulRangeCount = ulIndex;
                            EndDialog(hDlg, IDOK);

                            if (lprei->pData) {
                                MyFree(lprei->pData);
                            }

                        } else {
                            //
                            // Format and display the data
                            //
                            pFormatResString(szBuffer, ulEditedValue, ulEditedLen, lprei->ridResType);
                            SetDlgItemText(hDlg, IDC_EDITRES_VALUE, szBuffer);
                            //
                            // Update the Conflict List.
                            //
                            UpdateEditResConflictList(hDlg, lprei, ulEditedValue, ulEditedLen, lprei->ulCurrentFlags);
                        }

                    }
                    return TRUE;
                }

                case IDCANCEL: {

                    PRESOURCEEDITINFO lprei = (PRESOURCEEDITINFO)GetWindowLongPtr(hDlg, DWLP_USER);

                    if (lprei->pData) {
                        MyFree(lprei->pData);
                    }

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                }

                case IDC_EDITRES_VALUE: {
                    switch (HIWORD(wParam)) {
                        case EN_CHANGE: {

                            PRESOURCEEDITINFO lprei = (PRESOURCEEDITINFO)GetWindowLongPtr(hDlg, DWLP_USER);

                            // If Non user edit, then clear the flag, else
                            // clear the conflict list, since we are unsure
                            // of what the user has entered at this time

                            if (lprei->dwFlags & REI_FLAG_NONUSEREDIT) {
                                lprei->dwFlags &= ~REI_FLAG_NONUSEREDIT;
                            } else {
                                ClearEditResConflictList(hDlg, CEF_UNKNOWN);
                            }
                            break;
                        }

                        // If the edit control looses focus, then we should
                        // validte the contents
                        case EN_KILLFOCUS: {
                        }
                        break;
                    }
                    break;
                }
            }
            break;
        }

        case WM_HELP:      // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, DEVRES_HELP, HELP_WM_HELP, (ULONG_PTR)EditResHelpIDs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, DEVRES_HELP, HELP_CONTEXTMENU, (ULONG_PTR)EditResHelpIDs);
            break;
   }
   return FALSE;

} // EditResourceDlgProc




void
InitEditResDlg(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei,
    ULONG               ulVal,
    ULONG               ulLen
    )
{
    TCHAR       szBuffer[MAX_PATH], szInstr[MAX_PATH], szTemp[MAX_PATH],
                szResType[MAX_PATH], szResTypeLC[MAX_PATH];
    ULONG       ulSize = 0;


    //
    // Set the initial Value
    //
    pFormatResString(szBuffer, ulVal, ulLen, lprei->ridResType);
    SetDlgItemText(hDlg, IDC_EDITRES_VALUE, szBuffer);

    //
    // Setup the Spinner
    //
    SendDlgItemMessage(hDlg, IDC_EDITRES_SPIN, UDM_SETRANGE, 0, MAKELONG(MAX_SPINRANGE, 0));
    SendDlgItemMessage(hDlg, IDC_EDITRES_SPIN, UDM_SETPOS, 0, MAKELONG(0,0));
    SendDlgItemMessage(hDlg, IDC_EDITRES_SPIN, UDM_SETACCEL, 5, (LPARAM)(LPUDACCEL)udAccel);

    //
    // Limit the Edit Text.
    //
    switch (lprei->ridResType) {

        case ResType_Mem:
            LoadString(MyDllModuleHandle, IDS_MEMORY_FULL, szResType, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_MEMORY_FULL_LC, szResTypeLC, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_RANGEINSTR1, szInstr, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_RANGEINSTR2, szTemp, MAX_PATH);
            lstrcat(szInstr, szTemp);

            //
            // Limit the Input field to Start Val (8) + End Val(8) + seperator (4)
            //
            SendDlgItemMessage(hDlg, IDC_EDITRES_VALUE, EM_LIMITTEXT, 20, 0l);
            break;

        case ResType_IO:
            LoadString(MyDllModuleHandle, IDS_IO_FULL, szResType, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_RANGEINSTR1, szInstr, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_RANGEINSTR2, szTemp, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_IO_FULL_LC, szResTypeLC, MAX_PATH);
            lstrcat(szInstr, szTemp);

            //
            // Limit the Input field to Start Val (4) + End Val(4) + seperator (4)
            //
            SendDlgItemMessage(hDlg, IDC_EDITRES_VALUE, EM_LIMITTEXT, 12, 0l);
            break;

        case ResType_DMA:
            LoadString(MyDllModuleHandle, IDS_DMA_FULL, szResType, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_SINGLEINSTR1, szInstr, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_SINGLEINSTR2, szTemp, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_DMA_FULL_LC, szResTypeLC, MAX_PATH);
            lstrcat(szInstr, szTemp);

            //
            // Limit the Input field to Val (2)
            //
            SendDlgItemMessage(hDlg, IDC_EDITRES_VALUE, EM_LIMITTEXT, 2, 0l);
            break;

        case ResType_IRQ:
            LoadString(MyDllModuleHandle, IDS_IRQ_FULL, szResType, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_SINGLEINSTR1, szInstr, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_EDITRES_SINGLEINSTR2, szTemp, MAX_PATH);
            LoadString(MyDllModuleHandle, IDS_IRQ_FULL_LC, szResTypeLC, MAX_PATH);
            lstrcat(szInstr, szTemp);

            //
            // Limit the Input field to Val (2)
            //
            SendDlgItemMessage(hDlg, IDC_EDITRES_VALUE, EM_LIMITTEXT, 2, 0l);
            break;
    }

    //
    // Set the Instruction Text
    //
    wsprintf(szBuffer, szInstr, szResTypeLC);
    SetDlgItemText(hDlg, IDC_EDITRES_INSTRUCTIONS, szBuffer);

    //
    // Set the Dialog Title
    //
    LoadString(MyDllModuleHandle, IDS_EDITRES_TITLE, szTemp, MAX_PATH);
    wsprintf(szBuffer, szTemp, szResType);
    SetWindowText(hDlg, szBuffer);

    //
    // If this is a MF parent device, then show which children own this resource.
    //
    UpdateMFChildList(hDlg, lprei);

    //
    // Read the res des data and store a ptr to it so we
    // don't have to refetch it multiple times.
    //
    lprei->pData = NULL;
    if (CM_Get_Res_Des_Data_Size_Ex(&ulSize, lprei->ResDes, 0,lprei->hMachine) == CR_SUCCESS) {
        lprei->pData = MyMalloc(ulSize);
        if (lprei->pData != NULL) {
            CM_Get_Res_Des_Data_Ex(lprei->ResDes, lprei->pData, ulSize, 0,lprei->hMachine);
        }
    }

    //
    // Update the Conflict List.
    //
    UpdateEditResConflictList(hDlg, lprei, ulVal, ulLen, lprei->ulCurrentFlags);


} // InitEditResDlg



BOOL
LocateClosestValue(
    IN LPBYTE      pData,
    IN RESOURCEID  ResType,
    IN ULONG       TestValue,
    IN ULONG       TestLen,
    IN INT         Mode,
    OUT PULONG     OutValue, OPTIONAL
    OUT PULONG     OutLen, OPTIONAL
    OUT PULONG     OutIndex OPTIONAL
    )
/*++

Routine Description:

    This routine finds the nearest valid address/range
    to that the user specified
    if Mode == 0, the nearest value is used
    if Mode > 0, the nearest higher value is used
    if Mode < 0, the nearest lower value is used

Arguments:

    pData - information about the resources being selected
    ResType - type of resource being selected
    CurrentValue - value entered by user
    CurrentLen - length based on range entered by user
    Mode - search mode, -1 = previous, 1 = next, 0 = nearest
    OutValue - nearest valid value
    OutLen - length associated with nearest valid value

Return Value:

    If exact match found, return TRUE
    otherwise FALSE

--*/
{
    PGENERIC_RESOURCE   pGenRes = (PGENERIC_RESOURCE)pData;
    ULONG Start, Len, End, Flags , Align;
    ULONG BestVal;
    ULONG BestValL;
    ULONG BestValU;
    ULONG FoundVal = 0;
    ULONG FoundLen = 0;
    ULONG FoundIndex = 0;
    ULONG Index;
    BOOL FindNearest = TRUE; // indicates we should find nearest

    //
    // precedence (1) Value&Len match exactly
    // precedence (2) closest valid value
    //

    //
    // cover a catch-all case - start of the very first resource range
    //
    pGetRangeValues(pData, ResType, 0, &Start, &Len, &End, &Align, &Flags);
    //
    // we have at least 1 found value
    //
    FoundVal = Start;
    FoundLen = Len;

    //
    // Find a nearby valid range to the one supplied
    //

    //
    // check each range at a time
    // sometimes ranges may not be given in ascending order
    // eg, first range is a preferred, second range is alternative
    //
    for (Index = 0; Index < pGenRes->GENERIC_Header.GENERIC_Count; Index++) {

        //
        // get limits for this range
        //
        pGetRangeValues(pData, ResType, Index, &Start, &Len, &End, &Align, &Flags);

        //
        // first, try to find a value that is GOOD, that is <= TestValue
        //

        BestValL = TestValue;
        if (pAlignValues(&BestValL, Start, Len, End, Align, -1) == FALSE) {
            //
            // if it failed, use the lowest value in this range (ie Start)
            //
            BestValL = Start;
        }

        //
        // find an upper value that is aligned
        //
        if (BestValL == TestValue) {
            //
            // if match was exact, skip test
            //
            BestValU = TestValue;
        } else {
            //
            // search for upper limit
            //
            BestValU = TestValue;
            if (pAlignValues(&BestValU, Start, Len, End, Align, 1) == FALSE) {
                //
                // couldn't use it - find highest valid value
                //
                BestValU = End-Len+1;
                if (pAlignValues(&BestValU, Start, Len, End, Align, -1) == FALSE) {
                    //
                    // still no go
                    //
                    BestValU = BestValL;
                }
            }
        }

        //
        // now we have found our boundaries
        // may need to modify, depending on preferences
        //

        if (Mode<0) {
            //
            // if range is < TestVal, use highest, else lowest
            //
            if (BestValU <= TestValue) {
                BestVal = BestValU;
            } else {
                BestVal = BestValL;
            }
        } else if (Mode>0) {
            //
            // if range is > TestVal, use lowest, else highest
            //
            if (BestValL >= TestValue) {
                BestVal = BestValL;
            } else {
                BestVal = BestValU;
            }
        } else {
            //
            // use closest of the two values
            //
            if (Nearness(BestValL,TestValue)<= Nearness(BestValU,TestValue)) {
                BestVal = BestValL;
            } else {
                BestVal = BestValU;
            }
        }

        //
        // we know that BestVal is valid within the range
        // and is the choice for this range
        //

        //
        // handle the match cases
        //
        if (TestValue == BestVal && TestLen == Len) {
            //
            // exact match
            //

            if (OutValue != NULL) {
                *OutValue = BestVal;
            }
            if (OutLen != NULL) {
                *OutLen = Len;
            }
            if (OutIndex != NULL) {
                *OutIndex = Index;
            }
            return TRUE;
        }

        if (FindNearest && Mode != 0) {
            //
            // we are currently in "FindNearest" mode which means
            // we haven't found one in the direction we wanted
            //
            if (Mode < 0 && BestVal <= TestValue) {
                //
                // not looking for nearness now we've found one lower
                //
                FoundVal = BestVal;
                FoundLen = Len;
                FoundIndex = Index;
                FindNearest = FALSE;
            } else if (Mode > 0 && BestVal >= TestValue) {
                //
                // not looking for nearness now we've found one higher
                //
                FoundVal = BestVal;
                FoundLen = Len;
                FoundIndex = Index;
                FindNearest = FALSE;
            }

        } else if (FindNearest ||
            (Mode < 0 && BestVal <= TestValue) ||
            (Mode > 0 && BestVal >= TestValue)) {
            if (Nearness(BestVal,TestValue) < Nearness(FoundVal,TestValue)) {
                //
                // this address is nearer
                //
                FoundVal = BestVal;
                FoundLen = Len;
                FoundIndex = Index;
            } else if (Nearness(BestVal,TestValue) == Nearness(FoundVal,TestValue)) {
                //
                // this address guess is as near as nearest guess, pick the better length
                //
                // I can't see any place that this should happen
                // but theoretically it could happen
                // so this is a safety net more than anything else
                //
                if (Nearness(Len,TestLen) < Nearness(FoundLen,TestLen)) {
                    //
                    // this length is nearer
                    //
                    FoundVal = BestVal;
                    FoundLen = Len;
                    FoundIndex = Index;
                } else if (Nearness(Len,TestLen) == Nearness(FoundLen,TestLen)) {
                    //
                    // pick the bigger (safer)
                    //
                    if (Len > FoundLen) {
                        //
                        // this length is bigger
                        //
                        FoundVal = BestVal;
                        FoundLen = Len;
                        FoundIndex = Index;
                    }
                }
            }
        }
    }

    //
    // if we get here, we didn't find an exact match
    //

    // Use our best guess
    if (OutValue != NULL) {
        *OutValue = FoundVal;
    }
    if (OutLen != NULL) {
        *OutLen = FoundLen;
    }
    if (OutIndex != NULL) {
        *OutIndex = FoundIndex;
    }
    return FALSE;

}


void
GetOtherValues(
    IN     LPBYTE      pData,
    IN     RESOURCEID  ResType,
    IN     LONG        Increment,
    IN OUT PULONG      pulValue,
    IN OUT PULONG      pulLen,
    IN OUT PULONG      pulEnd
    )
/*++

Routine Description:

    Finds the next valid value, wrapping around to beginning/end value when end of range

Arguments:

    pData - resource data
    ResType - resource type
    Increment - 1 or -1
    pulValue - pointer to old/new start that is changed
    pulLen - pointer to old/new length
    pulEnd - pointer to old/new end

Return Value:

    none

--*/
{

    ULONG TestValue = *pulValue;
    ULONG TestLen = *pulLen;
    ULONG RetValue;
    ULONG RetLen;


    if (Increment == 1) {
        TestValue++;
        LocateClosestValue(pData,ResType,TestValue,TestLen, 1 ,&RetValue,&RetLen,NULL);
        if (RetValue < TestValue) {
            //
            // wrap around, find lowest possible valid address
            //
            LocateClosestValue(pData,ResType,0,TestLen, 0 ,&RetValue,&RetLen,NULL);
        }
    } else if (Increment == -1) {
        TestValue--;
        LocateClosestValue(pData,ResType,TestValue,TestLen, -1 ,&RetValue,&RetLen,NULL);
        if (RetValue > TestValue) {
            //
            // wrap around, find highest possible valid address
            //
            LocateClosestValue(pData,ResType,(ULONG)(-1),TestLen, 0 ,&RetValue,&RetLen,NULL);
        }
    }

    *pulValue = RetValue;
    *pulLen = RetLen;
    *pulEnd = RetValue + RetLen - 1;

    return;

} // GetOtherValues


void
UpdateEditResConflictList(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei,
    ULONG               ulVal,
    ULONG               ulLen,
    ULONG               ulFlags
    )
/*++

Routine Description:

    Updates all the conflict information for the selected resource
    Should give more details than UpdateDevResConflictList

Arguments:

    hDlg - handle of this dialog to display into
    lprei - resource edit info
    ulVal - value to try
    ulLen - length to test
    ulFlags - flags part of resdes

Return Value:

    none

--*/
{
    CONFIGRET   Status = CR_SUCCESS;
    HWND        hwndConflictList = GetDlgItem(hDlg, IDC_EDITRES_CONFLICTLIST);
    ULONG       ConflictCount = 0;
    ULONG       ConflictIndex = 0;
    ULONG       ulSize = 0;
    LPBYTE      pResourceData = NULL;
    CONFLICT_LIST ConflictList = 0;
    PDEVICE_INFO_SET pDeviceInfoSet;
    CONFLICT_DETAILS ConflictDetails;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szItemFormat[MAX_PATH];
    BOOL        ReservedResource = FALSE;
    BOOL        BadResource = FALSE;

    //
    // need resource-data for determining conflict
    //
    if (MakeResourceData(&pResourceData, &ulSize,
                         lprei->ridResType,
                         ulVal,
                         ulLen,
                         ulFlags)) {

        Status = CM_Query_Resource_Conflict_List(&ConflictList,
                                                    lprei->lpdi->DevInst,
                                                    lprei->ridResType,
                                                    pResourceData,
                                                    ulSize,
                                                    0,
                                                    lprei->hMachine);

        if (Status != CR_SUCCESS) {
            //
            // error occurred
            //
            ConflictList = 0;
            ConflictCount =  0;
            BadResource = TRUE;
        } else {
            //
            // find out how many things conflicted
            //
            Status = CM_Get_Resource_Conflict_Count(ConflictList,&ConflictCount);
            if (Status != CR_SUCCESS) {
                //
                // error shouldn't occur
                //
                MYASSERT(Status == CR_SUCCESS);
                ConflictCount = 0;
                BadResource = TRUE;
            }
        }
    } else {
        MYASSERT(FALSE);
        //
        // should not fail
        //
        ConflictList = 0;
        ConflictCount =  0;
        BadResource = TRUE;
    }
    if (BadResource) {
        //
        // The resource conflict information is indeterminate
        //
        SendMessage(hwndConflictList, LB_RESETCONTENT, 0, 0L);
        lprei->dwFlags &= ~REI_FLAGS_CONFLICT;
        LoadString(MyDllModuleHandle, IDS_EDITRES_UNKNOWNCONFLICT, szBuffer, MAX_PATH);
        SetDlgItemText(hDlg, IDC_EDITRES_CONFLICTTEXT, szBuffer);
        LoadString(MyDllModuleHandle, IDS_EDITRES_UNKNOWNCONFLICTINGDEVS, szBuffer, MAX_PATH);
        SendMessage(hwndConflictList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);

    } else if (ConflictCount || ReservedResource) {

    TreatAsReserved:

        SendMessage(hwndConflictList, LB_RESETCONTENT, 0, 0L);
        lprei->dwFlags |= REI_FLAGS_CONFLICT;

        if(ReservedResource == FALSE) {
            //
            // The resource conflicts with another unknown device.
            //
            LoadString(MyDllModuleHandle, IDS_EDITRES_DEVCONFLICT, szBuffer, MAX_PATH);
            SetDlgItemText(hDlg, IDC_EDITRES_CONFLICTTEXT, szBuffer);

            for(ConflictIndex = 0; ConflictIndex < ConflictCount ; ConflictIndex++) {

                //
                // obtain details for this conflict
                //
                ZeroMemory(&ConflictDetails,sizeof(ConflictDetails));
                ConflictDetails.CD_ulSize = sizeof(ConflictDetails);
                ConflictDetails.CD_ulMask = CM_CDMASK_DEVINST | CM_CDMASK_DESCRIPTION | CM_CDMASK_FLAGS;

                Status = CM_Get_Resource_Conflict_Details(ConflictList,ConflictIndex,&ConflictDetails);
                if (Status == CR_SUCCESS) {
                    if ((ConflictDetails.CD_ulFlags & CM_CDFLAGS_RESERVED) != 0) {
                        //
                        // treat as reserved - backtrack
                        //
                        ReservedResource = TRUE;
                        goto TreatAsReserved;
                    }
                    //
                    // convert CD_dnDevInst to string information
                    //
                    lstrcpy(szBuffer,ConflictDetails.CD_szDescription);
                    if (szBuffer[0] == 0) {
                        ReservedResource = TRUE;
                        goto TreatAsReserved;
                    }

                } else {
                    MYASSERT(Status == CR_SUCCESS);
                    ReservedResource = TRUE;
                    goto TreatAsReserved;
                }

                SendMessage(hwndConflictList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);
            }
        } else {
            LoadString(MyDllModuleHandle, IDS_EDITRES_RESERVED, szBuffer, MAX_PATH);
            SetDlgItemText(hDlg, IDC_EDITRES_CONFLICTTEXT, szBuffer);
            LoadString(MyDllModuleHandle, IDS_EDITRES_RESERVEDRANGE, szBuffer, MAX_PATH);
            SendMessage(hwndConflictList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);
        }

    } else {
        //
        // The resource does not conflict with any other devices.
        //
        SendMessage(hwndConflictList, LB_RESETCONTENT, 0, 0L);
        lprei->dwFlags &= ~REI_FLAGS_CONFLICT;
        LoadString(MyDllModuleHandle, IDS_EDITRES_NOCONFLICT, szBuffer, MAX_PATH);
        SetDlgItemText(hDlg, IDC_EDITRES_CONFLICTTEXT, szBuffer);
        LoadString(MyDllModuleHandle, IDS_EDITRES_NOCONFLICTINGDEVS, szBuffer, MAX_PATH);
        SendMessage(hwndConflictList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);
    }

    if (ConflictList) {
        CM_Free_Resource_Conflict_Handle(ConflictList);
    }

    if (pResourceData != NULL) {
        MyFree(pResourceData);
    }

    return;
}


BOOL
bValidateResourceVal(
    HWND                hDlg,
    PULONG              pulVal,
    PULONG              pulLen,
    PULONG              pulEnd,
    PULONG              pulIndex,
    PRESOURCEEDITINFO   lprei
    )
{
    TCHAR    szSetting[MAX_VAL_LEN], szNewSetting[MAX_VAL_LEN];
    TCHAR    szMessage[MAX_MSG_LEN], szTemp[MAX_MSG_LEN], szTemp1[MAX_MSG_LEN];
    TCHAR    szTitle[MAX_PATH];
    ULONG    ulVal, ulEnd, ulLen;
    ULONG    ulValidVal, ulValidLen;
    ULONG    ulIndex;
    BOOL     bRet;
    BOOL     exact = TRUE;


    GetDlgItemText(hDlg, IDC_EDITRES_VALUE, szSetting, MAX_VAL_LEN);

    if (pUnFormatResString(szSetting, &ulVal, &ulEnd, lprei->ridResType)) {

        ulLen = ulEnd - ulVal + 1;

        //
        // Validate the Current Settings
        //
        // If an exact match doesn't exist
        // use a close match
        // close is based on start address
        //

        if (LocateClosestValue(lprei->pData, lprei->ridResType,
                                ulVal, ulLen,0,
                                &ulValidVal, &ulValidLen,&ulIndex) == FALSE) {
            //
            // An alternate setting was found
            // we think this might be what the user wanted
            //
            LoadString(MyDllModuleHandle, IDS_EDITRES_ENTRYERROR, szTitle, MAX_PATH);
            //
            // BUGBUG!!! (jamiehun) why this cat'ing ???
            //
            LoadString(MyDllModuleHandle, IDS_EDITRES_VALIDATEERROR1, szTemp, MAX_MSG_LEN);
            LoadString(MyDllModuleHandle, IDS_EDITRES_VALIDATEERROR2, szTemp1, MAX_MSG_LEN);
            lstrcat(szTemp, szTemp1);
            LoadString(MyDllModuleHandle, IDS_EDITRES_VALIDATEERROR3, szTemp1, MAX_MSG_LEN);
            lstrcat(szTemp, szTemp1);

            pFormatResString(szSetting, ulVal, ulLen, lprei->ridResType);
            pFormatResString(szNewSetting, ulValidVal, ulValidLen, lprei->ridResType);

            wsprintf(szMessage, szTemp, szSetting, szNewSetting);

            if (MessageBox(hDlg, szMessage, szTitle,
                           MB_YESNO | MB_TASKMODAL | MB_ICONEXCLAMATION) == IDYES) {
                //
                // Update the Edited values.
                //
                *pulVal = ulValidVal;
                *pulLen = ulValidLen;
                *pulEnd = ulValidVal + ulValidLen - 1;
                *pulIndex = ulIndex;
                bRet = TRUE;
            } else {
                bRet = FALSE;
            }

        } else {
            //
            // The specified values are valid
            //
            *pulVal = ulVal;
            *pulLen = ulLen;
            *pulEnd = ulEnd;
            *pulIndex = ulIndex;
            bRet = TRUE;
        }

    } else {

        switch (lprei->ridResType) {
            case ResType_Mem:
                LoadString(MyDllModuleHandle, IDS_ERROR_BADMEMTEXT, szMessage, MAX_MSG_LEN);
                break;
            case ResType_IO:
                LoadString(MyDllModuleHandle, IDS_ERROR_BADIOTEXT, szMessage, MAX_MSG_LEN);
                break;
            case ResType_DMA:
                LoadString(MyDllModuleHandle, IDS_ERROR_BADDMATEXT, szMessage, MAX_MSG_LEN);
                break;
            case ResType_IRQ:
                LoadString(MyDllModuleHandle, IDS_ERROR_BADIRQTEXT, szMessage, MAX_MSG_LEN);
                break;
        }

        LoadString(MyDllModuleHandle, IDS_EDITRES_ENTRYERROR, szTitle, MAX_PATH);
        MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_TASKMODAL | MB_ICONASTERISK);
        bRet = FALSE;
    }

    return bRet;

} // bValidateResoureceVal



BOOL
bConflictWarn(
    HWND                hDlg,
    ULONG               ulVal,
    ULONG               ulLen,
    ULONG               ulEnd,
    PRESOURCEEDITINFO   lprei
    )
{
    BOOL    bRet = TRUE;
    TCHAR   szMessage[MAX_MSG_LEN], szTitle[MAX_PATH];


    if (!(lprei->dwFlags & REI_FLAG_NONUSEREDIT)) {
        //
        // user edits have been made so the conflict flag may not be
        // up-to-date, check conflicts now.
        //
        UpdateEditResConflictList(hDlg, lprei, ulVal, ulLen, lprei->ulCurrentFlags);
    }

    if (lprei->dwFlags & REI_FLAGS_CONFLICT) {

        LoadString(MyDllModuleHandle, IDS_EDITRES_CONFLICTWARNMSG, szMessage, MAX_MSG_LEN);
        LoadString(MyDllModuleHandle, IDS_EDITRES_CONFLICTWARNTITLE, szTitle, MAX_PATH);

        if (MessageBox(hDlg, szMessage, szTitle,
                MB_YESNO | MB_DEFBUTTON2| MB_TASKMODAL | MB_ICONEXCLAMATION) == IDNO) {
            bRet = FALSE;
        } else {
            bRet = TRUE;                // User approved conflict
        }
    }

    return bRet;

} // bConflictWarn



void
ClearEditResConflictList(
    HWND    hDlg,
    DWORD   dwFlags
    )
{
    HWND    hwndConflictList = GetDlgItem(hDlg, IDC_EDITRES_CONFLICTLIST);
    TCHAR   szBuffer[MAX_PATH];

    //
    // Clear the Conflict list to start.
    //
    SendMessage(hwndConflictList, LB_RESETCONTENT, 0, 0L);

    //
    // Load and set the info text string
    //
    if (dwFlags & CEF_UNKNOWN) {
        LoadString(MyDllModuleHandle, IDS_EDITRES_UNKNOWNCONFLICT, szBuffer, MAX_PATH);
    } else {
        LoadString(MyDllModuleHandle, IDS_EDITRES_NOCONFLICT, szBuffer, MAX_PATH);
    }
    SetDlgItemText(hDlg, IDC_EDITRES_CONFLICTTEXT, szBuffer);

    //
    // Load and set the List string
    //
    if (dwFlags & CEF_UNKNOWN) {
        LoadString(MyDllModuleHandle, IDS_EDITRES_UNKNOWNCONFLICTINGDEVS, szBuffer, MAX_PATH);
    } else {
        LoadString(MyDllModuleHandle, IDS_EDITRES_NOCONFLICTINGDEVS, szBuffer, MAX_PATH);
    }
    SendMessage(hwndConflictList, LB_ADDSTRING, 0, (LPARAM)(LPSTR)szBuffer);

} // ClearEditResConflictList





void
UpdateMFChildList(
    HWND                hDlg,
    PRESOURCEEDITINFO   lprei
    )
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(lprei);

    //
    // See if this is a MF parent device.  Check for a Child0000 subkey
    //
    // NOT IMPLEMENTED, SEE WINDOWS 95 SOURCES.
    //

} // UpdateMFChildList


