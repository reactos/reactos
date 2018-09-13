/****************************************************************************

    CodePage.c

    PURPOSE : Codepage related utilities for Universal Character Explorer

    Copyright (c) 1997-1999 Microsoft Corporation.
****************************************************************************/

#include "windows.h"
#include "commctrl.h"

#include "UCE.h"
#include "stdlib.h"
#include "tchar.h"
#include "stdio.h"
#include "winuser.h"
#include "string.h"
#include "search.h"
#include "getuname.h"

#include "winnls.h"
#include "wingdi.h"

BOOL CodePage_AddToList(LONG);

LONG *CodePage_pList       = NULL;
INT  CodePage_nCount       = 0;
LONG CodePage_lCurCodePage = 0;

/****************************************************************************

    Function : Enumerate code page callback function.

****************************************************************************/
BOOL
CALLBACK EnumCodePagesProc(
    LPTSTR lpCodePageString
    )
{
    LONG  lCodePageNum;
    TCHAR szBuf[256];

    lCodePageNum = _wtol(lpCodePageString);

    if(!IsValidCodePage(lCodePageNum))
    {
        return TRUE;
    }
    if(LoadString(hInst, lCodePageNum, szBuf, sizeof(szBuf)) == 0)
    {
        return TRUE;
    }

    CodePage_AddToList(lCodePageNum);
    return TRUE;
}

/****************************************************************************

    Function : Delete this list.

****************************************************************************/
BOOL
CodePage_DeleteList()
{
    free(CodePage_pList);
    CodePage_pList  = NULL;
    CodePage_nCount = 0;
    return TRUE;
}

/****************************************************************************

    Function : initialize list

****************************************************************************/
BOOL
CodePage_InitList()
{
    if (CodePage_pList)
    {
        CodePage_DeleteList();
    }

    CodePage_pList  = NULL;
    CodePage_nCount = 0;

    //
    // we need 1200 (Unicode codepage) that EnumSystemCodePages won't give us
    //
    CodePage_AddToList(UNICODE_CODEPAGE);

    EnumSystemCodePages((CODEPAGE_ENUMPROC)EnumCodePagesProc, CP_INSTALLED);

    return TRUE;
}

/***************************************************************************

    Function : Add a code page value to list.

****************************************************************************/
BOOL
CodePage_AddToList(
    LONG lCodePage
    )
{
    int   i, j;
    WCHAR wcBuf[256];
    WCHAR wcBufNew[256];


    if (CodePage_pList == NULL)
    {
        CodePage_pList = (LONG *) malloc(sizeof(LONG));
        if (CodePage_pList == NULL)
        {
            return FALSE;
        }
        CodePage_pList[CodePage_nCount] = lCodePage;
    }
    else
    {
        CodePage_pList = (LONG *) realloc(CodePage_pList,
                                          sizeof(LONG)*(CodePage_nCount+1));
        if (CodePage_pList == NULL)
        {
            return FALSE;
        }

        LoadString(hInst, lCodePage, wcBufNew, sizeof(wcBufNew));
        for(i = 1; i < CodePage_nCount; i++)
        {
            LoadString(hInst, CodePage_pList[i], wcBuf, sizeof(wcBuf));
            if(CompareString(LOCALE_USER_DEFAULT, 0, wcBufNew, -1,  wcBuf, -1) == 1) break;
        }

        for(j = CodePage_nCount; j > i; j--)
        {
            CodePage_pList[j] = CodePage_pList[j-1];
        }
        CodePage_pList[i] = lCodePage;
    }

    CodePage_nCount++;
    return TRUE;
}

/****************************************************************************

    Function : Fill a combobox with code pages.

****************************************************************************/
BOOL
CodePage_FillToComboBox(
    HWND hWnd,
    UINT uID
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  i;

    if (hCombo == NULL)
    {
        return FALSE;
    }

    SendMessage(
        hCombo,
        CB_RESETCONTENT,
        0,
        0 );

    for (i=0; i < CodePage_nCount; i++)
    {
       int   nIndex;
       TCHAR szBuf[256];

       if (CodePage_pList[i] == UNICODE_CODEPAGE)
       {
           LoadString(hInst,IDS_UNICODE,szBuf,sizeof(szBuf));
           nIndex = (int)SendMessage(
                        hCombo,
                        CB_ADDSTRING,
                        (WPARAM) 0,
                        (LPARAM) szBuf);
           if(nIndex != CB_ERR)
           {
               SendMessage(
                   hCombo,
                   CB_SETITEMDATA,
                   (WPARAM) nIndex,
                   (LPARAM) CodePage_pList[i]);
           }
       }
       else
       {
           LoadString(hInst, CodePage_pList[i], szBuf, sizeof(szBuf));
           SendMessage(
               hCombo,
               CB_ADDSTRING,
               (WPARAM) 0,
               (LPARAM) szBuf);
       }
    }

    return TRUE;
}

/****************************************************************************

    Function : Get current selected codepage

/****************************************************************************/
LONG CodePage_GetCurSelCodePage(
    HWND hWnd,
    UINT uID
    )
{
    HWND hCombo = (HWND) GetDlgItem(hWnd,uID);
    INT  nIndex;
    LONG lCodePage;

    if (hCombo == NULL)
    {
        return 0;
    }

    nIndex = (INT)SendMessage(
                hCombo,
                CB_GETCURSEL,
                (WPARAM) 0,
                (LPARAM) 0L);

    if (nIndex == CB_ERR)
    {
        return 0L;
    }

    lCodePage = CodePage_pList[nIndex];

    CodePage_lCurCodePage = lCodePage;
    return lCodePage;
}

/****************************************************************************

    Function : get current codepage value

/****************************************************************************/
LONG CodePage_GetCurCodePageVal()
{
   return CodePage_lCurCodePage;
}


/****************************************************************************

    Function : Set current codepage

/****************************************************************************/
// Set to this code page, if possible and return result of operation
BOOL CodePage_SetCurrent( LONG lCodePage , HWND hWnd , UINT uID )
{
  INT   i;
  HWND  hCombo = (HWND) GetDlgItem(hWnd,uID);
  DWORD dwResult;
  BOOL  bRet=FALSE;



  for( i=0; i<CodePage_nCount; i++)
  {
    if (CodePage_pList[i] == lCodePage)  // got a match
    {
      dwResult = (DWORD)SendMessage( hCombo,
                                     CB_SETCURSEL,
                                     (WPARAM) i,
                                     (LPARAM) 0L);
      if( CB_ERR != dwResult )
          bRet=TRUE;
    }
  }

  return bRet;
}

/****************************************************************************

    Function : Is this codepage on our list ?

/****************************************************************************/
BOOL
IsCodePageOnList(
    WORD wCodePage
    )
{
    int i = CodePage_nCount;

    while(i--)
    {
      if(CodePage_pList[i] == wCodePage)
          return TRUE;
    }

    return FALSE;
}
