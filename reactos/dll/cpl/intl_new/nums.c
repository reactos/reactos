/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/nums.c
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

#define SAMPLE_NUMBER               L"123456789"
#define SAMPLE_NEG_NUMBER           L"-123456789"
#define MAX_NUM_SEP_SAMPLES         2
#define MAX_FRAC_NUM_SAMPLES        9
#define MAX_FIELD_SEP_SAMPLES       1
#define MAX_FIELD_DIG_SAMPLES       3
#define MAX_NEG_SIGN_SAMPLES        1
#define MAX_NEG_NUMBERS_SAMPLES     5
#define MAX_LEAD_ZEROES_SAMPLES     2
#define MAX_LIST_SEP_SAMPLES        1
#define MAX_UNITS_SYS_SAMPLES       2
#define EOLN_SIZE                   sizeof(WCHAR)

/* FUNCTIONS ****************************************************************/

/* Init system of units control box */
VOID
InitUnitsSysCB(HWND hwndDlg)
{
    WCHAR wszUnitsSysSamples[MAX_UNITS_SYS_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"Metrics",
        L"Americans"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszUnitsSys[MAX_SAMPLES_STR_SIZE];

    /* Get current system of units */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_IMEASURE,
                   wszUnitsSys,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMUNITSSYS_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create list of standart system of units */
    for(nCBIndex=0;nCBIndex<MAX_UNITS_SYS_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMUNITSSYS_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszUnitsSysSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMUNITSSYS_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszUnitsSys),
                           (LPARAM)0);
}

/* Init elements list separator control box */
VOID
InitListSepCB(HWND hwndDlg)
{
    WCHAR wszListSepSamples[MAX_LIST_SEP_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L";"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszListSep[MAX_SAMPLES_STR_SIZE];

    /* Get current list separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SLIST,
                   wszListSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of signs */
    for(nCBIndex=0;nCBIndex<MAX_LIST_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszListSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszListSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                     CB_ADDSTRING,
                     MAX_LIST_SEP_SAMPLES+1,
                     (LPARAM)wszListSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszListSep);
    }
}

/* Init leading zeroes control box */
VOID
InitLeadingZeroesCB(HWND hwndDlg)
{
    WCHAR wszLeadNumFmtSamples[MAX_LEAD_ZEROES_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L",7",
        L"0,7"
    };

    int nCBIndex;
    int nRetCode;

    WCHAR wszLeadNumFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNumSep[MAX_SAMPLES_STR_SIZE];
    WCHAR* pwszResultStr;
    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;

    /* Get current leading zeroes format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_ILZERO,
                   wszLeadNumFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMLEADZERO_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDECIMAL,
                   wszNumSep,
                   dwValueSize);

    /* Create list of standart leading zeroes formats */
    for(nCBIndex=0;nCBIndex<MAX_LEAD_ZEROES_SAMPLES;nCBIndex++)
    {
        pwszResultStr = ReplaceSubStr(wszLeadNumFmtSamples[nCBIndex],
                                      wszNumSep,
                                      L",");
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMLEADZERO_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszResultStr);
        free(pwszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMLEADZERO_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszLeadNumFmt),
                           (LPARAM)0);
}

/* Init negative numbers format control box */
VOID
InitNegNumFmtCB(HWND hwndDlg)
{
    WCHAR wszNegNumFmtSamples[MAX_NEG_NUMBERS_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"(1,1)",
        L"-1,1",
        L"- 1,1",
        L"1,1-",
        L"1,1 -"
    };

    int nCBIndex;
    int nRetCode;

    WCHAR wszNegNumFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNumSep[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNegSign[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNewSample[MAX_SAMPLES_STR_SIZE];
    WCHAR* pwszResultStr;
    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;

    /* Get current negative numbers format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_INEGNUMBER,
                   wszNegNumFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGFMT_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDECIMAL,
                   wszNumSep,
                   dwValueSize);

    /* Get current negative sign */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SNEGATIVESIGN,
                   wszNegSign,
                   dwValueSize);

    /* Create standart list of negative numbers formats */
    for(nCBIndex=0;nCBIndex<MAX_NEG_NUMBERS_SAMPLES;nCBIndex++)
    {
        /* Replace standart separator to setted */
        pwszResultStr = ReplaceSubStr(wszNegNumFmtSamples[nCBIndex],
                                      wszNumSep,
                                      L",");
        wcscpy(wszNewSample,pwszResultStr);
        free(pwszResultStr);
        /* Replace standart negative sign to setted */
        pwszResultStr = ReplaceSubStr(wszNewSample,
                                      wszNegSign,
                                      L"-");
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGFMT_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszResultStr);
        free(pwszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGFMT_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszNegNumFmt),
                           (LPARAM)0);
}

