/** FILE: idate.c ********** Module Header ********************************
 *
 *  Control panel applet for International configuration.  This file holds
 *  everything to do with the Date dialog box within the International
 *  dialog in Control Panel.
 *
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *
 *  Copyright (C) 1990-1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "main.h"

//==========================================================================
//                            Local Definitions
//==========================================================================
#if 1
#define STATIC
#else
#define STATIC static
#endif

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
#define hCBDayOfWeek        (hOrder[0])
#define hFirst              (hOrder[1])
#define hSecond             (hOrder[2])
#define hThird              (hOrder[3])
#define hForth              (hOrder[4])
#define hFifth              (hOrder[5])
#define hSixth              (hOrder[6])
#define hSeventh            (hOrder[7])
#else
#define hCBDayOfWeek (hOrder[0])
#define hECSpace1    (hOrder[1])
#define hFirst       (hOrder[2])
#define hECSpace2    (hOrder[3])
#define hSecond      (hOrder[4])
#define hECSpace3    (hOrder[5])
#define hThird       (hOrder[6])
#endif


//==========================================================================
//                            External Declarations
//==========================================================================



//==========================================================================
//                            Local Data Declarations
//==========================================================================
#ifdef JAPAN    /* V-KeijiY     June.29.1992  */
HWND  hOrder[8];
HANDLE hDayofweekTail;
#else
HWND  hOrder[7];
#endif

HWND  hCBDay, hCBMonth, hCBYear;

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
HWND  hECSpace1, hECSpace2, hECSpace3, hECSpace4;
#endif

RECT  rECSpace1;
RECT  rLongDate;
short nSpacing;

#ifdef JAPAN    /* V-KeijiY     June.29.1992  */
// LONG_DATE_FORMAT
//HANDLE hECSpace4;
RECT rECSpace4;
BOOL bDayofweekTail;

// LONG_DATE_FORMAT
extern DWORD ConvertEraToJapaneseEra(WORD,WORD,WORD);
extern TCHAR NewEra[];
#endif

//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

// *** private functions ***

/* This finds the first control that is checked in the range
 * of controls (nFirst, nLast), inclusive.  If none is checked,
 * the last button will be returned.
 */
STATIC short WhichRadioButton (HWND hDlg, short nFirst, short nLast)
{
    for ( ; nFirst < nLast; ++nFirst)
    {
        if (IsDlgButtonChecked (hDlg, nFirst))
            break;
    }
    return (nFirst);
}


/* This sets the date sample in the dialog
 */
STATIC void ShowLongSample (HWND hDlg)
{
    TCHAR    szSample[120], szTemp[41];

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    TCHAR szDayofweekTemp[20];
#endif

    short    i;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    SendMessage (hCBDayOfWeek, CB_GETLBTEXT,
                 (WPARAM) SendMessage(hCBDayOfWeek, CB_GETCURSEL, 0L, 0L ),
                 (LPARAM) szDayofweekTemp);

    if (!bDayofweekTail)
        lstrcpy (szSample, szDayofweekTemp);
    else
        szSample[0] = TEXT('\0');
#else
    *szSample = TEXT('\0');
#endif

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// when bDayofweekTail is TRUE, hOrder[0] will be re-ordered
    for (i = 1; i < 7; i++)
#else
    for (i = 0; i < 7; i++)
#endif
    {
        SendMessage (hOrder[i], WM_GETTEXT, CharSizeOf(szTemp), (LONG)szTemp);
        lstrcat (szSample, szTemp);

    /* Only put spaces after separators  */
        if (i & 01)
            lstrcat (szSample, szSpace);
    }

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    SendMessage(hOrder[7], WM_GETTEXT, CharSizeOf(szTemp) - 1, (LPARAM)szTemp);
    if (lstrlen(szTemp))
        lstrcat(szSample, szTemp);
    if (bDayofweekTail)
    {
        lstrcat(szSample, szSpace);
        lstrcat(szSample, szDayofweekTemp);
    }
#endif

    SetDlgItemText (hDlg, LDATESAMPLE, szSample);
}


/* This rearranges the date components in memory and on the screen
 */
