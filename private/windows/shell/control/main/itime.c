/** FILE: itime.c ********** Module Header ********************************
 *
 *  Control panel applet for International configuration.  This file holds
 *  everything to do with the Time dialog box within the International
 *  Dialog of Control Panel.
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


//==========================================================================
//                            External Declarations
//==========================================================================
extern TCHAR szPct02D[];
extern TCHAR szPctD[];

//==========================================================================
//                            Local Data Declarations
//==========================================================================
TCHAR szPST[8];                    /* 24 Hour clock string */


//==========================================================================
//                            Local Function Prototypes
//==========================================================================


//==========================================================================
//                                Functions
//==========================================================================

BOOL APIENTRY TimeIntlDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    UINT AMLen;              /* length of AM/PM designator */
    TCHAR  szTime[20];       /* temp buffer */


    switch (message)
    {
    case WM_INITDIALOG:
        HourGlass (TRUE);
        SendDlgItemMessage (hDlg, TIME_AM, EM_LIMITTEXT, TIMESUF_LEN - 1, 0L);
        SendDlgItemMessage (hDlg, TIME_PM, EM_LIMITTEXT, TIMESUF_LEN - 1, 0L);
        if (Current.iTime == 0)
        {
            CheckRadioButton (hDlg, TIME_12, TIME_24, TIME_12);
            SetDlgItemText (hDlg, TIME_AM, Current.s1159);
            SetDlgItemText (hDlg, TIME_MERIDIAN, TEXT("00:00-11:59"));
            SetDlgItemText (hDlg, TIME_MERIDIAN2, TEXT("12:00-23:59"));
            szPST[0] = TEXT('\0');
        }
        else
        {
            CheckRadioButton (hDlg, TIME_12, TIME_24, TIME_24);
            SetDlgItemText (hDlg, TIME_MERIDIAN, szNull);
            SetDlgItemText (hDlg, TIME_MERIDIAN2, TEXT("00:00-23:59"));
            SetDlgItemText (hDlg, TIME_AM, szNull);
            EnableWindow (GetDlgItem (hDlg, TIME_AM), FALSE);
            ShowWindow (GetDlgItem (hDlg, TIME_AM), SW_HIDE);
            lstrcpy (szPST, Current.s2359);
        }
        SetDlgItemText (hDlg, TIME_PM, Current.s2359);
        SetDlgItemText (hDlg, TIME_SEP, Current.sTime);
        CheckRadioButton (hDlg, TIME_NOHOUR0, TIME_HOUR0, Current.iTLZero ? TIME_HOUR0 : TIME_NOHOUR0);
        CheckRadioButton (hDlg, TIME_SUFFIX, TIME_PREFIX, Current.iTimeMarker ? TIME_PREFIX : TIME_SUFFIX);

        HourGlass (FALSE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_HELP:
            goto DoHelp;

            case TIME_12:
                if (!IsDlgButtonChecked (hDlg, TIME_12))
                {
                    GetDlgItemText (hDlg, TIME_PM, szPST, CharSizeOf(szPST));
                    CheckRadioButton (hDlg, TIME_12, TIME_24, TIME_12);
                    EnableWindow (GetDlgItem (hDlg, TIME_AM), TRUE);
                    ShowWindow (GetDlgItem (hDlg, TIME_AM), SW_SHOW);
                    SetDlgItemText (hDlg, TIME_MERIDIAN, TEXT("00:00-11:59"));
                    SetDlgItemText (hDlg, TIME_MERIDIAN2, TEXT("12:00-23:59"));
                    SetDlgItemText (hDlg, TIME_AM, Current.s1159);
                    SetDlgItemText (hDlg, TIME_PM, Current.s2359);
                }
                break;

            case TIME_24:
                if (!IsDlgButtonChecked (hDlg, TIME_24))
                {
                    GetDlgItemText (hDlg, TIME_AM, Current.s1159, CharSizeOf(Current.s1159));
                    GetDlgItemText (hDlg, TIME_PM, Current.s2359, CharSizeOf(Current.s2359));
                    CheckRadioButton (hDlg, TIME_12, TIME_24, TIME_24);
                    SetDlgItemText (hDlg, TIME_MERIDIAN, szNull);
                    SetDlgItemText (hDlg, TIME_MERIDIAN2, TEXT("00:00-23:59"));
                    SetDlgItemText (hDlg, TIME_AM, szNull);
                    SetDlgItemText (hDlg, TIME_PM, szPST);
                    EnableWindow (GetDlgItem (hDlg, TIME_AM), FALSE);
                    ShowWindow (GetDlgItem (hDlg, TIME_AM), SW_HIDE);
                }
                break;

            case TIME_NOHOUR0:
            case TIME_HOUR0:
                CheckRadioButton (hDlg, TIME_NOHOUR0, TIME_HOUR0, LOWORD(wParam));
                break;

            case TIME_SUFFIX:
            case TIME_PREFIX:
                CheckRadioButton(hDlg, TIME_SUFFIX, TIME_PREFIX, LOWORD(wParam));
                break;

            case PUSH_OK:
                if (!GetDlgItemText (hDlg, TIME_SEP, szTime, CharSizeOf(Current.sTime)) ||
                    _tcspbrk (szTime, TEXT("Hhmst'")) || ExistDigits(szTime))
                {
                    MyMessageBox (hDlg, INTL+13, INITS+1, MB_OK | MB_ICONINFORMATION);
                    SendMessage (hDlg, WM_NEXTDLGCTL, (DWORD)GetDlgItem(hDlg, TIME_SEP), 1L);
                    break;
                }
                lstrcpy (Current.sTime, szTime);

                if (Current.iTime = (IsDlgButtonChecked (hDlg, TIME_24) ? 1 : 0))
                {
                    AMLen = GetDlgItemText (hDlg, TIME_PM, Current.s2359, TIMESUF_LEN);
                    lstrcpy ((LPTSTR)Current.s1159, Current.s2359);
                }
                else
                {
                    AMLen =  GetDlgItemText (hDlg, TIME_AM, Current.s1159, TIMESUF_LEN);
                    AMLen += GetDlgItemText (hDlg, TIME_PM, Current.s2359, TIMESUF_LEN);
                }
                Current.iTLZero = IsDlgButtonChecked (hDlg, TIME_HOUR0) ? 1 : 0;
                Current.iTimeMarker = IsDlgButtonChecked (hDlg, TIME_SUFFIX) ? 0 : 1;

                /*
                 *  Update stimeformat string.
                 */
                if (AMLen)
                {
                    // Use AM/PM designator in time picture.
                    if (Current.iTime)
                    {
                        // 24 hour format.
                        if (Current.iTimeMarker)
                        {
                            // Time Prefix.
                            wsprintf ( Current.sTimeFormat,
                                       ((Current.iTLZero) ? TEXT("tt HH%smm%sss")
                                                          : TEXT("tt H%smm%sss")),
                                       Current.sTime,
                                       Current.sTime );

                        }
                        else
                        {
                            // Time Suffix.
                            wsprintf ( Current.sTimeFormat,
                                       ((Current.iTLZero) ? TEXT("HH%smm%sss tt")
                                                          : TEXT("H%smm%sss tt")),
                                       Current.sTime,
                                       Current.sTime );
                        }
                    }
                    else
                    {
                        // 12 hour format.
                        if (Current.iTimeMarker)
                        {
                            // Time Prefix.
                            wsprintf ( Current.sTimeFormat,
                                       ((Current.iTLZero) ? TEXT("tt hh%smm%sss")
                                                          : TEXT("tt h%smm%sss")),
                                       Current.sTime,
                                       Current.sTime );
                        }
                        else
                        {
                            // Time Suffix.
                            wsprintf ( Current.sTimeFormat,
                                       ((Current.iTLZero) ? TEXT("hh%smm%sss tt")
                                                          : TEXT("h%smm%sss tt")),
                                       Current.sTime,
                                       Current.sTime );
                        }
                    }
                }
                else
                {
                    // Do NOT use AM/PM designator in time picture.
                    if (Current.iTime)
                    {
                        // 24 hour format.
                        wsprintf ( Current.sTimeFormat,
                                   ((Current.iTLZero) ? TEXT("HH%smm%sss")
                                                      : TEXT("H%smm%sss")),
                                   Current.sTime,
                                   Current.sTime );
                    }
                    else
                    {
                        // 12 hour format.
                        wsprintf ( Current.sTimeFormat,
                                   ((Current.iTLZero) ? TEXT("hh%smm%sss")
                                                      : TEXT("h%smm%sss")),
                                   Current.sTime,
                                   Current.sTime );
                    }
                }

                // fall thru....
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
            return (TRUE);
        }
        else
            return (FALSE);
        break;
    }
    return (TRUE);

    lParam;
}

