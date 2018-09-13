/** FILE: keybd.c ********** Module Header ********************************
 *
 *  Control panel applet for Keyboard configuration.  This file holds
 *  everything to do with the "Keyboard" dialog box in the Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *
 *  Copyright (C) 1990-1991 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                        Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

//==========================================================================
//                     Local Definitions
//==========================================================================
#define KSPEED_MIN      0
#define KSPEED_MAX      31
#define KSPEED_RANGE    (KSPEED_MAX - KSPEED_MIN + 1)

/* The following added for keyboard delay control by C. Stevens, Oct. 90 */

#define KDELAY_MIN 0
#define KDELAY_MAX 3
#define KDELAY_RANGE (KDELAY_MAX - KDELAY_MIN + 1)

#ifdef JAPAN    /* V-KeijiY  July.6.1992 */
#define SPI_KANJIMENU   8       //    Defined in ptypes32.h
#endif

//==========================================================================
//                    External Declarations
//==========================================================================


//==========================================================================
//                    Local Data Declarations
//==========================================================================
int    nSpeedScrollPos;    /* current keyboard speed scroll bar position */
int    nDelayScrollPos;    /* current keyboard delay scroll bar position */

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
TCHAR  szKanjiMode[6] = TEXT("roman");
TCHAR  szKanjiMenu[] = TEXT("kanjimenu");
#endif

//==========================================================================
//                    Local Function Prototypes
//==========================================================================

//==========================================================================
//                           Functions
//==========================================================================

