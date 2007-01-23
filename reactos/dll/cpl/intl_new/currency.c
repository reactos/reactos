/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/monetary.c
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
#define MAX_CURRENCY_UNIT_SAMPLES   2
#define MAX_POS_CURRENCY_SAMPLES    4
#define MAX_NEG_CURRENCY_SAMPLES    16
#define MAX_CURRENCY_SEP_SAMPLES    2
#define MAX_CURRENCY_FRAC_SAMPLES   10
#define MAX_FIELD_SEP_SAMPLES       1
#define MAX_FIELD_DIG_SAMPLES       3
#define EOLN_SIZE                   sizeof(WCHAR)

/* FUNCTIONS ****************************************************************/

/* Init number of digidts in field control box */
VOID
InitCurrencyDigNumCB(HWND hwndDlg)
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
                   LOCALE_SMONGROUPING,
                   wszFieldDigNum,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of field digits num */
    for(nCBIndex=0;nCBIndex<MAX_FIELD_DIG_SAMPLES;nCBIndex++)
    {

        pwszFieldDigNumSmpl=InsSpacesFmt(SAMPLE_NUMBER,wszFieldDigNumSamples[nCBIndex]);
        SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszFieldDigNumSmpl);
        free(pwszFieldDigNumSmpl);
    }

    pwszFieldDigNumSmpl=InsSpacesFmt(SAMPLE_NUMBER,wszFieldDigNum);
    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)pwszFieldDigNumSmpl);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                     CB_ADDSTRING,
                     MAX_FIELD_DIG_SAMPLES+1,
                     (LPARAM)pwszFieldDigNumSmpl);
        SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)pwszFieldDigNumSmpl);
    }

    free(pwszFieldDigNumSmpl);
}


/* Init currency field separator control box */
VOID
InitCurrencyFieldSepCB(HWND hwndDlg)
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
                   LOCALE_SMONTHOUSANDSEP,
                   wszFieldSep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of field separators */
    for(nCBIndex=0;nCBIndex<MAX_FIELD_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszFieldSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszFieldSep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                     CB_ADDSTRING,
                     MAX_FIELD_SEP_SAMPLES+1,
                     (LPARAM)wszFieldSep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszFieldSep);
    }
}

/* Init number of fractional symbols control box */
VOID
InitCurrencyFracNumCB(HWND hwndDlg)
{
    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszCurrencyFracNum[MAX_SAMPLES_STR_SIZE];
    WCHAR wszFracCount[MAX_SAMPLES_STR_SIZE];

    /* Get current number of fractional symbols */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_ICURRDIGITS,
                   wszCurrencyFracNum,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_FRACSYMBSNUM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of fractional symbols */
    for(nCBIndex=0;nCBIndex<MAX_CURRENCY_FRAC_SAMPLES;nCBIndex++)
    {
        /* convert to wide char */
        _itow(nCBIndex,wszFracCount,DECIMAL_RADIX);

        SendMessageW(GetDlgItem(hwndDlg, IDC_FRACSYMBSNUM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszFracCount);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_FRACSYMBSNUM_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszCurrencyFracNum),
                           (LPARAM)0);
}


