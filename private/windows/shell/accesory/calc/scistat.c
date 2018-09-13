/**************************************************************************/
/*** SCICALC Scientific Calculator for Windows 3.00.12                  ***/
/*** By Kraig Brockschmidt, Microsoft Co-op, Contractor, 1988-1989      ***/
/*** (c)1989 Microsoft Corporation.  All Rights Reserved.               ***/
/***                                                                    ***/
/*** scistat.c                                                          ***/
/***                                                                    ***/
/*** Functions contained:                                               ***/
/***    SetStat--Enable/disable the stat box, show or destroy the       ***/
/***        modeless dialog box.                                        ***/
/***    StatBoxProc--procedure for the statbox.  Handles the RET, LOAD, ***/
/***        CD, and CAD buttons, and handles double-clicks.             ***/
/***    StatFunctions--routines for DATA, SUM, AVE, and deviations.     ***/
/***                                                                    ***/
/*** Functions called:                                                  ***/
/***    SetStat                                                         ***/
/***                                                                    ***/
/*** Last modification Thu  26-Jan-1990                                 ***/
/*** -by- Amit Chatterjee [amitc]  26-Jan-1990.                         ***/
/*** Following bug fix was made:                                        ***/
/***                                                                    ***/
/*** Bug # 8499.                                                        ***/
/*** While fixing numbers in the stat array in memory, instead of using ***/
/*** the following for statement:                                       ***/
/***      for (lIndex=lData; lIndex < lStatNum - 1 ; lIndex++)          ***/
/*** the fix was to use:                                                ***/
/***      for (lIndex=lData; lIndex < lStatNum ; lIndex++)              ***/
/*** This is because lStatNum has already been decremented to care of   ***/
/*** a number being deleted.                                            ***/
/*** This fix will be in build 1.59.                                    ***/
/**************************************************************************/

#include "scicalc.h"
#include "calchelp.h"
#include "unifunc.h"

#define GMEMCHUNK 96L  /* Amount of memory to allocate at a time.         */

extern HNUMOBJ  ghnoNum;
extern HWND     hStatBox, hListBox, hEdit;
extern TCHAR    szBlank[6], *rgpsz[CSTRINGS];
extern LPTSTR   gpszNum;
extern INT      nTempCom;
extern BOOL     gbRecord;

extern BOOL FireUpPopupMenu( HWND, HINSTANCE, LPARAM );

GLOBALHANDLE    hgMem, hMem;   /* Coupla global memory handles.        */
BOOL            bFocus=TRUE;
LONG            lStatNum=0,    /* Number of data.                      */
                lReAllocCount; /* Number of data before ReAlloc.       */
HNUMOBJ *       lphnoStatNum;   /* Holding place for stat data.         */


/* Initiate or destroy the Statistics Box.                                */

VOID  APIENTRY SetStat (BOOL bOnOff)
{
    static int aStatOnlyKeys[] = { IDC_AVE, IDC_B_SUM, IDC_DEV, IDC_DATA };
    int i;

    if (bOnOff)
    {
        /* Create.                                                        */
        lReAllocCount=GMEMCHUNK/sizeof(ghnoNum); /* Set up lReAllocCount.   */

        /* Start the box.                                                 */
        hStatBox=CreateDialog(hInst, MAKEINTRESOURCE(IDD_SB), NULL, StatBoxProc);

        /* Get a handle on some memory (16 bytes initially.               */
        if (!(hgMem=GlobalAlloc(GHND, 0L)))
        {
            StatError();
            SendMessage(hStatBox, WM_COMMAND, GET_WM_COMMAND_MPS(ENDBOX, 0, 0));
            return;
        }
        ShowWindow(hStatBox, SW_SHOWNORMAL);
    }
    else
    {
        int lIndex;

        if ( hStatBox )
        {
            DestroyWindow(hStatBox);

            // Free the numobj's
            lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);
            for( lIndex = 0; lIndex < lStatNum; lIndex++ )
                NumObjDestroy( &lphnoStatNum[lIndex] );
            GlobalUnlock(hgMem);
            lStatNum = 0;

            GlobalFree(hgMem);  /* Free up the memory.                        */
            hStatBox=0;         /* Nullify handle.                            */
        }
    }

    // set the active state of the Ave, Sum, s, and Dat buttons
    for ( i=0; i<ARRAYSIZE(aStatOnlyKeys); i++)
        EnableWindow( GetDlgItem(g_hwndDlg, aStatOnlyKeys[i]), bOnOff );

    return;
}



