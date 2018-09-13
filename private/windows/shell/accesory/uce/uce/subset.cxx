/****************************************************************************

    Subset.c

    PURPOSE: Subset related utilities for UCE

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

#include "ucefile.h"

extern DWORD gFontType;

//
// Char buffer
//
WCHAR wcUnicodeBuffer[M64K];

/****************************************************************************

    Fill Subsets from UCE files

****************************************************************************/
BOOL
Subset_FillComboBox(
   HWND hWnd,
   UINT uID        )
{
  INT              i=0;
  INT              nIndex;
  PUCE_MEMORY_FILE pUceMemFile;
  INT              cFiles;
  PWSTR            pwszSubsetName;
  WCHAR            wszID[16];              // I know it is "ALL"
  HWND             hCombo = (HWND) GetDlgItem(hWnd,uID);

  if(NULL == hCombo)
  {
    return FALSE;
  }

  SendMessage( hCombo,
               CB_RESETCONTENT,
               0,
               0
             );

  // Let's add the All subset (hardcoded)
  LoadString( hInst , IDS_ALL , &wszID[0] , sizeof(wszID)-1 );
  if( wszID[0] )
  {
    wszID[sizeof(wszID)-1] = 0;  // safe!
    nIndex = (INT)SendMessage( hCombo,
                               CB_ADDSTRING,
                               (WPARAM) 0,
                               (LPARAM)(PCWSTR) &wszID[0]
                             );

    if (nIndex != CB_ERR)
    {
      SendMessage( hCombo,
                   CB_SETITEMDATA,
                   (WPARAM) nIndex,
                   (LPARAM) 0
                 );
    }

    // set the current selection here
    SendMessage( hCombo,
                 CB_SETCURSEL,
                 (WPARAM) nIndex,
                 (LPARAM) 0
               );

  }


  cFiles = UCE_GetFiles( &pUceMemFile );

  while( i<cFiles )
  {
    WORD cp;
    BOOL IsCpOnList;

    // make sure required codepage is on our list

    cp = UCE_GetCodepage( &pUceMemFile[i] );
    if(cp == UNICODE_CODEPAGE)
    {
        IsCpOnList = TRUE;
    }
    else
    {
        IsCpOnList = IsCodePageOnList(cp);
    }

    if( IsCpOnList )
    {
      WCHAR wcBuf[256];

      UCE_GetTableName( &pUceMemFile[i] , &pwszSubsetName );
      if(*pwszSubsetName == L'0')
      {
          LoadString(hInst, _wtol(pwszSubsetName), wcBuf, 255);
          pwszSubsetName = wcBuf;
      }
      nIndex = (INT)SendMessage( hCombo,
                                 CB_ADDSTRING,
                                 (WPARAM) 0,
                                 (LPARAM) pwszSubsetName
                                );

      if (nIndex != CB_ERR)
      {
        SendMessage( hCombo,
                     CB_SETITEMDATA,
                     (WPARAM) nIndex,
                     (LPARAM)(PUCE_MEMORY_FILE) &pUceMemFile[i]
                   );
      }
    }

    i++;
  }

  return TRUE;

}

/****************************************************************************

    Read the current UCE pointer

****************************************************************************/
BOOL
GetCurrentUCEFile(
    PUCE_MEMORY_FILE *ppUceMemFile,
    HWND   hWnd,
    UINT   uID   )
{
  INT  nIndex;
  BOOL bRet = FALSE;

  nIndex = (INT)SendDlgItemMessage( hWnd,
                                    uID,
                                    CB_GETCURSEL,
                                    (WPARAM) 0,
                                    (LPARAM) 0L
                                  );

  if( nIndex != CB_ERR )
  {
    *ppUceMemFile = (PUCE_MEMORY_FILE) SendDlgItemMessage( hWnd,
                                                           uID,
                                                           CB_GETITEMDATA,
                                                           (WPARAM) nIndex,
                                                           (LPARAM) 0L
                                                         );
    bRet = TRUE;
  }

  return bRet;
}

/****************************************************************************

    Get a Unicode buffer for the current view/subset when subset is all

    Called to retreive what to display

****************************************************************************/
BOOL
Subset_GetUnicodeCharsToDisplay(
HWND   hWnd,
UINT   uID,
LONG   lCodePage,
PWSTR *ppCodeList,
UINT  *puNum,
BOOL  *pbLineBreak)
{
  PUCE_MEMORY_FILE pUceMemFile= NULL;
  HWND             hCombo     = (HWND) GetDlgItem(hWnd,uID);

  if(NULL == hCombo)
  {
    return FALSE;
  }

  // Get the Unicode of the current code page
  if( !GetCurrentUCEFile( &pUceMemFile , hWnd ,uID ) || (NULL == pUceMemFile) )
  {
    return GetUnicodeBufferOfCodePage( lCodePage , ppCodeList , puNum );
  }
  else
  {
    if( pUceMemFile )
    {
      // Let's retreive the Unicode subset from the list
      // and throw it back to the display

      if( !Subset_GetUnicode(    // if fail, then select as if All is selected
                    hWnd,
                    pUceMemFile,
                    ppCodeList,
                    puNum,
                    pbLineBreak ))
        return GetUnicodeBufferOfCodePage( lCodePage , ppCodeList , puNum );
    }
  }

  return TRUE;
}