STATIC void ArrangeLongDate (HWND hDlg, short pattern)
{
    RECT  rRect;
    long  x, y, dx;
    short i;

    /* Determine the order of the date components
     * Notice that hFirst, hSecond, and hThird are macros for
     * components of the hOrder array.
     */
    switch (pattern)
    {
      case 1:

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
                hSecond = hCBDay;
        hThird = hECSpace4;
        hForth = hCBMonth;
        hFifth = hECSpace3;
        hSixth = hCBYear;
        hSeventh = hECSpace2;
#else
        hFirst  = hCBDay;
        hSecond = hCBMonth;
        hThird  = hCBYear;
#endif
        break;

      case 2:

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
        hSecond = hCBYear;
        hThird = hECSpace2;
        hForth = hCBMonth;
        hFifth = hECSpace3;
        hSixth = hCBDay;
        hSeventh = hECSpace4;
#else
        hFirst  = hCBYear;
        hSecond = hCBMonth;
        hThird  = hCBDay;
#endif
        break;

      case 0:
      default:

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
        hSecond = hCBMonth;
        hThird = hECSpace3;
        hForth = hCBDay;
        hFifth = hECSpace4;
        hSixth = hCBYear;
        hSeventh = hECSpace2;
#else
        hFirst  = hCBMonth;
        hSecond = hCBDay;
        hThird  = hCBYear;
#endif
        break;
    }

    /* Now move the windows to the right place and invalidate that part
     * of the dialog
     */

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    {
        HWND hJorder[9];

        // Reorder all elements includeing Day of week and Space1
        hJorder[0] = hDayofweekTail;
        if (!bDayofweekTail)
        {
            for (i=1; i < 9; i++)
                hJorder[i] = hOrder[i-1];
        }
        else
        {
            for (i = 1; i < 8; i++)
                hJorder[i] = hOrder[i];
            hJorder[8] = hCBDayOfWeek;
        }
        x = rLongDate.left;
        y = rLongDate.top;
        for (i=1; i < 9; i++)
        {
            GetWindowRect(hJorder[i], &rRect);
            ScreenToClient(hDlg, &rRect.left);
            ScreenToClient(hDlg, &rRect.right);
            SetWindowPos(hJorder[i], hJorder[i-1], x, y,
                 dx = rRect.right - rRect.left, 0, SWP_NOSIZE | SWP_NOREDRAW);
            x += dx + nSpacing;
        }
    }
#else
    x = rECSpace1.left;
    y = rECSpace1.top;
    dx = rECSpace1.right;
    for (i = 2; i < 7; i++)
    {
        x += dx + nSpacing;
        GetWindowRect (hOrder[i], &rRect);
        dx = rRect.right - rRect.left;
        SetWindowPos (hOrder[i], hOrder[i-1], x, y, 0, 0, SWP_NOSIZE | SWP_NOREDRAW);
    }
#endif

    InvalidateRect (hDlg, &rLongDate, TRUE);
}


/* This creates a single short date format string
 */
STATIC void CreateShortFormat (LPTSTR pszFormat)
{
#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    TCHAR    szDay[5], szMonth[5], szYear[5];
#else
    TCHAR    szDay[3], szMonth[3], szYear[5];
#endif
    TCHAR    *first, *second, *third;

    /* Load in the strings corresponding to leading zero and century;
     * truncate them if necessary
     */
    if (!LoadString (hModule, DATE + 5, szDay, CharSizeOf(szDay)) ||
        !LoadString (hModule, DATE + 6, szMonth, CharSizeOf(szMonth)) ||
        !LoadString (hModule, DATE + 7, szYear, CharSizeOf(szYear)))
        return;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    szDay[2] = szMonth[2] = TEXT('\0');
#endif

    if (!Current.iDayLzero)
        szDay[1] = TEXT('\0');
    if (!Current.iMonLzero)
        szMonth[1] = TEXT('\0');
    if (!Current.iCentury)
        szYear[2] = TEXT('\0');

    /* Determine the order and create the format string
     */
    if (Current.iDate == 2)
    {
        first = szYear;
        second = szMonth;
        third = szDay;
    }
    else
    {
        third = szYear;
        if (Current.iDate == 1)
        {
            first = szDay;
            second = szMonth;
        }
        else
        {
            first = szMonth;
            second = szDay;
        }
    }
    wsprintf (pszFormat, TEXT("%s%s%s%s%s"), first, Current.sDateSep,
              second, Current.sDateSep, third);
}


/* This creates a single long date format string
 */
