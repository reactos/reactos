/*
 * PROJECT:         ReactOS International Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/intl/setupreg.c
 * PURPOSE:         ReactOS International Control Panel
 * PROGRAMMERS:     Alexey Zavyalov (gen_x@mail.ru)
*/

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

/* GLOBALS ******************************************************************/

#define NUM_SHEETS           4

/* FUNCTIONS ****************************************************************/

/* Insert the space  */
TCHAR*
InsSpacePos(const TCHAR *szInsStr, const int nPos)
{
    LPTSTR pszDestStr;
    int nDestStrCnt=0;
    int nStrCnt;
    int nStrSize;

    pszDestStr = (LPTSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(TCHAR));

    _tcscpy(pszDestStr, szInsStr);

    nStrSize = _tcslen(szInsStr);

    for (nStrCnt = 0; nStrCnt < nStrSize; nStrCnt++)
    {
        if (nStrCnt == nStrSize - nPos)
        {
            pszDestStr[nDestStrCnt] = _T(' ');
            nDestStrCnt++;
        }

        pszDestStr[nDestStrCnt] = szInsStr[nStrCnt];
        nDestStrCnt++;
    }

    pszDestStr[nDestStrCnt] = _T('\0');

    return pszDestStr;
}

/* Insert the spaces by format string separated by ';' */
LPTSTR
InsSpacesFmt(const TCHAR *szSourceStr, const TCHAR *szFmtStr)
{
    LPTSTR pszDestStr;
    LPTSTR pszTempStr;
    TCHAR szFmtVal[255];
    int nFmtCount=0;
    int nValCount=0;
    int nLastVal=0;
    int nSpaceOffset=0;
    BOOL wasNul=FALSE;

    pszDestStr = (LPTSTR) malloc(255 * sizeof(TCHAR));

    _tcscpy(pszDestStr, szSourceStr);

    /* if format is clean return source string */
    if (!*szFmtStr)
        return pszDestStr;

    /* Search for all format values */
    for (nFmtCount = 0; nFmtCount <= (int)_tcslen(szFmtStr); nFmtCount++)
    {
        if (szFmtStr[nFmtCount] == _T(';') || szFmtStr[nFmtCount] == _T('\0'))
        {
            if (_ttoi(szFmtVal) == 0 && !wasNul)
            {
                wasNul = TRUE;
                break;
            }

            /* If was 0, repeat spaces */
            if (wasNul)
            {
                nSpaceOffset += nLastVal;
            }
            else
            {
                nSpaceOffset += _ttoi(szFmtVal);
            }

            szFmtVal[nValCount] = _T('\0');
            nValCount=0;

            /* insert space to finded position plus all pos before */
            pszTempStr = InsSpacePos(pszDestStr, nSpaceOffset);
            _tcscpy(pszDestStr, pszTempStr);
            free(pszTempStr);

            /* num of spaces total increment */
            if (!wasNul)
            {
                nSpaceOffset++;
                nLastVal = _ttoi(szFmtVal);
            }
        }
        else
        {
            szFmtVal[nValCount++] = szFmtStr[nFmtCount];
        }
    }

    /* Create spaces for rest part of string */
    if (wasNul && nLastVal!=0)
    {
        for (nFmtCount = nSpaceOffset + nLastVal; nFmtCount < _tcslen(pszDestStr); nFmtCount += nLastVal + 1)
        {
            pszTempStr = InsSpacePos(pszDestStr, nFmtCount);
            _tcscpy(pszDestStr,pszTempStr);
            free(pszTempStr);
        }
    }

    return pszDestStr;
}

/* Replace given template in source string with string to replace and return recieved string */
TCHAR*
ReplaceSubStr(const TCHAR *szSourceStr,
              const TCHAR *szStrToReplace,
              const TCHAR *szTempl)
{
    int nCharCnt;
    int nSubStrCnt;
    int nDestStrCnt;
    int nFirstCharCnt;
    LPTSTR szDestStr;

    szDestStr = (LPTSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(TCHAR));
    nDestStrCnt = 0;
    nFirstCharCnt = 0;

    _tcscpy(szDestStr, _T(L""));

    while (nFirstCharCnt < (int)_tcslen(szSourceStr))
    {
        if (szSourceStr[nFirstCharCnt] == szTempl[0])
        {
            nSubStrCnt=0;
            for (nCharCnt = nFirstCharCnt; nCharCnt < nFirstCharCnt + (int)_tcslen(szTempl); nCharCnt++)
            {
                if (szSourceStr[nCharCnt] == szTempl[nSubStrCnt])
                {
                    nSubStrCnt++;
                }
                else
                {
                    break;
                }

                if ((int)_tcslen(szTempl) == nSubStrCnt)
                {
                    _tcscat(szDestStr, szStrToReplace);
                    nDestStrCnt = (int)_tcslen(szDestStr);
                    nFirstCharCnt += (int)_tcslen(szTempl) - 1;
                    break;
                }
            }
        }
        else
        {
            szDestStr[nDestStrCnt++] = wszSourceStr[nFirstCharCnt];
            szDestStr[nDestStrCnt] = _T('\0');
        }

        nFirstCharCnt++;
    }

    return szDestStr;
}

static
VOID
InitPropSheetPage(PROPSHEETPAGE *PsPage, WORD IdDlg, DLGPROC DlgProc)
{
    ZeroMemory(PsPage, sizeof(PROPSHEETPAGE));
    PsPage->dwSize = sizeof(PROPSHEETPAGE);
    PsPage->dwFlags = PSP_DEFAULT;
    PsPage->hInstance = hApplet;
    PsPage->pszTemplate = MAKEINTRESOURCE(IdDlg);
    PsPage->pfnDlgProc = DlgProc;
}

/* Create applets */
LONG
APIENTRY
SetupApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{

    PROPSHEETPAGE PsPage[NUM_SHEETS];
    PROPSHEETHEADER psh;
    TCHAR Caption[MAX_STR_SIZE];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(PsPage) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = PsPage;

    InitPropSheetPage(&PsPage[0], IDD_NUMSOPTSSETUP, NumsOptsSetProc);
    InitPropSheetPage(&PsPage[1], IDD_CURRENCYOPTSSETUP, CurrencyOptsSetProc);
    InitPropSheetPage(&PsPage[2], IDD_TIMEOPTSSETUP, TimeOptsSetProc);
    InitPropSheetPage(&PsPage[3], IDD_DATEOPTSSETUP, DateOptsSetProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
