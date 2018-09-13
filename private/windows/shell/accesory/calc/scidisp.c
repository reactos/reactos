/****************************Module*Header***********************************\
* Module Name: SCIDISP.C
*
* Module Descripton:
*
* Warnings:
*
* Created:
*
* Author:
\****************************************************************************/

#include "scicalc.h"
#include "unifunc.h"
#include "input.h"


extern HNUMOBJ      ghnoNum;
extern eNUMOBJ_FMT  nFE;
extern TCHAR        szDec[5];
extern TCHAR        gszSep[5];
extern LPTSTR       gpszNum;
extern BOOL         gbRecord;
extern BOOL         gbUseSep;
extern CALCINPUTOBJ gcio;


/****************************************************************************\
* void DisplayNum(void)
*
* Convert ghnoNum to a string in the current radix.
*
* Updates the following globals:
*   ghnoNum, gpszNum
\****************************************************************************/
//
// State of calc last time DisplayNum was called
//
typedef struct {
    HNUMOBJ     hnoNum;
    LONG        nPrecision;
    LONG        nRadix;
    INT         nFE;
    INT         nCalc;
    INT         nHexMode;
    BOOL        fIntMath;
    BOOL        bRecord;
    BOOL        bUseSep;
} LASTDISP;

LASTDISP gldPrevious = { NULL, -1, -1, -1, -1, -1, FALSE, FALSE, FALSE };

#define InvalidLastDisp( pglp ) ((pglp)->hnoNum == NULL )


void AddNumSeparator(TCHAR sep, int sepLen, PTSTR szDisplay,
                     PTSTR szSepDisplay);


void DisplayNum(void)
{
    SetWaitCursor( TRUE );

    //
    // Only change the display if
    //  we are in record mode                               -OR-
    //  this is the first time DisplayNum has been called,  -OR-
    //  something important has changed since the last time DisplayNum was
    //  called.
    //
    if ( gbRecord || InvalidLastDisp( &gldPrevious ) ||
            !NumObjIsEq( gldPrevious.hnoNum,      ghnoNum     ) ||
            gldPrevious.nPrecision  != nPrecision   ||
            gldPrevious.nRadix      != nRadix       ||
            gldPrevious.nFE         != (int)nFE     ||
            gldPrevious.nCalc       != nCalc        ||
            gldPrevious.bUseSep     != gbUseSep     ||
            gldPrevious.nHexMode    != nHexMode     ||
            gldPrevious.fIntMath    != F_INTMATH()  ||
            gldPrevious.bRecord     != gbRecord )
    {
        // Assign is an expensive operation, only do when really needed
        if ( ghnoNum )
            NumObjAssign( &gldPrevious.hnoNum, ghnoNum );

        gldPrevious.nPrecision = nPrecision;
        gldPrevious.nRadix     = nRadix;
        gldPrevious.nFE        = (int)nFE;
        gldPrevious.nCalc      = nCalc;
        gldPrevious.nHexMode   = nHexMode;

        gldPrevious.fIntMath   = F_INTMATH();
        gldPrevious.bRecord    = gbRecord;
        gldPrevious.bUseSep    = gbUseSep;

        if (gbRecord)
        {
            // Display the string and return.

            CIO_vConvertToString(&gpszNum, &gcio, nRadix);
        }
        else if (!F_INTMATH())
        {
            // Decimal conversion

            NumObjGetSzValue( &gpszNum, ghnoNum, nRadix, nFE );
        }
        else
        {
            // Non-decimal conversion
            int i;

            // Truncate to an integer.  Do not round here.
            intrat( &ghnoNum );

            // Check the range.
            if ( NumObjIsLess( ghnoNum, HNO_ZERO ) )
            {
                // if negative make positive by doing a twos complement
                NumObjNegate( &ghnoNum );
                subrat( &ghnoNum, HNO_ONE );
                NumObjNot( &ghnoNum );
            }

            andrat( &ghnoNum, g_ahnoChopNumbers[nHexMode] );

            NumObjGetSzValue( &gpszNum, ghnoNum, nRadix, FMT_FLOAT );

            // Clobber trailing decimal point
            i = lstrlen( gpszNum ) - 1;
            if ( i >= 0 && gpszNum[i] == szDec[0] )
                gpszNum[i] = TEXT('\0');
        }

        // Display the string and return.

        if (!gbUseSep)
        {
            TCHAR szTrailSpace[256];

            lstrcpy(szTrailSpace,gpszNum);
            lstrcat(szTrailSpace,TEXT(" "));
            SetDlgItemText(g_hwndDlg, IDC_DISPLAY, szTrailSpace);
        }
        else
        {
            switch(nRadix)
            {
                TCHAR szSepNum[256];

                case 10:
                    AddNumSeparator(gszSep[0], 3, gpszNum, szSepNum);
                    lstrcat(szSepNum,TEXT(" "));
                    SetDlgItemText(g_hwndDlg, IDC_DISPLAY, szSepNum);
                    break;

                case 2:
                case 16:
                    AddNumSeparator(TEXT(' '), 4, gpszNum, szSepNum);
                    lstrcat(szSepNum,TEXT(" "));
                    SetDlgItemText(g_hwndDlg, IDC_DISPLAY, szSepNum);
                    break;

                default:
                    lstrcpy(szSepNum,gpszNum);
                    lstrcat(szSepNum,TEXT(" "));
                    SetDlgItemText(g_hwndDlg, IDC_DISPLAY, szSepNum);
            }
        }
    }

    SetWaitCursor( FALSE );

    return;
}

