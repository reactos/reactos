/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/date.c
 * PURPOSE:         ReactOS International Control Panel
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#define WINVER 0x0500

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

const INT YEAR_STR_MAX_SIZE=4;
const INT EOLN_SIZE=sizeof(WCHAR); /* size of EOLN char */
#define MAX_SHORT_FMT_SAMPLES    5
#define MAX_LONG_FMT_SAMPLES     2
#define MAX_SHRT_DATE_SEPARATORS 3
#define STD_DATE_SEP             L"."
#define YEAR_DIFF                (99)
#define MAX_YEAR                 (9999)

/* FUNCTIONS ****************************************************************/

/* if char is 'y' or 'M' or 'd' return TRUE, else FALSE */
BOOL
isDateCompAl(WCHAR walpha)
{
    
    if((walpha == L'y') || (walpha == L'M') || (walpha == L'd') || (walpha == L' ')) return TRUE;
    else return FALSE;
}

/* Find first date separator in string */
WCHAR*
FindDateSep(const WCHAR *wszSourceStr)
{
    int nDateCompCount=0;
    int nDateSepCount=0;

	WCHAR* wszFindedSep;
	wszFindedSep=(WCHAR*) malloc(MAX_SAMPLES_STR_SIZE*sizeof(WCHAR));

    wcscpy(wszFindedSep,STD_DATE_SEP);

    while(nDateCompCount<wcslen(wszSourceStr))
    {
        if(!isDateCompAl(wszSourceStr[nDateCompCount]) && (wszSourceStr[nDateCompCount]!=L'\''))
        {
            while(!isDateCompAl(wszSourceStr[nDateCompCount]) && (wszSourceStr[nDateCompCount]!=L'\''))
            {
                wszFindedSep[nDateSepCount++]=wszSourceStr[nDateCompCount];
                nDateCompCount++;
            }
            wszFindedSep[nDateSepCount]='\0';
            return wszFindedSep;
        }
        nDateCompCount++;
    }

    return wszFindedSep;
}

/* Replace given template in source string with string to replace and return recieved string */