STATIC void CreateLongFormat (LPTSTR pszFormat)
{
    LPTSTR  pszEnd, pszTemp;
    TCHAR   szTemp[CharSizeOf(Current.sLongDate)];
    short   i, j;
    TCHAR   ch;
    LPTSTR  pszBase;

    pszBase = pszFormat;
    pszEnd = pszFormat + CharSizeOf(Current.sLongDate) - 1;

    /* Put in 0, 3, or 4 'd's to signify a leader
     */

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    if ((i = (short) SendMessage(hCBDayOfWeek, CB_GETCURSEL, 0L, 0L)) &&
        !bDayofweekTail)
    {
        ch = TEXT('d');
        i += 2;
        if (i >= 5)
        {
            i -= 4;
            ch = TEXT('W');   // for Japanese Day of week string
        }
        while (i--)
            *pszFormat++ = ch;
    }
#else
    if (i = (short) SendMessage (hCBDayOfWeek, CB_GETCURSEL, 0L, 0L))
    {
        i += 2;
        while (i--)
            *pszFormat++ = TEXT('d');
    }
#endif

    /* Make up the rest of the string
     * Notice the i++ in the SendMessage line
     */
    for (i = 1; i < 7; i++)        /* THREE laps */
    {
        /* Get the separator and surround it with "'" so you can
         * have M, d, or y in it
         * Notice that a space is appended to the end if necessary,
         * and "'" is changed to "''"
         */
        if ((pszFormat + SendMessage (hOrder[i++], WM_GETTEXT, 80,
                                      (LONG)szTemp)) < pszEnd)
        {
            *pszFormat++ = TEXT('\'');
            pszTemp = szTemp;
            while (*pszTemp)
            {
                if (*pszTemp == TEXT('\''))
                    *pszFormat++ = TEXT('\'');
                *pszFormat++ = *pszTemp++;
            }
            if (*CharPrev (pszBase, pszFormat) != TEXT(' '))
                *pszFormat++ = TEXT(' ');
            *pszFormat++ = TEXT('\'');
        }

        /* Now format the component; the index of the selected item
         * in the combobox plus one is the number of chars we want
         * except that year gets twice as much
         */
        j = (short) (SendMessage (hOrder[i], CB_GETCURSEL, 0L, 0L) + 1);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
        if (hOrder[i] == hCBMonth)
            ch = TEXT('M');
        else if (hOrder[i] == hCBYear)
        {
            if (j > 2)
            {
                switch (j)
                {
                    case 6:
                        *pszFormat++ = TEXT('G');
                        *pszFormat++ = TEXT('G');
                    case 4:
                        j = 2;
                        break;
                    case 5:
                        *pszFormat++ = TEXT('G');
                        *pszFormat++ = TEXT('G');
                    case 3:
                        j = 1;
                        break;
                }
                ch = TEXT('n');
            }
            else
            {
                j *= 2;
                ch = TEXT('y');
            }
        }
        else
            ch = TEXT('d');
#else
        if (hOrder[i] == hCBYear)
        {
            j *= 2;
            ch = TEXT('y');
        }
        else if (hOrder[i] == hCBMonth)
            ch = TEXT('M');
        else
            ch = TEXT('d');
#endif

        if (pszFormat + j < pszEnd)
        {
            while (j--)
                *pszFormat++ = ch;
        }
    }
    *pszFormat = TEXT('\0');

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    {
        TCHAR szTemp[6];
        int nCt;
        BOOL bLdBt;

        i = (short) SendMessage(hCBDayOfWeek, CB_GETCURSEL, 0L, 0L);
        SendMessage(hOrder[7], WM_GETTEXT, CharSizeOf(szTemp), (LPARAM)szTemp);
        if (lstrlen(szTemp))
        {
            lstrcat(pszFormat, TEXT("'"));
            lstrcat(pszFormat, szTemp);
            nCt = 0;
            while(szTemp[nCt])
            {
                nCt++;
                bLdBt = FALSE;
            }
        }
        if (bLdBt || (!bLdBt && szTemp[nCt-1] != TEXT(' ')))
        {
            lstrcat(pszFormat, TEXT(" "));
            lstrcat(pszFormat, TEXT("'"));
        }
        else
        {
            if (i && bDayofweekTail)
                lstrcat(pszFormat, TEXT("' '"));
        }
        pszFormat = pszFormat + lstrlen(pszFormat);
        if (i && bDayofweekTail)
        {
            ch = TEXT('d');
            i += 2;
            if (i >= 5)
            {
                i -= 4;
                ch = TEXT('W');       // for Japanese Day of week string
            }
            while (i--)
                *pszFormat++ = ch;
        }
        *pszFormat = TEXT('\0');
    }
#endif
}