/* Windows procedure for the Dialog Statistix Box.                        */
INT_PTR FAR APIENTRY StatBoxProc (
     HWND           hStatBox,
     UINT           iMessage,
     WPARAM         wParam,
     LPARAM         lParam)
{
    static LONG lData=-1;  /* Data index in listbox.                   */
    LONG        lIndex;    /* Temp index for counting.                 */
    DWORD       dwSize;    /* Holding place for GlobalSize.            */
    static DWORD    control[] = {
        IDC_STATLIST,   CALC_SCI_STATISTICS_VALUE,
        IDC_CAD,        CALC_SCI_CAD,
        IDC_CD,         CALC_SCI_CD,
        IDC_LOAD,       CALC_SCI_LOAD,
        IDC_FOCUS,      CALC_SCI_RET,
        IDC_NTEXT,      CALC_SCI_NUMBER,
        IDC_NUMTEXT,    CALC_SCI_NUMBER,
        0,              0 };

    switch (iMessage)
    {
        case WM_HELP:
        {
            LPHELPINFO phi = (LPHELPINFO)lParam;
            HWND hwndChild = GetDlgItem(hStatBox,phi->iCtrlId);
            WinHelp( hwndChild, rgpsz[IDS_HELPFILE], HELP_WM_HELP, (ULONG_PTR)(void *)control );
            return TRUE;
        }

        case WM_CONTEXTMENU:
            WinHelp( (HWND)wParam, rgpsz[IDS_HELPFILE], HELP_CONTEXTMENU, (ULONG_PTR)(void *)control );
            return TRUE;

        case WM_CLOSE:
            SetStat(FALSE);

        case WM_DESTROY:
            lStatNum=0L; /* Reset data count.                     */
            return(TRUE);

        case WM_INITDIALOG:
            /* Get a handle to this here things listbox display.          */
            hListBox=GetDlgItem(hStatBox, IDC_STATLIST);
            return TRUE;

        case WM_COMMAND:
            /* Check for LOAD or double-click and recall number if so.    */

            if (GET_WM_COMMAND_CMD(wParam, lParam)==LBN_DBLCLK ||
                        GET_WM_COMMAND_ID(wParam, lParam)==IDC_LOAD)
            {
                /* Lock data, get pointer to it, and get index of item.   */
                lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);
                lData=(LONG)SendMessage(hListBox,LB_GETCURSEL,0,0L);

                if (lStatNum>0 && lData !=LB_ERR)
                    // SPEED: REVIEW: can we use a pointer instead of Assign?
                    NumObjAssign( &ghnoNum, lphnoStatNum[lData]);  /* Get the data.         */
                else
                    MessageBeep(0); /* Cannodo if no data nor selection.  */

                // Cancel kbd input mode
                gbRecord = FALSE;

                DisplayNum ();
                nTempCom = 32;
                GlobalUnlock(hgMem); /* Let the memory move!              */
                break;
            }

            // switch (wParam)
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_FOCUS:
                    /* Change focus back to main window.  Primarily for   */
                    /* use with the keyboard.                             */
                    SetFocus(g_hwndDlg);
                    return (TRUE);

                case IDC_CD:
                    /* Clear the selected item from the listbox.          */
                    /* Get the index and a pointer to the data.           */
                    lData=(LONG)SendMessage(hListBox,LB_GETCURSEL,0,0L);

                    /* Check for possible error conditions.               */
                    if (lData==LB_ERR || lData > lStatNum-1 || lStatNum==0)
                    {
                        MessageBeep (0);
                        break;
                    }

                    /* Fix listbox strings.                               */
                    lIndex=(LONG)SendMessage(hListBox, LB_DELETESTRING, (WORD)lData, 0L);

                    if ((--lStatNum)==0)
                        goto ClearItAll;

                    /* Place the highlight over the next one.             */
                    if (lData<lIndex || lIndex==0)
                        lIndex=lData+1;

                    SendMessage(hListBox, LB_SETCURSEL, (WORD)lIndex-1, 0L);

                    lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);

                    /* Fix numbers in memory.                             */
                    for (lIndex=lData; lIndex < lStatNum ; lIndex++)
                    {
                        NumObjAssign( &lphnoStatNum[lIndex], lphnoStatNum[lIndex+1] );
                    }

                    GlobalUnlock(hgMem);  /* Movin' again.                */

                    /* Update the number by the "n=".                     */
                    SetDlgItemInt(hStatBox, IDC_NUMTEXT, lStatNum, FALSE);

                    dwSize=(DWORD)GlobalSize(hgMem); /* Get size of memory block.*/

                    /* Unallocate memory if not needed after data removal.*/
                    /* hMem is used so we don't possibly trach hgMem.     */
                    if ((lStatNum % lReAllocCount)==0)
                        if ((hMem=GlobalReAlloc(hgMem, dwSize-GMEMCHUNK, GMEM_ZEROINIT)))
                            hgMem=hMem;
                    return(TRUE);

                case IDC_CAD:
