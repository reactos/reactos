#include "intl.h"

#define NUM_SHEETS           4

/* Insert the space  */
PWSTR
InsSpacePos(PCWSTR szInsStr, const int nPos)
{
    PWSTR pszDestStr;
    INT nDestStrCnt = 0;
    INT nStrCnt;
    INT nStrSize;

    pszDestStr = (PWSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    wcscpy(pszDestStr, szInsStr);

    nStrSize = wcslen(szInsStr);

    for (nStrCnt = 0; nStrCnt < nStrSize; nStrCnt++)
    {
        if (nStrCnt == nStrSize - nPos)
        {
            pszDestStr[nDestStrCnt] = L' ';
            nDestStrCnt++;
        }

        pszDestStr[nDestStrCnt] = szInsStr[nStrCnt];
        nDestStrCnt++;
    }

    pszDestStr[nDestStrCnt] = L'\0';

    return pszDestStr;
}

/* Insert the spaces by format string separated by ';' */
PWSTR
InsSpacesFmt(PCWSTR szSourceStr, PCWSTR szFmtStr)
{
    PWSTR pszDestStr;
    PWSTR pszTempStr;
    WCHAR szFmtVal[255];
    UINT nFmtCount = 0;
    INT nValCount = 0;
    INT nLastVal = 0;
    INT nSpaceOffset = 0;
    BOOL wasNul=FALSE;

    pszDestStr = (PWSTR)malloc(255 * sizeof(WCHAR));

    wcscpy(pszDestStr, szSourceStr);

    /* If format is clean return source string */
    if (!*szFmtStr)
        return pszDestStr;

    /* Search for all format values */
    for (nFmtCount = 0; nFmtCount <= wcslen(szFmtStr); nFmtCount++)
    {
        if (szFmtStr[nFmtCount] == L';' || szFmtStr[nFmtCount] == L'\0')
        {
            if (_wtoi(szFmtVal) == 0 && !wasNul)
            {
                wasNul=TRUE;
                break;
            }

            /* If was 0, repeat spaces */
            if (wasNul)
            {
                nSpaceOffset += nLastVal;
            }
            else
            {
                nSpaceOffset += _wtoi(szFmtVal);
            }

            szFmtVal[nValCount] = L'\0';
            nValCount=0;

            /* Insert space to finded position plus all pos before */
            pszTempStr = InsSpacePos(pszDestStr, nSpaceOffset);
            wcscpy(pszDestStr,pszTempStr);
            free(pszTempStr);

            /* Num of spaces total increment */
            if (!wasNul)
            {
                nSpaceOffset++;
                nLastVal = _wtoi(szFmtVal);
            }
        }
        else
        {
            szFmtVal[nValCount++] = szFmtStr[nFmtCount];
        }
    }

    /* Create spaces for rest part of string */
    if (wasNul && nLastVal != 0)
    {
        for (nFmtCount = nSpaceOffset + nLastVal; nFmtCount < wcslen(pszDestStr); nFmtCount += nLastVal + 1)
        {
            pszTempStr = InsSpacePos(pszDestStr, nFmtCount);
            wcscpy(pszDestStr, pszTempStr);
            free(pszTempStr);
        }
    }

    return pszDestStr;
}

/* Replace given template in source string with string to replace and return received string */
PWSTR
ReplaceSubStr(PCWSTR szSourceStr,
              PCWSTR szStrToReplace,
              PCWSTR szTempl)
{
    PWSTR szDestStr;
    UINT nCharCnt;
    UINT nSubStrCnt;
    UINT nDestStrCnt;
    UINT nFirstCharCnt;

    szDestStr = (PWSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));

    nDestStrCnt = 0;
    nFirstCharCnt = 0;

    wcscpy(szDestStr, L"");

    while (nFirstCharCnt < wcslen(szSourceStr))
    {
        if (szSourceStr[nFirstCharCnt] == szTempl[0])
        {
            nSubStrCnt = 0;
            for (nCharCnt = nFirstCharCnt; nCharCnt < nFirstCharCnt + wcslen(szTempl); nCharCnt++)
            {
                if (szSourceStr[nCharCnt] == szTempl[nSubStrCnt])
                {
                    nSubStrCnt++;
                }
                else
                {
                    break;
                }

                if (wcslen(szTempl) == nSubStrCnt)
                {
                    wcscat(szDestStr, szStrToReplace);
                    nDestStrCnt = wcslen(szDestStr);
                    nFirstCharCnt += wcslen(szTempl) - 1;
                    break;
                }
            }
        }
        else
        {
            szDestStr[nDestStrCnt++] = szSourceStr[nFirstCharCnt];
            szDestStr[nDestStrCnt] = L'\0';
        }

        nFirstCharCnt++;
    }

    return szDestStr;
}


static VOID
InitPropSheetPage(PROPSHEETPAGEW *psp, WORD idDlg, DLGPROC DlgProc, PGLOBALDATA pGlobalData)
{
  ZeroMemory(psp, sizeof(PROPSHEETPAGEW));
  psp->dwSize = sizeof(PROPSHEETPAGEW);
  psp->dwFlags = PSP_DEFAULT;
  psp->hInstance = hApplet;
  psp->pszTemplate = MAKEINTRESOURCE(idDlg);
  psp->pfnDlgProc = DlgProc;
  psp->lParam = (LPARAM)pGlobalData;
}


/* Create applets */
LONG
APIENTRY
SetupApplet(
    HWND hwndDlg,
    PGLOBALDATA pGlobalData)
{
    PROPSHEETPAGEW PsPage[NUM_SHEETS + 1];
    PROPSHEETHEADERW psh;
    WCHAR Caption[MAX_STR_SIZE];
    INT_PTR ret;

    LoadStringW(hApplet, IDS_CUSTOMIZE_TITLE, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    psh.hwndParent = hwndDlg;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = (sizeof(PsPage) / sizeof(PROPSHEETPAGE)) - 1;
    psh.nStartPage = 0;
    psh.ppsp = PsPage;

    InitPropSheetPage(&PsPage[0], IDD_NUMBERSPAGE, NumbersPageProc, pGlobalData);
    InitPropSheetPage(&PsPage[1], IDD_CURRENCYPAGE, CurrencyPageProc, pGlobalData);
    InitPropSheetPage(&PsPage[2], IDD_TIMEPAGE, TimePageProc, pGlobalData);
    InitPropSheetPage(&PsPage[3], IDD_DATEPAGE, DatePageProc, pGlobalData);

    if (IsSortPageNeeded(pGlobalData->lcid))
    {
        psh.nPages++;
        InitPropSheetPage(&PsPage[4], IDD_SORTPAGE, SortPageProc, pGlobalData);
    }

    ret = PropertySheetW(&psh);

    return (LONG)(ret != -1);
}

/* EOF */
