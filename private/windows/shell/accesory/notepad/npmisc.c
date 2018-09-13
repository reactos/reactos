/*
 * misc functions
 *  Copyright (C) 1984-1995 Microsoft Inc.
 */

#include "precomp.h"

BOOL fCase = FALSE;         /* Flag specifying case sensitive search */
BOOL fReverse = FALSE;      /* Flag for direction of search */

extern HWND hDlgFind;       /* handle to modeless FindText window */

LPTSTR ReverseScan(
    LPTSTR lpSource,
    LPTSTR lpLast,
    LPTSTR lpSearch,
    BOOL fCaseSensitive )
{
   TCHAR cLastCharU;
   TCHAR cLastCharL;
   INT   iLen;

   cLastCharU= (TCHAR) (INT_PTR) CharUpper( (LPTSTR)(INT_PTR)(*lpSearch) );
   cLastCharL= (TCHAR) (INT_PTR) CharLower( (LPTSTR)(INT_PTR)(*lpSearch) );

   iLen = lstrlen(lpSearch);

   if (!lpLast)
      lpLast = lpSource + lstrlen(lpSource);

   do
   {
      if (lpLast == lpSource)
         return NULL;

      --lpLast;

      if (fCaseSensitive)
      {
         if (*lpLast != *lpSearch)
            continue;
      }
      else
      {
           if( !( *lpLast == cLastCharU || *lpLast == cLastCharL ) )
            continue;
      }

      if (fCaseSensitive)
      {
         if (!_tcsncmp( lpLast, lpSearch, iLen))
            break;
      }
      else
      {
         //
         // compare whole string using locale specific comparison.
         // do not use C runtime version since it may be wrong.
         //

         if( 2 == CompareString( LOCALE_USER_DEFAULT,
                    NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
                    lpLast,   iLen,
                    lpSearch, iLen) )
            break;
      }
   } while (TRUE);

   return lpLast;
}

LPTSTR ForwardScan(LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive )
{
   TCHAR cFirstCharU;
   TCHAR cFirstCharL;
   int iLen = lstrlen(lpSearch);

   cFirstCharU= (TCHAR) (INT_PTR) CharUpper( (LPTSTR)(INT_PTR)(*lpSearch) );
   cFirstCharL= (TCHAR) (INT_PTR) CharLower( (LPTSTR)(INT_PTR)(*lpSearch) );

   while (*lpSource)
   {
      if (fCaseSensitive)
      {
         if (*lpSource != *lpSearch)
         {
            lpSource++;
            continue;
         }
      }
      else
      {
         if( !( *lpSource == cFirstCharU || *lpSource == cFirstCharL ) )
         {
            lpSource++;
            continue;
         }
      }

      if (fCaseSensitive)
      {
         if (!_tcsncmp( lpSource, lpSearch, iLen))
            break;
      }
      else
      {
         if( 2 == CompareString( LOCALE_USER_DEFAULT,
                    NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
                    lpSource, iLen,
                    lpSearch, iLen) )
            break;
      }

      lpSource++;
   }

   return *lpSource ? lpSource : NULL;
}


/* search forward or backward in the edit control text for the given pattern */
/* It is the responsibility of the caller to set the cursor                  */

BOOL Search (TCHAR * szKey)
{
    BOOL      bStatus= FALSE;
    TCHAR   * pStart, *pMatch;
    DWORD     StartIndex, LineNum, EndIndex;
    DWORD     SelStart, SelEnd, i;
    HANDLE    hEText;           // handle to edit text
    UINT      uSelState;
    HMENU     hMenu;
    BOOL      bSelectAll = FALSE;


    if (!*szKey)
        return( bStatus );

    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&SelStart, (LPARAM)&SelEnd);


    /*
    when we finish the search, we highlight the text found, and continue 
    the search after the end of the highlighted position (in forward 
    case) or from the begining of the highlighted position in the reverse
    direction (in reverse case). this would break if the user has 
    selected all text. this hack would take care of it. (this is consistent
    with VC editors' search too.*/

    hMenu = GetMenu(hwndNP);
    uSelState = GetMenuState(GetSubMenu(hMenu, 1), M_SELECTALL, MF_BYCOMMAND);
    if (uSelState == MF_GRAYED)
    {
        bSelectAll = TRUE;
        SelStart = SelEnd =0;
    }


    /*
     * get pointer to edit control text to search
     */

    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hEText )  // silently return if we can't get it
    {
        return( bStatus );
    }
    pStart= LocalLock( hEText );
    if( !pStart )
    {
        return( bStatus );
    }

    if (fReverse)
    {
        /* Get current line number */
        LineNum= (DWORD)SendMessage(hwndEdit, EM_LINEFROMCHAR, SelStart, 0);
        /* Get index to start of the line */
        StartIndex= (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, LineNum, 0);
        /* Set upper limit for search text */
        EndIndex= SelStart;
        pMatch= NULL;

        /* Search line by line, from LineNum to 0 */
        i = LineNum;
        while (TRUE)
        {
            pMatch= ReverseScan(pStart+StartIndex,pStart+EndIndex,szKey,fCase);
            if (pMatch)
               break;
            /* current StartIndex is the upper limit for the next search */
            EndIndex= StartIndex;

            if (i)
            {
                /* Get start of the next line */
                i-- ;
                StartIndex = (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, i, 0);
            }
            else
               break ;
        }
    }
    else
    {
            pMatch= ForwardScan(pStart+SelEnd, szKey, fCase);
    }

    LocalUnlock(hEText);

    if (pMatch == NULL)
    {
        //
        // alert user on not finding any text unless it is replace all
        //
        if( !(FR.Flags & FR_REPLACEALL) )
        {
            HANDLE hPrevCursor= SetCursor( hStdCursor );
            AlertBox( hDlgFind ? hDlgFind : hwndNP,
                      szNN,
                      szCFS,
                      szSearch,
                      MB_APPLMODAL | MB_OK | MB_ICONASTERISK);
            SetCursor( hPrevCursor );
        }
    }
    else
    {
        SelStart = (DWORD)(pMatch - pStart);
        SendMessage( hwndEdit, EM_SETSEL, SelStart, SelStart+lstrlen(szKey));

        // since we are selecting the found text, enable SelectAll again.
        if (bSelectAll)
        {
            EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_ENABLED);
        }

        //
        // show the selected text unless it is replace all
        //

        if( !(FR.Flags & FR_REPLACEALL) )
        {
            SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
        }
        bStatus= TRUE;   // found
    }

    return( bStatus );
}