/* Setted up short date separator to registry */
BOOL
SetShortDateSep(HWND hwndDlg)
{
    WCHAR wszShortDateSep[MAX_SAMPLES_STR_SIZE];
    int nSepStrSize;
    int nSepCount;

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszShortDateSep);

    /* Get setted separator string size */
    nSepStrSize = wcslen(wszShortDateSep);

    /* Check date components */
    for(nSepCount=0;nSepCount<nSepStrSize;nSepCount++)
    {
        if(iswalnum(wszShortDateSep[nSepCount]) || (wszShortDateSep[nSepCount]=='\''))
        {
            MessageBoxW(NULL,
                        L"Entered short date separator contain incorrect symbol",
                        L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
    }

    /* Save date separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDATE, wszShortDateSep);

    return TRUE;
}

/* Setted up short date format to registry */
BOOL
SetShortDateFormat(HWND hwndDlg)
{
    WCHAR wszShortDateFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszShortDateSep[MAX_SAMPLES_STR_SIZE];
    WCHAR wszFindedDateSep[MAX_SAMPLES_STR_SIZE];

    WCHAR* pwszResultStr;
    BOOL OpenApostFlg = FALSE;
    int nFmtStrSize;
    int nDateCompCount;

    /* Get setted format */
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszShortDateFmt);

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszShortDateSep);

    /* Get setted format-string size */
    nFmtStrSize = wcslen(wszShortDateFmt);

    /* Check date components */
    for(nDateCompCount=0;nDateCompCount<nFmtStrSize;nDateCompCount++)
    {
        if(wszShortDateFmt[nDateCompCount]==L'\'')
        {
            OpenApostFlg=!OpenApostFlg;
        }
        if(iswalnum(wszShortDateFmt[nDateCompCount]) &&
           !isDateCompAl(wszShortDateFmt[nDateCompCount]) &&
           !OpenApostFlg)
        {
            MessageBoxW(NULL,
                        L"Entered short date format contain incorrect symbol",
                        L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
    }

    if(OpenApostFlg)
    {
        MessageBoxW(NULL,
                    L"Entered short date format contain incorrect symbol",
                    L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* substring replacement of separator */
    wcscpy(wszFindedDateSep,FindDateSep(wszShortDateFmt));
    pwszResultStr = ReplaceSubStr(wszShortDateFmt,wszShortDateSep,wszFindedDateSep);
    wcscpy(wszShortDateFmt,pwszResultStr);
    free(pwszResultStr);

    /* Save short date format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, wszShortDateFmt);

    return TRUE;
}

/* Setted up long date format to registry */
BOOL
SetLongDateFormat(HWND hwndDlg)
{
    WCHAR wszLongDateFmt[MAX_SAMPLES_STR_SIZE];
    BOOL OpenApostFlg = FALSE;
    int nFmtStrSize;
    int nDateCompCount;

    /* Get setted format */
    SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszLongDateFmt);

    /* Get setted format string size */
    nFmtStrSize = wcslen(wszLongDateFmt);

    /* Check date components */
    for(nDateCompCount=0;nDateCompCount<nFmtStrSize;nDateCompCount++)
    {
        if(wszLongDateFmt[nDateCompCount]==L'\'')
        {
            OpenApostFlg=!OpenApostFlg;
        }
        if(iswalnum(wszLongDateFmt[nDateCompCount]) &&
           !isDateCompAl(wszLongDateFmt[nDateCompCount]) &&
           !OpenApostFlg)
        {
            MessageBoxW(NULL,
                        L"Entered long date format contain incorrect symbol",
                        L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
    }

    if(OpenApostFlg)
    {
        MessageBoxW(NULL,
                    L"Entered long date format contain incorrect symbol",
                    L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* Save short date format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, wszLongDateFmt);

    return TRUE;
}

/* Init short date separator control box */
VOID
InitShortDateSepSamples(HWND hwndDlg)
{
    WCHAR ShortDateSepSamples[MAX_SHRT_DATE_SEPARATORS][MAX_SAMPLES_STR_SIZE]=
    {
        L".",
        L"/",
        L"-"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszShortDateSep[MAX_SAMPLES_STR_SIZE];

    /* Get current short date separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDATE,
                   wszShortDateSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of separators */
    for(nCBIndex=0;nCBIndex<MAX_SHRT_DATE_SEPARATORS;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)ShortDateSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszShortDateSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                     CB_ADDSTRING,
                     MAX_SHRT_DATE_SEPARATORS+1,
                     (LPARAM)wszShortDateSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszShortDateSep);
    }
}

/* Init short date control box */
VOID
InitShortDateCB(HWND hwndDlg)
{
    WCHAR ShortDateFmtSamples[MAX_SHORT_FMT_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"dd.MM.yyyy",
        L"dd.MM.yy",
        L"d.M.yy",
        L"dd/MM/yy",
        L"yyyy-MM-dd"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszShortDateFmt[MAX_SAMPLES_STR_SIZE];

    /* Get current short date format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SSHORTDATE,
                   wszShortDateFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of date formats */
    for(nCBIndex=0;nCBIndex<MAX_SHORT_FMT_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)ShortDateFmtSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszShortDateFmt);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                     CB_ADDSTRING,
                     MAX_SHORT_FMT_SAMPLES+1,
                     (LPARAM)wszShortDateFmt);
        SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATEFMT_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszShortDateFmt);
    }
}

/* Init long date control box */
VOID
InitLongDateCB(HWND hwndDlg)
{
    /* Where this data stored? */
    WCHAR LongDateFmtSamples[MAX_LONG_FMT_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"d MMMM yyyy 'y.'",
        L"dd MMMM yyyy 'y.'"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszLongDateFmt[MAX_SAMPLES_STR_SIZE];

    /* Get current long date format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SLONGDATE,
                   wszLongDateFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of date formats */
    for(nCBIndex=0;nCBIndex<MAX_LONG_FMT_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)LongDateFmtSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszLongDateFmt);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                     CB_ADDSTRING,
                     MAX_LONG_FMT_SAMPLES+1,
                     (LPARAM)wszLongDateFmt);
        SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATEFMT_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszLongDateFmt);
    }
}

/* Set up max date value to registry */
VOID
SetMaxDate(HWND hwndDlg)
{
    const HWND hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);
    WCHAR wszMaxDateVal[YEAR_STR_MAX_SIZE];
    INT nSpinVal;

    /* Get spin value */
    nSpinVal=LOWORD(SendMessage(hWndYearSpin,
                    UDM_GETPOS,
                    0,
                    0));

    /* convert to wide char */
    _itow(nSpinVal,wszMaxDateVal,DECIMAL_RADIX);

    /* Save max date value */
    SetCalendarInfoW(LOCALE_USER_DEFAULT,
                     CAL_GREGORIAN,
                     48 , /* CAL_ITWODIGITYEARMAX */
                     (LPCWSTR)&wszMaxDateVal);
}