/* Init negative sign control box */
VOID
InitNegSignCB(HWND hwndDlg)
{
    WCHAR wszNegSignSamples[MAX_NEG_SIGN_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"-"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszNegSign[MAX_SAMPLES_STR_SIZE];

    /* Get current negative sign */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SNEGATIVESIGN,
                   wszNegSign,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of signs */
    for(nCBIndex=0;nCBIndex<MAX_NEG_SIGN_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszNegSignSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszNegSign);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                     CB_ADDSTRING,
                     MAX_NUM_SEP_SAMPLES+1,
                     (LPARAM)wszNegSign);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszNegSign);
    }
}

/* Init number of digidts in field control box */
VOID
InitFieldDigNumCB(HWND hwndDlg)
{
    WCHAR wszFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"0;0",
        L"3;0",
        L"3;2;0"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszFieldDigNum[MAX_SAMPLES_STR_SIZE];
    WCHAR* pwszFieldDigNumSmpl;

    /* Get current field digits num */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SGROUPING,
                   wszFieldDigNum,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of field digits num */
    for(nCBIndex=0;nCBIndex<MAX_FIELD_DIG_SAMPLES;nCBIndex++)
    {

        pwszFieldDigNumSmpl=InsSpacesFmt(SAMPLE_NUMBER,wszFieldDigNumSamples[nCBIndex]);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszFieldDigNumSmpl);
        free(pwszFieldDigNumSmpl);
    }

    pwszFieldDigNumSmpl=InsSpacesFmt(SAMPLE_NUMBER,wszFieldDigNum);
    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)pwszFieldDigNumSmpl);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                     CB_ADDSTRING,
                     MAX_FIELD_DIG_SAMPLES+1,
                     (LPARAM)pwszFieldDigNumSmpl);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)pwszFieldDigNumSmpl);
    }

    free(pwszFieldDigNumSmpl);
}

/* Init field separator control box */
VOID
InitNumFieldSepCB(HWND hwndDlg)
{
    WCHAR wszFieldSepSamples[MAX_FIELD_SEP_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L" "
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get current field separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_STHOUSAND,
                   wszFieldSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of field separators */
    for(nCBIndex=0;nCBIndex<MAX_FIELD_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszFieldSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszFieldSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                     CB_ADDSTRING,
                     MAX_FIELD_SEP_SAMPLES+1,
                     (LPARAM)wszFieldSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszFieldSep);
    }
}

/* Init number of fractional symbols control box */
VOID
InitNumOfFracSymbCB(HWND hwndDlg)
{
    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszFracNum[MAX_SAMPLES_STR_SIZE];
    WCHAR wszFracCount[MAX_SAMPLES_STR_SIZE];

    /* Get current number of fractional symbols */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_IDIGITS,
                   wszFracNum,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFRACSYBMS_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of fractional symbols */
    for(nCBIndex=0;nCBIndex<MAX_FRAC_NUM_SAMPLES;nCBIndex++)
    {
        /* convert to wide char */
        _itow(nCBIndex,wszFracCount,DECIMAL_RADIX);

        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFRACSYBMS_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszFracCount);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFRACSYBMS_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszFracNum),
                           (LPARAM)0);
}

/* Init num decimal separator control box */
VOID
InitNumDecimalSepCB(HWND hwndDlg)
{
    WCHAR wszNumSepSamples[MAX_NUM_SEP_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L",",
        L"."
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszNumSep[MAX_SAMPLES_STR_SIZE];

    /* Get current decimal separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDECIMAL,
                   wszNumSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of decimal separators */
    for(nCBIndex=0;nCBIndex<MAX_NUM_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszNumSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszNumSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                     CB_ADDSTRING,
                     MAX_NUM_SEP_SAMPLES+1,
                     (LPARAM)wszNumSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszNumSep);
    }
}

/* Set units system */
BOOL
SetNumUnitsSys(HWND hwndDlg)
{
    WCHAR wszUnitsSys[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted units system */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NUMUNITSSYS_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszUnitsSys,DECIMAL_RADIX);

    /* Save units system */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, wszUnitsSys);

    return TRUE;
}

/* Set elements list separator */
BOOL
SetNumListSep(HWND hwndDlg)
{
    WCHAR wszListSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted list separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMELEMLISTSEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszListSep);

    /* Save list separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SLIST, wszListSep);

    return TRUE;
}

/* Set leading zero */
BOOL
SetNumLeadZero(HWND hwndDlg)
{
    WCHAR wszLeadZero[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted leading zero format */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NUMLEADZERO_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszLeadZero,DECIMAL_RADIX);

    /* Save leading zero format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO, wszLeadZero);

    return TRUE;
}

/* Set negative sum format */
BOOL
SetNegSumFmt(HWND hwndDlg)
{
    WCHAR wszNegSumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted negative sum format */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGFMT_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszNegSumFmt,DECIMAL_RADIX);

    /* Save negative sum format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, wszNegSumFmt);

    return TRUE;
}

/* Set negative sign */
BOOL
SetNumNegSign(HWND hwndDlg)
{
    WCHAR wszNegSign[MAX_SAMPLES_STR_SIZE];

    /* Get setted negative sign */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMNEGSIGN_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszNegSign);

    /* Save negative sign */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SNEGATIVESIGN, wszNegSign);

    return TRUE;
}

