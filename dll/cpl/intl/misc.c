#include "intl.h"

/* Insert the space  */
PWSTR
InsSpacePos(PCWSTR szInsStr, const int nPos)
{
    PWSTR pszDestStr;
    INT nDestStrCnt = 0;
    INT nStrCnt;
    INT nStrSize;

    pszDestStr = (PWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));
    if (pszDestStr == NULL)
        return NULL;

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

    pszDestStr = (PWSTR)HeapAlloc(GetProcessHeap(), 0, 255 * sizeof(WCHAR));
    if (pszDestStr == NULL)
        return NULL;

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
            HeapFree(GetProcessHeap(), 0, pszTempStr);

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
            HeapFree(GetProcessHeap(), 0, pszTempStr);
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

    szDestStr = (PWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_SAMPLES_STR_SIZE * sizeof(WCHAR));
    if (szDestStr == NULL)
        return NULL;

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


VOID
GetSelectedComboBoxText(
    HWND hwndDlg,
    INT nIdDlgItem,
    PWSTR Buffer,
    UINT uSize)
{
    HWND hChildWnd;
    PWSTR tmp;
    INT nIndex;
    UINT uReqSize;

    /* Get handle to time format control */
    hChildWnd = GetDlgItem(hwndDlg, nIdDlgItem);
    if (hChildWnd == NULL)
        return;

    /* Get index to selected time format */
    nIndex = SendMessageW(hChildWnd, CB_GETCURSEL, 0, 0);
    if (nIndex == CB_ERR)
    {
        /* No selection? Get content of the edit control */
        SendMessageW(hChildWnd, WM_GETTEXT, uSize, (LPARAM)Buffer);
    }
    else
    {
        /* Get requested size, including the null terminator;
         * it shouldn't be required because the previous CB_LIMITTEXT,
         * but it would be better to check it anyways */
        uReqSize = SendMessageW(hChildWnd, CB_GETLBTEXTLEN, (WPARAM)nIndex, 0) + 1;

        /* Allocate enough space to be more safe */
        tmp = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, uReqSize * sizeof(WCHAR));
        if (tmp != NULL)
        {
            /* Get selected time format text */
            SendMessageW(hChildWnd, CB_GETLBTEXT, (WPARAM)nIndex, (LPARAM)tmp);

            /* Finally, copy the result into the output */
            wcsncpy(Buffer, tmp, uSize);

            HeapFree(GetProcessHeap(), 0, tmp);
        }
    }
}


VOID
GetSelectedComboBoxIndex(
    HWND hwndDlg,
    INT nIdDlgItem,
    PINT pValue)
{
    *pValue = SendDlgItemMessageW(hwndDlg, nIdDlgItem, CB_GETCURSEL, 0, 0);
}

/* EOF */