BOOL KeyboardDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    HWND   hCtrl;
    static int nKbdOrig;
    int   *pnScrollPtr;
    int    nMin, nMax, nRange;

    static int wOriginalDelay = 2, wOriginalSpeed = 20;

    switch (message)
    {
    case WM_INITDIALOG:
        HourGlass (TRUE);

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
        GetProfileString (szWindows, szKanjiMenu,
                          szKanjiMode, szKanjiMode, CharSizeOf(szKanjiMode));
        CheckRadioButton (hDlg, KEYMODE_ROMAN, KEYMODE_KANJI,
                          lstrcmpi(szKanjiMode, TEXT("kanji")) ?
                              KEYMODE_ROMAN : KEYMODE_KANJI);
#endif

        /* Get initial settings and set up scroll bars.  The keyboard delay
         * scroll bar logic added by C. Stevens, Oct. 90
         */

        SystemParametersInfo (SPI_GETKEYBOARDSPEED, 0,
                                    (PVOID)(int * ) &wOriginalSpeed, FALSE);
        SystemParametersInfo (SPI_GETKEYBOARDDELAY, 0,
                                    (PVOID)(int * ) &wOriginalDelay, FALSE);

        nSpeedScrollPos = wOriginalSpeed;
        nDelayScrollPos = KDELAY_MAX - wOriginalDelay + KDELAY_MIN;

        SetScrollRange (GetDlgItem (hDlg, KSPEED_SCROLL), SB_CTL, KSPEED_MIN, KSPEED_MAX, FALSE);
        SetScrollPos (GetDlgItem (hDlg, KSPEED_SCROLL), SB_CTL, nSpeedScrollPos, FALSE);

        SetScrollRange (GetDlgItem (hDlg, KDELAY_SCROLL), SB_CTL, KDELAY_MIN, KDELAY_MAX, FALSE);
        SetScrollPos (GetDlgItem (hDlg, KDELAY_SCROLL), SB_CTL, nDelayScrollPos, FALSE);

        HourGlass (FALSE);
        break;

    case WM_HSCROLL:

        /* Determine which scroll bar is being used.  Copy its attributes
         * into nKbdOrig,nMin,nMax,nRange,pnScrollPtr, and hCtrl.  This
         * code added to process keyboard speed AND delay scrollbars.
         * C. Stevens, Oct. 90
         */

        if ((HWND) lParam == GetDlgItem (hDlg, KSPEED_SCROLL))
        {
            if (LOWORD(wParam) != SB_ENDSCROLL)
                nKbdOrig = nSpeedScrollPos;

            nMin = KSPEED_MIN;
            nMax = KSPEED_MAX;
            nRange = KSPEED_RANGE;
            pnScrollPtr = &nSpeedScrollPos;
            hCtrl = GetDlgItem (hDlg, KSPEED_SCROLL);
        }
        else
        {
           if (LOWORD(wParam) != SB_ENDSCROLL)
                nKbdOrig = nDelayScrollPos;

            nMin = KDELAY_MIN;
            nMax = KDELAY_MAX;
            nRange = KDELAY_RANGE;
            pnScrollPtr = &nDelayScrollPos;
            hCtrl = GetDlgItem (hDlg, KDELAY_SCROLL);
        }

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            if (--(*pnScrollPtr) < nMin)
                *pnScrollPtr = nMin;
            break;

        case SB_LINEDOWN:
            if (++(*pnScrollPtr) > nMax)
                *pnScrollPtr = nMax;
            break;

        case SB_PAGEUP:
            if ((*pnScrollPtr -= (nRange / 4)) < nMin)
                *pnScrollPtr = nMin;
            break;

        case SB_PAGEDOWN:
            if ((*pnScrollPtr += (nRange / 4)) > nMax)
                *pnScrollPtr = nMax;
            break;

        case SB_THUMBPOSITION:
            *pnScrollPtr = HIWORD(wParam);
            break;

        case SB_TOP:
            *pnScrollPtr = nMin;
            break;

        case SB_BOTTOM:
            *pnScrollPtr = nMax;
            break;

        case SB_ENDSCROLL:

            /* Form keyboard driver byte from keyboard delay and speed.
             *   Added by C. Stevens, Oct. 90
             */

// [stevecat] remove test to fix "boundary bug" - if user holds down arrow
//            key until scroll thumb reaches end of scroll range, clipping
//            of values (done above) will make this test fail - hence causing
//            speed and/or delay to NOT be set.
//            if (nKbdOrig != *pnScrollPtr)
//            {
            // Always set new speed and delay on EndScroll message

            SystemParametersInfo (SPI_SETKEYBOARDSPEED, (WORD)nSpeedScrollPos,
                                  0L, FALSE);
            SystemParametersInfo (SPI_SETKEYBOARDDELAY, (WORD) KDELAY_MAX -
                                  nDelayScrollPos + KDELAY_MIN, 0L, FALSE);

//            }
            SetDlgItemText(hDlg, KSPEED_EDIT, MYNUL);
            break;
          }

        if (nKbdOrig != *pnScrollPtr && LOWORD(wParam) != SB_ENDSCROLL)
        {
            SetScrollPos(hCtrl, SB_CTL, *pnScrollPtr, TRUE);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
        case KEYMODE_ROMAN:
        case KEYMODE_KANJI:
            lstrcpy (szKanjiMode, (wParam == KEYMODE_KANJI ?
                                     TEXT("kanji") : TEXT("roman") ));
            CheckRadioButton(hDlg, KEYMODE_ROMAN, KEYMODE_KANJI, wParam);
            break;
#endif

        case IDOK:
            HourGlass (TRUE);

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
            if (!WriteProfileString (szWindows, szKanjiMenu, szKanjiMode))
                MyMessageBox(hDlg, UTILS+1, INITS+1, MB_OK|MB_ICONINFORMATION);
            SystemParametersInfo (SPI_KANJIMENU,
                                  lstrcmpi (szKanjiMode, TEXT("kanji")) ?
                                     2 : 3, (LONG) NULL, SPIF_SENDWININICHANGE );
#endif

            /* Form keyboard driver byte from keyboard delay and speed.
             *    Added by C. Stevens, Oct. 90
             */

            SystemParametersInfo (SPI_SETKEYBOARDSPEED, (WORD) nSpeedScrollPos,
                                                                    0L, TRUE);
            SystemParametersInfo (SPI_SETKEYBOARDDELAY, (WORD)KDELAY_MAX -
                                   nDelayScrollPos + KDELAY_MIN, 0L,TRUE);

            SendWinIniChange (szWindows);
            EndDialog (hDlg, 0L);
            HourGlass (FALSE);
            break;

        case IDCANCEL:

            /* restore original keyboard speed */

            SystemParametersInfo (SPI_SETKEYBOARDSPEED, (WORD) wOriginalSpeed,
                                                                0L, FALSE);
            SystemParametersInfo (SPI_SETKEYBOARDDELAY, (WORD) wOriginalDelay,
                                                                0L, FALSE);
            EndDialog (hDlg, 0L);
            break;
        }
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
            return TRUE;
        }
        else
            return FALSE;
        break;
    }
    return (TRUE);
}