/* This reads the long date string into the ldf struct,
 * and selects the correct format for each component
 */
STATIC short ReadLongDate (LPTSTR pszLDate)
{
    WORD    wKey = 0;
    short   nOrder;
    LDF     LongDF;

    /* Parse the string and set the leadin
     */
    ParseLDF (pszLDate, &LongDF);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    if (LongDF.Leadin)
        wKey = ((LongDF.Leadin & 0xF0) == LDF_JaDAY) ? LongDF.Leadin & 0x0F :
                                                (LongDF.Leadin & 0x0f) - 2;
#else
    if (LongDF.Leadin)
        wKey = (WORD) ((LongDF.Leadin & 0x0F) - 2);
#endif

    SendMessage (hCBDayOfWeek, CB_SETCURSEL, wKey, 0L);

    /* Set the format for each of the components
     */
    for (nOrder = 2; nOrder >= 0; nOrder--)
    {
        wKey = LongDF.Order[nOrder];
        switch (wKey & 0xF0)
        {
          case LDF_DAY:
            SendMessage (hCBDay, CB_SETCURSEL, (wKey & 0x0F) - 1, 0L);
            break;

          case LDF_MONTH:
            SendMessage (hCBMonth, CB_SETCURSEL, (wKey & 0x0F) - 1, 0L);
            break;

          case LDF_YEAR:
            SendMessage (hCBYear, CB_SETCURSEL, (wKey & 0x0F) / 2 - 1, 0L);
            break;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
          case LDF_JaYEAR:
            SendMessage(hCBYear, CB_SETCURSEL, (wKey & 0x0F) + 1, 0L);
            break;
#endif
        }
    }

    /* Determine the order (only these three are allowed
     */
#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    switch(wKey & 0xF0)
    {
        case LDF_MONTH:
            nOrder = 0;
            break;
        case LDF_DAY:
            nOrder = 1;
            break;
        case LDF_YEAR:
        case LDF_JaYEAR:
            nOrder = 2;
            break;
        default:
            nOrder = -1;
    }
    bDayofweekTail = LongDF.Trailin;
#else
    if ((wKey & 0xF0) == LDF_MONTH)
        nOrder = 0;
    else if ((wKey & 0xF0) == LDF_DAY)
        nOrder = 1;
    else if ((wKey & 0xF0) == LDF_YEAR)
        nOrder = 2;
    else
        nOrder = -1;
#endif

    /* Put the separators into the appropriate controls
     */
    SendMessage (hECSpace1, WM_SETTEXT, 0, (LONG) LongDF.LeadinSep);
    SendMessage (hECSpace2, WM_SETTEXT, 0, (LONG) LongDF.Sep[0]);
    SendMessage (hECSpace3, WM_SETTEXT, 0, (LONG) LongDF.Sep[1]);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    SendMessage (hECSpace4, WM_SETTEXT, 0, (LPARAM) LongDF.Sep[2]);
#endif

    return (nOrder);
}


/* This initializes the international date dialog
 */