/* Init positive currency sum format control box */
VOID
InitPosCurrencySumCB(HWND hwndDlg)
{
    WCHAR wszPosCurrencySumSamples[MAX_POS_CURRENCY_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"$1,1",
        L"1,1$",
        L"$ 1,1",
        L"1,1 $"
    };

    int nCBIndex;
    int nRetCode;

    WCHAR wszCurrPosFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszCurrencyUnit[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNewSample[MAX_SAMPLES_STR_SIZE];
    WCHAR wszCurrencySep[MAX_SAMPLES_STR_SIZE];
    WCHAR* pwszResultStr;
    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;

    /* Get current currency sum format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_ICURRENCY,
                   wszCurrPosFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_POSCURRENCYSUM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Get current currency separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SMONDECIMALSEP,
                   wszCurrencySep,
                   dwValueSize);

    /* Get current currency unit */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SCURRENCY,
                   wszCurrencyUnit,
                   dwValueSize);

    /* Create standart list of currency sum formats */
    for(nCBIndex=0;nCBIndex<MAX_POS_CURRENCY_SAMPLES;nCBIndex++)
    {
        pwszResultStr = ReplaceSubStr(wszPosCurrencySumSamples[nCBIndex],
                                      wszCurrencyUnit,
                                      L"$");
        wcscpy(wszNewSample,pwszResultStr);
        free(pwszResultStr);
        pwszResultStr = ReplaceSubStr(wszNewSample,
                                      wszCurrencySep,
                                      L",");
        SendMessageW(GetDlgItem(hwndDlg, IDC_POSCURRENCYSUM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszResultStr);
        free(pwszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_POSCURRENCYSUM_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszCurrPosFmt),
                           (LPARAM)0);
}

/* Init negative currency sum format control box */
VOID
InitNegCurrencySumCB(HWND hwndDlg)
{
    WCHAR wszNegCurrencySumSamples[MAX_NEG_CURRENCY_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"(?1,1)",
        L"-?1,1",
        L"?-1,1",
        L"?1,1-",
        L"(1,1?)",
        L"-1,1?",
        L"1,1-?",
        L"1,1?-",
        L"-1,1 ?",
        L"-? 1,1",
        L"1,1 ?-",
        L"? 1,1-",
        L"? -1,1",
        L"1,1- ?",
        L"(? 1,1)",
        L"(1,1 ?)" /* 16 */
    };

    int nCBIndex;
    int nRetCode;

    WCHAR wszCurrNegFmt[MAX_SAMPLES_STR_SIZE];
    WCHAR wszCurrencyUnit[MAX_SAMPLES_STR_SIZE];
    WCHAR wszCurrencySep[MAX_SAMPLES_STR_SIZE];
    WCHAR wszNewSample[MAX_SAMPLES_STR_SIZE];
    WCHAR* pwszResultStr;
    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;

    /* Get current currency sum format */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_INEGCURR,
                   wszCurrNegFmt,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_NEGCURRENCYSUM_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Get current currency unit */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SCURRENCY,
                   wszCurrencyUnit,
                   dwValueSize);

    /* Get current currency separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SMONDECIMALSEP,
                   wszCurrencySep,
                   dwValueSize);

    /* Create standart list of currency sum formats */
    for(nCBIndex=0;nCBIndex<MAX_NEG_CURRENCY_SAMPLES;nCBIndex++)
    {
        pwszResultStr = ReplaceSubStr(wszNegCurrencySumSamples[nCBIndex],
                                      wszCurrencyUnit,
                                      L"?");
        wcscpy(wszNewSample,pwszResultStr);
        free(pwszResultStr);
        pwszResultStr = ReplaceSubStr(wszNewSample,
                                      wszCurrencySep,
                                      L",");
        SendMessageW(GetDlgItem(hwndDlg, IDC_NEGCURRENCYSUM_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pwszResultStr);
        free(pwszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_NEGCURRENCYSUM_COMBO),
                           CB_SETCURSEL,
                           (WPARAM)_wtoi(wszCurrNegFmt),
                           (LPARAM)0);
}

/* Init currency separator control box */
VOID
InitCurrencySepCB(HWND hwndDlg)
{
    WCHAR wszCurrencySepSamples[MAX_CURRENCY_SEP_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L",",
        L"."
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszCurrencySep[MAX_SAMPLES_STR_SIZE];

    /* Get current currency separator */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SMONDECIMALSEP,
                   wszCurrencySep,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of currency separators */
    for(nCBIndex=0;nCBIndex<MAX_CURRENCY_SEP_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszCurrencySepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszCurrencySep);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                     CB_ADDSTRING,
                     MAX_CURRENCY_SEP_SAMPLES+1,
                     (LPARAM)wszCurrencySep);
        SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszCurrencySep);
    }
}

/* Init currency unit control box */
VOID
InitCurrencyUnitCB(HWND hwndDlg)
{
    WCHAR wszCurrencyUnitSamples[MAX_CURRENCY_UNIT_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"$"
    };

    int nCBIndex;
    int nRetCode;

    DWORD dwValueSize=MAX_SAMPLES_STR_SIZE*sizeof(WCHAR)+EOLN_SIZE;
    WCHAR wszCurrencyUnit[MAX_SAMPLES_STR_SIZE];

    /* Get current currency unit */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SCURRENCY,
                   wszCurrencyUnit,
                   dwValueSize);

    /* Clear all box content */
    SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                 CB_RESETCONTENT,
                 (WPARAM)0,
                 (LPARAM)0);

    /* Create standart list of currency units */
    for(nCBIndex=0;nCBIndex<MAX_CURRENCY_UNIT_SAMPLES;nCBIndex++)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)wszCurrencyUnitSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)wszCurrencyUnit);

    /* if is not success, add new value to list and select them */
    if(nRetCode == CB_ERR)
    {
        SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                     CB_ADDSTRING,
                     MAX_CURRENCY_UNIT_SAMPLES+1,
                     (LPARAM)wszCurrencyUnit);
        SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                     CB_SELECTSTRING,
                     -1,
                     (LPARAM)(LPCSTR)wszCurrencyUnit);
    }
}

/* Set number of digidts in field  */
BOOL
SetCurrencyDigNum(HWND hwndDlg)
{
    WCHAR wszFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES][MAX_SAMPLES_STR_SIZE]=
    {
        L"0;0",
        L"3;0",
        L"3;2;0"
    };

    int nCurrSel;

    /* Get setted number of digidts in field */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_DIGINFIELDNUM_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* Save number of digidts in field */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONGROUPING, wszFieldDigNumSamples[nCurrSel]);


    return TRUE;
}

/* Set currency field separator */
BOOL
SetCurrencyFieldSep(HWND hwndDlg)
{
    WCHAR wszCurrencyFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency field separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_FIELDSEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszCurrencyFieldSep);

    /* Save currency field separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONTHOUSANDSEP, wszCurrencyFieldSep);

    return TRUE;
}

/* Set number of fractional symbols */
BOOL
SetCurrencyFracSymNum(HWND hwndDlg)
{
    WCHAR wszCurrencyFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_FRACSYMBSNUM_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszCurrencyFracSymNum,DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICURRDIGITS, wszCurrencyFracSymNum);

    return TRUE;
}