/****************************************************************************\
*
* WatchDogThread
*
* Thread to look out for functions that take too long.  If it finds one, it
* prompts the user if he wants to abort the function, and asks RATPAK to
* abort if he does.
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
BOOL gfExiting = FALSE;
HANDLE ghCalcStart = NULL;
HANDLE ghCalcDone = NULL;
HANDLE ghDogThread = NULL;

INT_PTR TimeOutMessageBox( void );

DWORD WINAPI WatchDogThread( LPVOID pvParam ) {
    DWORD   cmsWait;
    INT_PTR iRet;

    while( !gfExiting ) {
        WaitForSingleObject( ghCalcStart, INFINITE );
        if (gfExiting)
            break;

        cmsWait = CMS_CALC_TIMEOUT;

        while( WaitForSingleObject( ghCalcDone, cmsWait ) == WAIT_TIMEOUT ) {

            // Put up the msg box
            MessageBeep( MB_ICONEXCLAMATION );
            iRet = TimeOutMessageBox();

            // if user wants to cancel, then stop
            if (gfExiting || iRet == IDYES || iRet == IDCANCEL) {
                NumObjAbortOperation(TRUE);
                break;
            } else {
                cmsWait *= 2;
                if (cmsWait > CMS_MAX_TIMEOUT) {
                    cmsWait = CMS_MAX_TIMEOUT;
                }
            }
        }
    }

    return 42;
}

/****************************************************************************\
*
* TimeCalc
*
*   Function to keep track of how long Calc is taking to do a calculation.
* If calc takes too long (about 10 sec's), then a popup is put up asking the
* user if he wants to abort the operation.
*
* Usage:
*   TimeCalc( TRUE );
*   do a lengthy operation
*   TimeCalc( FALSE );
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
HWND ghwndTimeOutDlg = NULL;

void TimeCalc( BOOL fStart ) {
    if (ghCalcStart == NULL) {
        ghCalcStart = CreateEvent( NULL, FALSE, FALSE, NULL );
    }

    if (ghCalcDone == NULL) {
        ghCalcDone = CreateEvent( NULL, TRUE, FALSE, NULL );
    }

    if (ghDogThread == NULL) {
        DWORD tid;
        ghDogThread = CreateThread( NULL, 0, WatchDogThread, NULL, 0, &tid );
    }

    if (fStart) {
        NumObjAbortOperation(FALSE);
        ResetEvent( ghCalcDone );
        SetEvent( ghCalcStart );
    } else {

        SetEvent( ghCalcDone );

        if( ghwndTimeOutDlg != NULL ) {
            SendMessage( ghwndTimeOutDlg, WM_COMMAND, IDRETRY, 0L );
        }

        if( NumObjWasAborted() ) {
            DisplayError(SCERR_ABORTED);
        }
    }
}


/****************************************************************************\
*
* KillTimeCalc
*
* Should be called only at the end of the program, just before exiting, to
* kill the background timer thread and free its resources.
*
* History
*   26-Nov-1996 JonPa   Wrote it.
*
\****************************************************************************/
void KillTimeCalc( void ) {
    gfExiting = TRUE;
    SetEvent( ghCalcStart );
    SetEvent( ghCalcDone );

    WaitForSingleObject( ghDogThread, CMS_MAX_TIMEOUT );

    CloseHandle( ghCalcStart );
    CloseHandle( ghCalcDone );
    CloseHandle( ghDogThread );
}