/* Get max date value from registry set */
INT
GetMaxDate()
{
    int nMaxDateVal;

    GetCalendarInfoW(LOCALE_USER_DEFAULT,
                     CAL_GREGORIAN,
                     CAL_ITWODIGITYEARMAX | CAL_RETURN_NUMBER,
                     NULL,
                     0, /* ret type - number */
                     (LPDWORD)&nMaxDateVal);

    return nMaxDateVal;
}

/* Set's MIN data edit control value to MAX-99 */
static
VOID
SetMinData(HWND hwndDlg)
{
    WCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    const HWND hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Get spin value */
    INT nSpinVal=LOWORD(SendMessage(hWndYearSpin,
                        UDM_GETPOS,
                        0,
                        0));

    /* Set min year value */
    wsprintf(OutBuffer, L"%d", (DWORD)nSpinVal-YEAR_DIFF);
    SendMessageW(GetDlgItem(hwndDlg, IDC_FIRSTYEAR_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);
}

/* Init spin control */
static
VOID
InitMinMaxDateSpin(HWND hwndDlg)
{
    WCHAR OutBuffer[YEAR_STR_MAX_SIZE];
    const HWND hWndYearSpin = GetDlgItem(hwndDlg, IDC_SCR_MAX_YEAR);

    /* Init max date value */
    wsprintf(OutBuffer, L"%04d", (DWORD)GetMaxDate());
    SendMessageW(GetDlgItem(hwndDlg, IDC_SECONDYEAR_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);

    /* Init min date value */
    wsprintf(OutBuffer, L"%04d", (DWORD)GetMaxDate()-YEAR_DIFF);
    SendMessageW(GetDlgItem(hwndDlg, IDC_FIRSTYEAR_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);

    /* Init updown control */
    /* Set bounds */
    SendMessageW(hWndYearSpin,
                 UDM_SETRANGE,
                 0,
                 MAKELONG(MAX_YEAR,YEAR_DIFF));

    /* Set current value */
    SendMessageW(hWndYearSpin,
                 UDM_SETPOS,
                 0,
                 MAKELONG(GetMaxDate(),0));

}

/* Update all date locale samples */
static
VOID
UpdateDateLocaleSamples(HWND hwndDlg,
                        LCID lcidLocale)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get short date format sample */
    GetDateFormatW(lcidLocale, DATE_SHORTDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_SHRTDATESAMPLE_EDIT), WM_SETTEXT,
        0, (LPARAM)OutBuffer);

    /* Get long date sample */
    GetDateFormatW(lcidLocale, DATE_LONGDATE, NULL, NULL, OutBuffer,
        MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_LONGDATESAMPLE_EDIT),
        WM_SETTEXT, 0, (LPARAM)OutBuffer);
}

/* Date options setup page dialog callback */
INT_PTR
CALLBACK
DateOptsSetProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    //int i,j;
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            InitMinMaxDateSpin(hwndDlg);
            UpdateDateLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
            InitShortDateCB(hwndDlg);
            InitLongDateCB(hwndDlg);
            InitShortDateSepSamples(hwndDlg);
            /* TODO: Add other calendar types */
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_SECONDYEAR_EDIT:
                {
                    if(HIWORD(wParam)==EN_CHANGE)
                    {
                        SetMinData(hwndDlg);
                    }
                }

                case IDC_SCR_MAX_YEAR:
                {
                    /* Set "Apply" button enabled */
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;
                case IDC_CALTYPE_COMBO:
                case IDC_HIJCHRON_COMBO:
                case IDC_SHRTDATEFMT_COMBO:
                case IDC_SHRTDATESEP_COMBO:
                case IDC_LONGDATEFMT_COMBO:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE ||
                        HIWORD(wParam) == CBN_EDITCHANGE)
                    {
                        /* Set "Apply" button enabled */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;
            /* If push apply button */
            if (lpnm->code == (UINT)PSN_APPLY)
            {
                SetMaxDate(hwndDlg);
                if(!SetShortDateSep(hwndDlg)) break;
                if(!SetShortDateFormat(hwndDlg)) break;
                if(!SetLongDateFormat(hwndDlg)) break;
                InitShortDateCB(hwndDlg);
                /* FIXME: */
                Sleep(15);
                UpdateDateLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
            }
        }
        break;

  }
  return FALSE;
}

/* EOF */