BOOL InitDateIntlDlg (HWND hDlg)
{
    int     i;
    TCHAR   szInput[41];
    LDF     LongDF;

    /* Initialize these globals
     */
    hCBDayOfWeek = GetDlgItem (hDlg, DAYOFWEEK);

#ifdef JAPAN    /* V-KeijiY  July.20.1992 */
    hSecond = hCBDay = GetDlgItem (hDlg, DAYLONG);
    hForth = hCBMonth = GetDlgItem (hDlg, MONTHLONG);
    hSixth = hCBYear = GetDlgItem (hDlg, YEARLONG);

    hFirst = hECSpace1 = GetDlgItem (hDlg, SPACE1);
    hThird = hECSpace2 = GetDlgItem (hDlg, SPACE2);
    hFifth = hECSpace3 = GetDlgItem (hDlg, SPACE3);
    hSeventh = hECSpace4 = GetDlgItem (hDlg, SPACE4);
#else
    hFirst = hCBDay = GetDlgItem (hDlg, DAYLONG);
    hSecond = hCBMonth = GetDlgItem (hDlg, MONTHLONG);
    hThird = hCBYear = GetDlgItem (hDlg, YEARLONG);
    hECSpace1 = GetDlgItem (hDlg, SPACE1);
    hECSpace2 = GetDlgItem (hDlg, SPACE2);
    hECSpace3 = GetDlgItem (hDlg, SPACE3);
#endif

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    hDayofweekTail = GetDlgItem (hDlg, DAYOFWEEKTAIL);
#endif

    /* Set the limits on the separators
     */
    SendMessage (hECSpace1, EM_LIMITTEXT, CharSizeOf(LongDF.LeadinSep) - 1, 0L);
    SendMessage (hECSpace2, EM_LIMITTEXT, CharSizeOf(LongDF.Sep[0]) - 1, 0L);
    SendMessage (hECSpace3, EM_LIMITTEXT, CharSizeOf(LongDF.Sep[1]) - 1, 0L);

    /* rLongDate will be a (hDlg client) rectangle that covers all of the
     * "moveable" dialog controls, wherever they move; this is a
     * rect from the top left of Day to the bottom right of Year
     */
    GetWindowRect (hCBDay, &rLongDate);
    ScreenToClient (hDlg, (LPPOINT) & rLongDate.left);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    GetWindowRect(hECSpace4, &rECSpace1);
#else
    GetWindowRect (hCBYear, &rECSpace1);
#endif

    ScreenToClient (hDlg, (LPPOINT) & rECSpace1.right);
    rLongDate.right = rECSpace1.right;
    rLongDate.bottom = rECSpace1.bottom;

    /* rECSpace1 has the (hDlg client) upper left point of hECSpace1
     * plus the width and the height.
     */
    GetWindowRect (hECSpace1, (LPRECT) & rECSpace1);
    ScreenToClient (hDlg, (LPPOINT) & rECSpace1.left);
    ScreenToClient (hDlg, (LPPOINT) & rECSpace1.right);
    if (rLongDate.bottom < rECSpace1.bottom)
        rLongDate.bottom = rECSpace1.bottom;

    /* This will be used as spacing between controls when moving
     */
    nSpacing = (short) (rLongDate.left - rECSpace1.right);

    /* Change (right, bottom) to (width, height)
     */
    rECSpace1.bottom -= rECSpace1.top;
    rECSpace1.right -= rECSpace1.left;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// Will move between Day of week and Apace4 (everything)
    {
       RECT rcTemp;

       GetWindowRect(hCBDayOfWeek, &rcTemp);
       ScreenToClient(hDlg, &rcTemp.left);
       rLongDate.left = rcTemp.left;
    }
#endif

    /* Initialize the year combobox with the current year
     */
    GetDate ();
    MyItoa (wDateTime[YEAR] % 100, szInput, 10);
    SendMessage (hCBYear, CB_ADDSTRING, 0L, (LONG)szInput);
    MyItoa (wDateTime[YEAR], szInput, 10);
    SendMessage (hCBYear, CB_ADDSTRING, 0L, (LONG)szInput);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
// Support Japanese emperor's era
// combobox entry #3 -> 3
//                   -> 03
// Combobox entry #5 -> "HEISEI"3
//                #6 -> "HEISEI"03
    {
       TCHAR szB[6];
       DWORD dd = ConvertEraToJapaneseEra(wDateTime[YEAR],wDateTime[MONTH],
                                           wDateTime[DAY]);
       wsprintf(szInput, TEXT("%d"), LOWORD(dd));
       SendMessage(hCBYear, CB_ADDSTRING, 0L, (LPARAM)szInput);
       wsprintf(szInput, TEXT("%02.02d"), LOWORD(dd));
       SendMessage(hCBYear, CB_ADDSTRING, 0L, (LPARAM)szInput);
       if (HIWORD(dd) >= 5)       // new era we never know
           lstrcpy(szB, NewEra);
       else
           LoadString(hModule, JaEMPERORYEAR + HIWORD(dd), szB, CharSizeOf(szB));
       wsprintf(szInput, TEXT("%s%d"), szB, LOWORD(dd));
       SendMessage(hCBYear, CB_ADDSTRING, 0L, (LPARAM)szInput);
       wsprintf(szInput, TEXT("%s%02.02d"), szB, LOWORD(dd));
       SendMessage(hCBYear, CB_ADDSTRING, 0L, (LPARAM)szInput);
    }
#endif

    /* Initialize the DayOfWeek combobox
     */
    for (i = 0; i < 3; i++)
    {
        if (!LoadString (hModule, (WORD)(DATE + i), szInput, CharSizeOf(szInput)))
            return (FALSE);
        SendMessage (hCBDayOfWeek, CB_ADDSTRING, 0L, (LONG)szInput);
    }

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    for (i = 0; i < 2; i++ )
    {
        if (!LoadString( hModule, (unsigned) DATE + i + 10, szInput, CharSizeOf(szInput)))
            return(FALSE);
        SendMessage(hCBDayOfWeek, CB_ADDSTRING, 0L, (LPARAM)szInput);
    }
#endif // MSKK

    /* Initialize the Day combobox
     */
    szInput[0] = TEXT('0');
    szInput[1] = TEXT('5');
    szInput[2] = TEXT('\0');
    SendMessage (hCBDay, CB_ADDSTRING, 0L, (LONG)(szInput + 1));
    SendMessage (hCBDay, CB_ADDSTRING, 0L, (LONG)szInput);

    /* Initialize the Month combobox; note that we assume szInput has
     * "03" in it at this time.
     */
    szInput[1] = TEXT('3');
    SendMessage (hCBMonth, CB_ADDSTRING, 0L, (LONG)(szInput + 1));
    SendMessage (hCBMonth, CB_ADDSTRING, 0L, (LONG)szInput);
    for (i = 0; i < 2; i++)
    {
        if (!LoadString (hModule, (WORD)(DATE + 3 + i), szInput, CharSizeOf(szInput)))
            return (FALSE);
        SendMessage (hCBMonth, CB_ADDSTRING, 0, (LONG)szInput);
    }

    /* Parse the long date string, get the order, and set defaults if invalid
     */

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
    SendMessage (hECSpace1, EM_LIMITTEXT, MAX_SPACE_NUM, 0L);
    SendMessage (hECSpace2, EM_LIMITTEXT, MAX_SPACE_NUM, 0L);
    SendMessage (hECSpace3, EM_LIMITTEXT, MAX_SPACE_NUM, 0L);

    // LONG_DATE_FORMAT
    SendMessage (hECSpace4, EM_LIMITTEXT, MAX_SPACE_NUM, 0L);
#endif

    Current.iLDate = ReadLongDate (Current.sLongDate);
    switch (Current.iDate)
    {
      case 0:
      case 1:
      case 2:
        i = Current.iDate;
        break;

      default:
        SendMessage (hCBDayOfWeek, CB_SETCURSEL, 2, 0L);
        SendMessage (hCBDay, CB_SETCURSEL, 0, 0L);
        SendMessage (hCBMonth, CB_SETCURSEL, 3, 0L);
        SendMessage (hCBYear, CB_SETCURSEL, 1, 0L);

        SetDlgItemText (hDlg, SPACE1, szComma);
        SetDlgItemText (hDlg, SPACE3, szComma);

        i = 0;
        break;
    }
    CheckRadioButton (hDlg, MDY, YMD, i + MDY);

    /* Initialize the rest of the controls and arrange the dialog
     */
    SetDlgItemText (hDlg, DATE_SEP, Current.sDateSep);
    SendDlgItemMessage (hDlg, DATE_SEP, EM_LIMITTEXT, 1, 0L);

    CheckDlgButton (hDlg, DAY_LEADINGZERO, Current.iDayLzero);
    CheckDlgButton (hDlg, MONTH_LEADINGZERO, Current.iMonLzero);
    CheckDlgButton (hDlg, CENTURY, Current.iCentury);
    switch (Current.iLDate)
    {
      case 0:
      case 1:
      case 2:
        i = Current.iLDate;
        break;

      default:
        i = 0;
        break;
    }
    CheckRadioButton (hDlg, LONG_MDY, LONG_YMD, i + LONG_MDY);

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
    CheckDlgButton (hDlg, DAYOFWEEKTAIL, !bDayofweekTail);
#endif

    ArrangeLongDate (hDlg, (short) i);
}