ClearItAll:
                    /* Nuke it all!                                       */
                    SendMessage(hListBox, LB_RESETCONTENT, 0L, 0L);
                    SetDlgItemInt(hStatBox, IDC_NUMTEXT, 0, FALSE);;

                    // Free the numobj's
                    lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);
                    for( lIndex = 0; lIndex < lStatNum; lIndex++ )
                        NumObjDestroy( &lphnoStatNum[lIndex] );
                    GlobalUnlock(hgMem);

                    GlobalFree(hgMem); /* Drop the memory.                */
                    lStatNum = 0;
                    hgMem=GlobalAlloc(GHND, 0L); /* Get a CLEAN slate.    */
                    return(TRUE);
            }
    }
    return (FALSE);
}



/* Routine for functions AVE, SUM, DEV, and DATA.                         */

VOID  APIENTRY StatFunctions (WPARAM wParam)
    {
    LONG           lIndex; /* Temp index.                                 */
    DWORD          dwSize; /* Return value for GlobalSize.                */

    switch (wParam)
    {
        case IDC_DATA: /* Add current fpNum to listbox.                       */
            if ((lStatNum % lReAllocCount)==0)
            {
                /* If needed, allocate another 96 bytes.                  */

                dwSize=(DWORD)GlobalSize(hgMem);
                if (StatAlloc (1, dwSize))
                {
                    GlobalCompact((DWORD)-1L);
                    if (StatAlloc (1, dwSize))
                    {
                        StatError ();
                        return;
                    }
                }
                hgMem=hMem;
            }

            /* Add the display string to the listbox.                     */
            hListBox=GetDlgItem(hStatBox, IDC_STATLIST);

            lIndex=StatAlloc (2,0L);
            if (lIndex==LB_ERR || lIndex==LB_ERRSPACE)
            {
                GlobalCompact((DWORD)-1L);

                lIndex=StatAlloc (2,0L);
                if (lIndex==LB_ERR || lIndex==LB_ERRSPACE)
                {
                    StatError ();
                    return;
                }
            }

            /* Highlight last entered string.                             */
            SendMessage(hListBox, LB_SETCURSEL, (WORD)lIndex, 0L);

            /* Add the number and increase the "n=" value.                */
            lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);

            NumObjAssign( &lphnoStatNum[lStatNum], ghnoNum );

            SetDlgItemInt(hStatBox, IDC_NUMTEXT, ++lStatNum, FALSE);
            break;

        case IDC_AVE: /* Calculate averages and sums.                         */
        case IDC_B_SUM: {
            DECLARE_HNUMOBJ( hnoTemp );

            lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);

            /* Sum the numbers or squares, depending on bInv.             */
            NumObjAssign( &ghnoNum, HNO_ZERO );

            for (lIndex=0L; lIndex < lStatNum; lIndex++)
            {
                NumObjAssign( &hnoTemp, lphnoStatNum[lIndex] );
                if (bInv)
                {
                    DECLARE_HNUMOBJ( hno );
                    /* Get sum of squares.      */
                    NumObjAssign( &hno, hnoTemp );
                    mulrat( &hno, hnoTemp );
                    addrat( &ghnoNum, hno );
                    NumObjDestroy( &hno );
                }
                else
                {
                    /* Get sum.                          */
                    addrat( &ghnoNum, hnoTemp );
                }
            }

            if (wParam==IDC_AVE) /* Divide by lStatNum=# of items for mean.   */
            {
                DECLARE_HNUMOBJ( hno );
                if (lStatNum==0)
                {
                    DisplayError (SCERR_DIVIDEZERO);
                    break;
                }
                NumObjSetIntValue( &hno, lStatNum );
                divrat( &ghnoNum, hno );
                NumObjDestroy( &hno );
            }
            NumObjDestroy( &hnoTemp );
            /* Fall out for sums.                                         */
            break;
        }

        case IDC_DEV: { /* Calculate deviations.                                */
            DECLARE_HNUMOBJ(hnoTemp);
            DECLARE_HNUMOBJ(hnoX);
            DECLARE_HNUMOBJ( hno );

            if (lStatNum <=1) /* 1 item or less, NO deviation.            */
            {
                NumObjAssign( &ghnoNum, HNO_ZERO );
                return;
            }

            /* Get sum and sum of squares.                                */
            lphnoStatNum=(HNUMOBJ *)GlobalLock(hgMem);

            NumObjAssign( &ghnoNum, HNO_ZERO );
            NumObjAssign( &hnoTemp, HNO_ZERO );

            for (lIndex=0L; lIndex < lStatNum; lIndex++)
            {

                NumObjAssign(&hnoX, lphnoStatNum[lIndex]);

                addrat( &hnoTemp, hnoX );

                NumObjAssign( &hno, hnoX );
                mulrat( &hno, hnoX );
                addrat( &ghnoNum, hno );

            }


            /*      xý- nxý/ný                               */
            /* fpTemp=fpNum-(fpTemp*fpTemp/(double)lStatNum);*/
            /*                                               */
            NumObjSetIntValue( &hno, lStatNum );
            NumObjAssign( &hnoX, hnoTemp );
            mulrat( &hnoX, hnoTemp );
            divrat( &hnoX, hno );
            NumObjAssign( &hnoTemp, ghnoNum );
            subrat( &hnoTemp, hnoX );


            /* All numbers are identical if fpTemp==0                     */
            if (NumObjIsZero( hnoTemp))
                NumObjAssign( &ghnoNum, HNO_ZERO); /* No deviation.          */
            else {
                /* If bInv=TRUE, divide by n (number of data) otherwise   */
                /* divide by n-1.                                         */
                /* fpNum=sqrt(fpTemp/(lStatNum-1+(LONG)bInv));            */
                //
                // hno still equals lStatNum
                if (!bInv) {
                    subrat( &hno, HNO_ONE );
                }
                divrat( &hnoTemp, hno );
                rootrat( &hnoTemp, HNO_TWO );
                NumObjAssign( &ghnoNum, hnoTemp );
            }
            NumObjDestroy( &hno );
            NumObjDestroy( &hnoX );
            NumObjDestroy( &hnoTemp );
            break;
        }
    }
    GlobalUnlock(hgMem); /* Da memwry is fwee to move as Findows fishes.  */
    return;
}


LONG NEAR StatAlloc (WORD wType, DWORD dwSize)
{
    LONG           lRet=FALSE;

    if (wType==1)
    {
        if ((hMem=GlobalReAlloc(hgMem, dwSize+GMEMCHUNK, GMEM_ZEROINIT)))
            return 0L;
    }
    else
    {
        lRet=(LONG)SendMessage(hListBox, LB_ADDSTRING, 0, (LONG_PTR)(LPTSTR)gpszNum);
        return lRet;
    }
    return 1L;
}


VOID NEAR StatError (VOID)
{
    TCHAR    szFoo[50];  /* This comes locally. Gets the Stat Box Caption. */

    MessageBeep(0);

    /* Error if out of room.                                              */
    GetWindowText(hStatBox, szFoo, 49);
    MessageBox(hStatBox, rgpsz[IDS_STATMEM], szFoo, MB_OK);

    return;
}
