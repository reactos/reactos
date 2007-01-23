/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/time.c
 * PURPOSE:         ReactOS International Control Panel
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define NO_FLAG                 0
#define MAX_TIME_FMT_SAMPLES    3
#define MAX_TIME_SEP_SAMPLES    1
#define MAX_TIME_AM_SAMPLES     2
#define MAX_TIME_PM_SAMPLES     2
#define EOLN_SIZE               sizeof(WCHAR)
#define STD_TIME_SEP            L":"

/* FUNCTIONS ****************************************************************/

/* if char is 'h' or 'H' or 'm' or 's' or 't' or ' ' return TRUE, else FALSE */
BOOL
isTimeComp(WCHAR walpha)
{
    
    if((walpha == L'h') ||
       (walpha == L'H') ||
       (walpha == L'm') ||
       (walpha == L's') ||
       (walpha == L't') ||
       (walpha == L' ')) return TRUE;
    else return FALSE;
}

/* Find first time separator in string */
WCHAR*
FindTimeSep(const WCHAR *wszSourceStr)
{
    int nDateCompCount=0;
    int nDateSepCount=0;

	WCHAR* wszFindedSep;
	wszFindedSep=(WCHAR*) malloc(MAX_SAMPLES_STR_SIZE*sizeof(WCHAR));

    wcscpy(wszFindedSep,STD_TIME_SEP);

    while(nDateCompCount<wcslen(wszSourceStr))
    {
        if(!isTimeComp(wszSourceStr[nDateCompCount]) && (wszSourceStr[nDateCompCount]!=L'\''))
        {
            while(!isTimeComp(wszSourceStr[nDateCompCount]) && (wszSourceStr[nDateCompCount]!=L'\''))
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

/* Init time PM control box */
VOID
InitPM(HWND hwndDlg)
{
    WCHAR wszTimePMSamples[MAX_TIME_PM_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"PM",
        L""
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszTimePM[MAX_SAMPLES_STR_SIZE];

    /* Get current time PM */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_S2359,
                   wszTimePM,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of time PM */
    for(nCBIndex=0;nCBIndex<MAX_TIME_PM_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszTimePMSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszTimePM);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                     CB_ADDSTRING,
                     MAX_TIME_PM_SAMPLES+1,
                     (LPARAM)wszTimePM);
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszTimePM);
    }

}

/* Init time AM control box */
VOID
InitAM(HWND hwndDlg)
{
    WCHAR wszTimeAMSamples[MAX_TIME_AM_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"AM",
        L""
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszTimeAM[MAX_SAMPLES_STR_SIZE];

    /* Get current time AM */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_S1159,
                   wszTimeAM,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of time AM */
    for(nCBIndex=0;nCBIndex<MAX_TIME_AM_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszTimeAMSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszTimeAM);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                     CB_ADDSTRING,
                     MAX_TIME_AM_SAMPLES+1,
                     (LPARAM)wszTimeAM);
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszTimeAM);
    }

}

/* Init time separator control box */
VOID
InitTimeSeparatorCB(HWND hwndDlg)
{
    WCHAR wszTimeSepSamples[MAX_TIME_SEP_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L":"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszTimeSep[MAX_SAMPLES_STR_SIZE];

    /* Get current time separator*/
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_STIME,
                   wszTimeSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of time separators */
    for(nCBIndex=0;nCBIndex<MAX_TIME_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszTimeSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszTimeSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                     CB_ADDSTRING,
                     MAX_TIME_SEP_SAMPLES+1,
                     (LPARAM)wszTimeSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszTimeSep);
    }

}

/* Init time format control box */
VOID
InitTimeFormatCB(HWND hwndDlg)
{
    WCHAR wszTimeFmtSamples[MAX_TIME_FMT_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"H:mm:ss",
        L"HH:mm:ss",
        L"h:mm:ss tt"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszTimeFmt[MAX_SAMPLES_STR_SIZE];

    /* Get current time format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_STIMEFORMAT,
                   wszTimeFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of time formats */
    for(nCBIndex=0;nCBIndex<MAX_TIME_FMT_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszTimeFmtSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszTimeFmt);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                     CB_ADDSTRING,
                     MAX_TIME_FMT_SAMPLES+1,
                     (LPARAM)wszTimeFmt);
        SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszTimeFmt);
    }
}