/****************************************************************************\
*
* TimeOutMessageBox
*
*   Puts up a dialog that looks like a message box.  If the operation returns
* before the user has responded to the dialog, the dialog gets taken away.
*
* History
*   04-Dec-1996 JonPa   Wrote it.
*
\****************************************************************************/
INT_PTR
CALLBACK TimeOutDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    RECT rc;
    int y;

    switch( uMsg ) {
    case WM_INITDIALOG:
        ghwndTimeOutDlg = hwndDlg;

        //
        // Move ourselves to be over the main calc window
        //

        // Find the display window so we don't cover it up.
        GetWindowRect(GetDlgItem(g_hwndDlg, IDC_DISPLAY), &rc );
        y = rc.bottom;

        // Get the main calc window pos
        GetWindowRect( g_hwndDlg, &rc );

        SetWindowPos( hwndDlg, 0, rc.left + 15, y + 40, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        break;

    case WM_COMMAND:
        EndDialog( hwndDlg, LOWORD(wParam) );
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR TimeOutMessageBox( void ) {
    return (int)DialogBox( hInst, MAKEINTRESOURCE(IDD_TIMEOUT), NULL, TimeOutDlgProc );
}



/****************************************************************************\
*
* AddNumSeparator
*
*   Inserts the specified separator into the number string at the sepLen
* interval.   This is from the left of the decimal.  The right of the
* decimal (ie fractional part) is copied as is.
*
* History
*   08-Sept-1998 KPeery   Wrote it.
*
\****************************************************************************/
void
AddNumSeparator(TCHAR sep, int sepLen, PTSTR szDisplay, PTSTR szSepDisplay)
{
    PTSTR src,dest, dec;
    int   len, count;

    if ((sep == TEXT('\0')) || (sepLen < 1))
    {
        lstrcpy(szSepDisplay,szDisplay);
        return;
    }

    // find decimal point

    for(dec=szDisplay; (*dec != szDec[0]) && (*dec != TEXT('\0')); dec++)
        ; // do nothing

    // at this point dec should point to '\0' or '.' we will add the left
    // side of the number to the final string

    // num of digits
    len=(int)(dec-szDisplay);

    // plus num of commas
    len+=(len-(*szDisplay == TEXT('-') ? 2 : 1))/sepLen;

    // account for decimal
    count=-1;

    for(src=dec, dest=szSepDisplay+len; src >= szDisplay; src--, dest--)
    {
        if ((count > 0) && ((count % sepLen) == 0))
        {
            *dest=sep;
            dest--;

            // if neg num sign then we didn't want that last comma
            if (*src == TEXT('-'))
               dest++;
        }

        *dest=*src;
        count++;
    }

    //
    // ok, now add the right (fractional) part of the number to the final
    // string.
    //
    dest=szSepDisplay+len;

    lstrcpy(dest,dec);

}