/*****************************************************/
/******************** public functions ***************/
/*****************************************************/

/* This is the dialog procedure for the
 * international date dialog
 */
BOOL APIENTRY DateIntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam)
{
    BOOL bShowLongDate = FALSE;
    TCHAR    szInput[3];

    switch (message)
    {
      case WM_INITDIALOG:
        HourGlass (TRUE);
        InitDateIntlDlg (hDlg);
        bShowLongDate = TRUE;
        HourGlass (FALSE);
        break;

      case WM_COMMAND:
        switch (LOWORD(wParam))
        {
          case IDD_HELP:
            goto DoHelp;

          case MDY:
          case DMY:
          case YMD:
            CheckRadioButton (hDlg, MDY, YMD, wParam);
            break;

          case DAY_LEADINGZERO:
          case MONTH_LEADINGZERO:
          case CENTURY:
            CheckDlgButton (hDlg, (int) LOWORD(wParam), (WORD)
                            (!(BOOL)(IsDlgButtonChecked (hDlg, (int)(LOWORD(wParam))))));
            break;

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
// LONG_DATE_FORMAT
          case DAYOFWEEKTAIL:
          {
            int i;

            bDayofweekTail = IsDlgButtonChecked(hDlg, wParam);
            CheckDlgButton(hDlg, wParam, !bDayofweekTail);
            for (i = LONG_MDY; i <= LONG_YMD; i++)
            {
                if (IsDlgButtonChecked(hDlg, i))
                    ArrangeLongDate(hDlg, (short)(i - LONG_MDY));
            }
            bShowLongDate = TRUE;
            break;
          }
#endif

          case LONG_MDY:
          case LONG_DMY:
          case LONG_YMD:
            if (!IsDlgButtonChecked (hDlg, (int)LOWORD(wParam)))
            {
                CheckRadioButton (hDlg, LONG_MDY, LONG_YMD, LOWORD(wParam));
                ArrangeLongDate (hDlg, (short) (LOWORD(wParam) - LONG_MDY));
                bShowLongDate = TRUE;
            }
            break;

          case DAYOFWEEK:
          case DAYLONG:
          case MONTHLONG:
          case YEARLONG:
            if (HIWORD(wParam) == LBN_SELCHANGE)
                bShowLongDate = TRUE;
            break;

          case SPACE1:
          case SPACE2:
          case SPACE3:

#ifdef JAPAN    /* V-KeijiY  June.29.1992 */
          case SPACE4:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                /* Release selection */
                SendMessage((HWND)lParam,EM_SETSEL,0,MAKELONG(0,0));
            }
#endif

            if (HIWORD(wParam) == EN_CHANGE)
                bShowLongDate = TRUE;
            break;

          case PUSH_OK:
            //
            //  Don't allow Date template chars as separator chars.  It will
            //  confuse the NLS apis.
            //

            if (!GetDlgItemText (hDlg, DATE_SEP, szInput, CharSizeOf(szInput)) ||
                _tcspbrk (szInput, TEXT("Mdyg'")) || ExistDigits(szInput))
            {
                MyMessageBox (hDlg, INTL+15, INITS+1, MB_OK | MB_ICONINFORMATION);
                SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, DATE_SEP), 1L);
                break;
            }

            lstrcpy (Current.sDateSep, szInput);

            Current.iDate = WhichRadioButton (hDlg, MDY, YMD) - MDY;
            Current.iLDate = WhichRadioButton (hDlg, LONG_MDY, LONG_YMD)
                                                    - (short) LONG_MDY;
            Current.iDayLzero = IsDlgButtonChecked (hDlg, DAY_LEADINGZERO);
            Current.iMonLzero = IsDlgButtonChecked (hDlg, MONTH_LEADINGZERO);
            Current.iCentury = IsDlgButtonChecked (hDlg, CENTURY);
            CreateLongFormat (Current.sLongDate);
            CreateShortFormat (Current.sShortDate);

            // fall through...

          case PUSH_CANCEL:
            EndDialog (hDlg, 0L);
            break;
        }
        break;

      default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp (hDlg);
        }
        else
            return FALSE;
        break;
    }

    if (bShowLongDate)
        ShowLongSample (hDlg);
    return (TRUE);

    UNREFERENCED_PARAMETER(lParam);
}