/* Set time format to registry */
BOOL
SetTimeFormat(HWND hwndDlg)
{
    WCHAR wszTimeFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszTimeSep[MAX_SAMPLES_STR_SIZE];
    WCHAR wszFindedTimeSep[MAX_SAMPLES_STR_SIZE];

    WCHAR* pwszResultStr;
    BOOL OpenApostFlg = FALSE;
    int nFmtStrSize;
    int nTimeCompCount;

    /* Get setted format */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEFMT_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszTimeFmt);

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszTimeSep);

    /* Get setted format-string size */
    nFmtStrSize = wcslen(wszTimeFmt);

    /* Check date components */
    for(nTimeCompCount=0;nTimeCompCount<nFmtStrSize;nTimeCompCount++)
    {
        if(wszTimeFmt[nTimeCompCount]==L'\'')
        {
            OpenApostFlg=!OpenApostFlg;
        }
        if(iswalnum(wszTimeFmt[nTimeCompCount]) &&
           !isTimeComp(wszTimeFmt[nTimeCompCount]) &&
           !OpenApostFlg)
        {
            MessageBoxW(NULL,
                        L"Entered time format contain incorrect symbol",
                        L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
    }

    if(OpenApostFlg)
    {
        MessageBoxW(NULL,
                    L"Entered time format contain incorrect symbol",
                    L"Error", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    /* substring replacement of separator */
    wcscpy(wszFindedTimeSep,FindTimeSep(wszTimeFmt));
    pwszResultStr = ReplaceSubStr(wszTimeFmt,wszTimeSep,wszFindedTimeSep);
    wcscpy(wszTimeFmt,pwszResultStr);
    free(pwszResultStr);

    /* Save time format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, wszTimeFmt);

    return TRUE;
}

/* Setted up time separator to registry */
BOOL
SetTimeSep(HWND hwndDlg)
{
    WCHAR wszTimeSep[MAX_SAMPLES_STR_SIZE];
    int nSepStrSize;
    int nSepCount;

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMESEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszTimeSep);

    /* Get setted separator string size */
    nSepStrSize = wcslen(wszTimeSep);

    /* Check time components */
    for(nSepCount=0;nSepCount<nSepStrSize;nSepCount++)
    {
        if(iswalnum(wszTimeSep[nSepCount]) || (wszTimeSep[nSepCount]=='\''))
        {
            MessageBoxW(NULL,
                        L"Entered time separator contain incorrect symbol",
                        L"Error", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        
    }

    /* Save time separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STIME, wszTimeSep);

    return TRUE;
}

/* Setted up time AM to registry */
BOOL
SetTimeAM(HWND hwndDlg)
{
    WCHAR wszTimeAM[MAX_SAMPLES_STR_SIZE];

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEAM_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszTimeAM);

    /* Save time AM */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_S1159, wszTimeAM);

    return TRUE;
}

/* Setted up time PM to registry */
BOOL
SetTimePM(HWND hwndDlg)
{
    WCHAR wszTimePM[MAX_SAMPLES_STR_SIZE];

    /* Get setted separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIMEPM_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszTimePM);

    /* Save time PM */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_S2359, wszTimePM);

    return TRUE;
}

/* Update all time locale samples */
static
VOID
UpdateTimeLocaleSamples(HWND hwndDlg,
                        LCID lcidLocale)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get time format sample */
    GetTimeFormatW(lcidLocale,
                   NO_FLAG,
                   NULL,
                   NULL,
                   OutBuffer,
                   MAX_FMT_SIZE);
    SendMessageW(GetDlgItem(hwndDlg, IDC_TIME_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);

    /* TODO: Add unknown control box processing */
}

/* Date options setup page dialog callback */
INT_PTR
CALLBACK
TimeOptsSetProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            InitTimeFormatCB(hwndDlg);
            InitTimeSeparatorCB(hwndDlg);
            InitAM(hwndDlg);
            InitPM(hwndDlg);
            UpdateTimeLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_TIMEFMT_COMBO:
                case IDC_TIMESEP_COMBO:
                case IDC_TIMEAM_COMBO:
                case IDC_TIMEPM_COMBO:
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
                if(!SetTimeFormat(hwndDlg)) break;
                if(!SetTimeSep(hwndDlg)) break;
                if(!SetTimeAM(hwndDlg)) break;
                if(!SetTimePM(hwndDlg)) break;

                InitTimeFormatCB(hwndDlg);
                

                /* FIXME: */
                Sleep(15);
                UpdateTimeLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
            }
        }
        break;
  }
  return FALSE;
}


/* EOF */