/* Set number of digidts in field  */
BOOL
SetFieldDigNum(HWND hwndDlg)
{
    WCHAR wszFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"0;0",
        L"3;0",
        L"3;2;0"
    };

    int nCurrSel;

    /* Get setted number of digidts in field */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDIGFIELD_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* Save number of digidts in field */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, wszFieldDigNumSamples[nCurrSel]);

    return TRUE;
}

/* Set field separator */
BOOL
SetNumFieldSep(HWND hwndDlg)
{
    WCHAR wszFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted field separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFIELDSEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszFieldSep);

    /* Save field separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, wszFieldSep);

    return TRUE;
}

/* Set number of fractional symbols */
BOOL
SetFracSymNum(HWND hwndDlg)
{
    WCHAR wszFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NUMFRACSYBMS_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszFracSymNum,DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, wszFracSymNum);

    return TRUE;
}

/* Set num decimal separator */
BOOL
SetNumDecimalSep(HWND hwndDlg)
{
    WCHAR wszDecimalSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted decimal separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NUMDECIMALSEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszDecimalSep);

    /* Save decimal separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, wszDecimalSep);

    return TRUE;
}

/* Update all numbers locale samples */
static
VOID
UpdateNumSamples(HWND hwndDlg,
                        LCID lcidLocale)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get positive number format sample */
    GetNumberFormatW(lcidLocale,
                     0,
                     SAMPLE_NUMBER,
                     NULL,
                     OutBuffer,
                     MAX_FMT_SIZE);

    SendMessageW(GetDlgItem(hwndDlg, IDC_NUM_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);

    /* Get positive number format sample */
    GetNumberFormatW(lcidLocale,
                     0,
                     SAMPLE_NEG_NUMBER,
                     NULL,
                     OutBuffer,
                     MAX_FMT_SIZE);

    SendMessageW(GetDlgItem(hwndDlg, IDC_NEGNUM_EDIT),
                 WM_SETTEXT,
                 0,
                 (LPARAM)OutBuffer);
}

/* Numbers options setup page dialog callback */
INT_PTR
CALLBACK
NumsOptsSetProc(HWND hwndDlg,
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
            /* Init combo boxes */
            InitNumDecimalSepCB(hwndDlg);
            InitNumOfFracSymbCB(hwndDlg);
            InitNumFieldSepCB(hwndDlg);
            InitFieldDigNumCB(hwndDlg);
            InitNegSignCB(hwndDlg);
            InitNegNumFmtCB(hwndDlg);
            InitLeadingZeroesCB(hwndDlg);
            InitListSepCB(hwndDlg);
            InitUnitsSysCB(hwndDlg);

            UpdateNumSamples(hwndDlg, LOCALE_USER_DEFAULT);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_NUMDECIMALSEP_COMBO:
                case IDC_NUMFRACSYBMS_COMBO:
                case IDC_NUMFIELDSEP_COMBO:
                case IDC_NUMDIGFIELD_COMBO:
                case IDC_NUMNEGSIGN_COMBO:
                case IDC_NUMNEGFMT_COMBO:
                case IDC_NUMLEADZERO_COMBO:
                case IDC_NUMELEMLISTSEP_COMBO:
                case IDC_NUMUNITSSYS_COMBO:
                case IDC_NUMREGDIG_COMBO:
                case IDC_NUMDIGREPL_COMBO:
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
                if(!SetNumDecimalSep(hwndDlg)) break;
                if(!SetFracSymNum(hwndDlg)) break;
                if(!SetNumFieldSep(hwndDlg)) break;
                if(!SetFieldDigNum(hwndDlg)) break;
                if(!SetNumNegSign(hwndDlg)) break;
                if(!SetNegSumFmt(hwndDlg)) break;
                if(!SetNumLeadZero(hwndDlg)) break;
                if(!SetNumListSep(hwndDlg)) break;
                if(!SetNumUnitsSys(hwndDlg)) break;

                /* Update sum format combo boxes */
                InitLeadingZeroesCB(hwndDlg);
                InitNegNumFmtCB(hwndDlg);

                /* FIXME: */
                Sleep(15);
                /* Update sum format samples */
                UpdateNumSamples(hwndDlg, LOCALE_USER_DEFAULT);
            }
        }
        break;
  }
  return FALSE;
}


/* EOF */