/* Set currency separator */
BOOL
SetCurrencySep(HWND hwndDlg)
{
    WCHAR wszCurrencySep[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency decimal separator */
    SendMessageW(GetDlgItem(hwndDlg, IDC_WHOLEFRACTSEP_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszCurrencySep);

    /* TODO: Add check for correctly input */

    /* Save currency separator */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SMONDECIMALSEP, wszCurrencySep);

    return TRUE;
}

/* Set negative currency sum format */
BOOL
SetNegCurrencySumFmt(HWND hwndDlg)
{
    WCHAR wszNegCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_NEGCURRENCYSUM_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszNegCurrencySumFmt,DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGCURR, wszNegCurrencySumFmt);

    return TRUE;
}

/* Set positive currency sum format */
BOOL
SetPosCurrencySumFmt(HWND hwndDlg)
{
    WCHAR wszPosCurrencySumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted currency unit */
    nCurrSel=SendMessageW(GetDlgItem(hwndDlg, IDC_POSCURRENCYSUM_COMBO),
                          CB_GETCURSEL,
                          (WPARAM)0,
                          (LPARAM)0);

    /* convert to wide char */
    _itow(nCurrSel,wszPosCurrencySumFmt,DECIMAL_RADIX);

    /* Save currency sum format */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ICURRENCY, wszPosCurrencySumFmt);

    return TRUE;
}

/* Set currency unit */
BOOL
SetCurrencyUnit(HWND hwndDlg)
{
    WCHAR wszCurrencyUnit[MAX_SAMPLES_STR_SIZE];

    /* Get setted currency unit */
    SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCYUNIT_COMBO),
                 WM_GETTEXT,
                 (WPARAM)MAX_SAMPLES_STR_SIZE,
                 (LPARAM)(LPCSTR)wszCurrencyUnit);

    /* Save currency unit */
    SetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SCURRENCY, wszCurrencyUnit);

    return TRUE;
}

/* Update all currency locale samples */
static
VOID
UpdateCurrencyLocaleSamples(HWND hwndDlg,
                        LCID lcidLocale)
{
    WCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get currency format sample */
    GetCurrencyFormatW(lcidLocale,
                       LOCALE_USE_CP_ACP,
                       SAMPLE_NUMBER,
                       NULL,
                       OutBuffer,
                       MAX_FMT_SIZE);

    SendMessageW(GetDlgItem(hwndDlg, IDC_CURRENCY_EDIT),
                 WM_SETTEXT,
                 (WPARAM)0,
                 (LPARAM)OutBuffer);

    /* Get negative currency format sample */
    GetCurrencyFormatW(lcidLocale,
                       LOCALE_USE_CP_ACP,
                       SAMPLE_NEG_NUMBER,
                       NULL,
                       OutBuffer,
                       MAX_FMT_SIZE);

    SendMessageW(GetDlgItem(hwndDlg, IDC_NEGCURRENCY_EDIT),
                 WM_SETTEXT,
                 (WPARAM)0,
                 (LPARAM)OutBuffer);
}

/* Currency options setup page dialog callback */
INT_PTR
CALLBACK
CurrencyOptsSetProc(HWND hwndDlg,
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
            InitCurrencyUnitCB(hwndDlg);
            InitCurrencySepCB(hwndDlg);
            InitCurrencyFieldSepCB(hwndDlg);
            InitCurrencyFracNumCB(hwndDlg);
            InitPosCurrencySumCB(hwndDlg);
            InitNegCurrencySumCB(hwndDlg);
            InitCurrencyDigNumCB(hwndDlg);

            UpdateCurrencyLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_CURRENCYUNIT_COMBO:
                case IDC_POSCURRENCYSUM_COMBO:
                case IDC_NEGCURRENCYSUM_COMBO:
                case IDC_WHOLEFRACTSEP_COMBO:
                case IDC_FRACSYMBSNUM_COMBO:
                case IDC_FIELDSEP_COMBO:
                case IDC_DIGINFIELDNUM_COMBO:
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
                if(!SetCurrencyUnit(hwndDlg)) break;
                if(!SetPosCurrencySumFmt(hwndDlg)) break;
                if(!SetNegCurrencySumFmt(hwndDlg)) break;
                if(!SetCurrencySep(hwndDlg)) break;
                if(!SetCurrencyFracSymNum(hwndDlg)) break;
                if(!SetCurrencyFieldSep(hwndDlg)) break;
                if(!SetCurrencyDigNum(hwndDlg)) break;

                /* Update sum format samples */
                InitPosCurrencySumCB(hwndDlg);
                InitNegCurrencySumCB(hwndDlg);

                /* FIXME: */
                Sleep(15);
                UpdateCurrencyLocaleSamples(hwndDlg, LOCALE_USER_DEFAULT);
            }
        }
        break;
  }
  return FALSE;
}

/* EOF */