/* ** Recreate notepad edit window, get text from old window and put in
      new window.  Called when user changes style from wrap on/off */
BOOL FAR NpReCreate( long style )
{
    RECT    rcT1;
    HWND    hwndT1;
    HANDLE  hT1;
    int     cchTextNew;
    TCHAR*  pchText;
    BOOL    fWrap = ((style & WS_HSCROLL) == 0);
    HCURSOR hPrevCursor;
    BOOL    bModified;     // modify flag from old edit buffer

    /* if wordwrap, remove soft carriage returns */
    hPrevCursor= SetCursor( hWaitCursor );     // this may take some time...
    if (!fWrap)
        SendMessage(hwndEdit, EM_FMTLINES, FALSE, 0L);

    bModified= (SendMessage( hwndEdit, EM_GETMODIFY, 0,0 ) != 0);

    cchTextNew= (int)SendMessage( hwndEdit, WM_GETTEXTLENGTH, 0, 0L );
    hT1= LocalAlloc( LMEM_MOVEABLE, ByteCountOf(cchTextNew + 1) );
    if( !hT1 )
    {
        /* failed, was wordwrap; insert soft carriage returns */
        if (!fWrap)
            SendMessage(hwndEdit, EM_FMTLINES, TRUE, 0L);
        SetCursor( hPrevCursor );
        return FALSE;
    }

    GetClientRect( hwndNP, (LPRECT)&rcT1 );

    /*
     * save the current edit control text.
     */
    pchText= LocalLock (hT1);
    SendMessage( hwndEdit, WM_GETTEXT, cchTextNew+1, (LPARAM)pchText );
    hwndT1= CreateWindowEx( WS_EX_CLIENTEDGE,
        TEXT("Edit"),
        TEXT(""), // pchText
        style,
        0,
        0,
        rcT1.right,
        rcT1.bottom,
        hwndNP,
        (HMENU)ID_EDIT,
        hInstanceNP, NULL );
    if( !hwndT1 )
    {
        SetCursor( hPrevCursor );
        if (!fWrap)
            SendMessage( hwndEdit, EM_FMTLINES, TRUE, 0L );
        LocalUnlock(hT1);
        LocalFree(hT1);
        return FALSE;
    }

    //
    // The user can "add" styles to the edit window after it is
    // created (like WS_EX_RTLREADING) when language packs are installed.
    // Preserve these styles when changing the word wrap.
    //

    SetWindowLong( hwndT1 ,
                   GWL_EXSTYLE ,
                   GetWindowLong( hwndEdit , GWL_EXSTYLE )|WS_EX_CLIENTEDGE ) ;

    // Set font before set text to save time calculating
    SendMessage( hwndT1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0) );

    if (!SendMessage (hwndT1, WM_SETTEXT, 0, (LPARAM) pchText))
    {
        SetCursor( hPrevCursor );
        if (!fWrap)
            SendMessage( hwndEdit, EM_FMTLINES, TRUE, 0L );
        DestroyWindow( hwndT1 );
        LocalUnlock( hT1 );
        LocalFree( hT1 );
        return FALSE;
    }
    LocalUnlock(hT1);


    DestroyWindow( hwndEdit );     // out with the old
    hwndEdit = hwndT1;             // in with the new
    /*
     * Win32s does not support the EM_SETHANDLE message, so just do
     * the assignment.  hT1 already contains the edit control text.
     */
    /* free the earlier allocated memory in hEdit */
    if (hEdit)
        LocalFree(hEdit);

    hEdit = hT1;

    /* limit text for safety's sake. */
    PostMessage( hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L );

    ShowWindow(hwndNP, SW_SHOW);
    SetTitle( fUntitled ? szUntitled : szFileName );
    SendMessage( hwndEdit, EM_SETMODIFY, bModified, 0L );
    SetFocus(hwndEdit);

    SetCursor( hPrevCursor );   // restore cursor
    return TRUE;
}