/****************************************************************************

  Convert a codepage to unicode buffer

****************************************************************************/
BOOL
GetUnicodeBufferOfCodePage(
        LONG   lCodePage,
        PWSTR *ppwBuf,
        UINT  *puNum )
{
  *puNum  = WCharCP( lCodePage, wcUnicodeBuffer);
  *ppwBuf = wcUnicodeBuffer ;

  return (*puNum);
}

/****************************************************************************

   Decide whether to retreive unicode from 1-d or 2-d current selection

****************************************************************************/
BOOL
Subset_GetUnicode(
        HWND hWnd,
        PUCE_MEMORY_FILE pUceMemFile,
        PWSTR *ppwBuf,
        UINT *puNum,
        BOOL *pbLineBreak )
{
  BOOL bRet = FALSE;

  switch( ((PUCE_HEADER)(pUceMemFile->pvData))->Row )
  {
  case 0:           // 1-d array
    {
      // should delete grid window if any

      bRet = GetUnicodeCharsFromList(
                                     hWnd,
                                     pUceMemFile,
                                     &wcUnicodeBuffer[0],
                                     puNum,
                                     pbLineBreak
                                    );
    }
    break;

  default:          // 2-d array
    {
      // should delete list window, if any
      bRet = GetUnicodeCharsFromGridWindow(
                                           hWnd,
                                           &wcUnicodeBuffer[0],
                                           puNum,
                                           pbLineBreak
                                          );                 // till now
    }
    break;

  }

  // Update user buffer
  *ppwBuf = &wcUnicodeBuffer[0];

  return bRet;
}


/****************************************************************************

    When user selchange a subset, let's call our guys (if really changed)
    to bring up a new list (if required)
    must call SubSetChanged(...) after this

****************************************************************************/
BOOL
Subset_OnSelChange(
    HWND hWnd,
    UINT uID )
{
  static PUCE_MEMORY_FILE pLastUceMemFile = NULL;
  PUCE_MEMORY_FILE        pCurUceMemFile;
  PSTR                    pFile;
  PUCE_HEADER             pHeader;

  DWORD                   dw;
  WCHAR                  *pWC;

  // Let's know if it changed really
  if( !GetCurrentUCEFile( &pCurUceMemFile , hWnd , uID ) ||
      (NULL == pCurUceMemFile)
    )
  {
    pLastUceMemFile = pCurUceMemFile;
    DestroyAllListWindows();
    return FALSE;
  }

  // Need to check also if the window list is available
  if( (pLastUceMemFile == pCurUceMemFile) && IsListWindow( pCurUceMemFile ) )
    return FALSE;

  // If so, let's update our list view or grid
  pFile = (PSTR) pCurUceMemFile->pvData;
  pHeader  = (PUCE_HEADER) pCurUceMemFile->pvData;
  pLastUceMemFile = pCurUceMemFile;

  dw  = *(((DWORD*)pCurUceMemFile->pvData)+1);
  pWC = (WCHAR*)(((BYTE*)pCurUceMemFile->pvData)+dw);
  if(lstrcmp(pWC, L"010200") == 0)                             // Ideograf.UCE
  {
     UINT CharSet;

     if(!(gFontType & DBCS_FONTTYPE) &&
        (CharSet = Font_DBCS_CharSet()) != 0)
     {
         Font_SelectByCharSet(hWnd, ID_FONT, CharSet);
     }
  }
  else
  {
      CHARSETINFO csi;
      CPINFO      cpinfo;

      // Indicate change of view to a new font
      GetCPInfo(pHeader->Codepage, &cpinfo);
      if(cpinfo.MaxCharSize > 1 &&
         TranslateCharsetInfo((DWORD*)pHeader->Codepage, &csi, TCI_SRCCODEPAGE))
      {
          Font_SelectByCharSet(hWnd,ID_FONT, csi.ciCharset);
      }
  }

  // Let's fetch the most appropriate code page
  if( CodePage_SetCurrent( pHeader->Codepage , hWnd , ID_VIEW ) )
  {
      PostMessage( hWnd ,
                   WM_COMMAND,
                   MAKELONG(ID_VIEW,CBN_SELCHANGE),
                   0L );
  }

  switch( pHeader->Row )
  {
  case 0:
    // Destroy grid if needed
    DestroyGridWindow();
    CreateListWindow( hWnd , (PWSTR)(pFile+pHeader->OffsetTableName) ) ;
    FillGroupsInListBox( hWnd , pCurUceMemFile );
    break;

  default:
    DestroyListWindow();
    DestroyGridWindow();
    CreateGridWindow( hWnd , ID_FONT , pCurUceMemFile );
    // create grid if needed
    break;
  }

  EnableWindow(GetDlgItem(hwndDialog, ID_VIEW), FALSE);

  return TRUE;
}

