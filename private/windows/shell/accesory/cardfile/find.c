#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

#if defined(WIN32)
LPTSTR ReverseScan(LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSearch, BOOL fCaseSensitive )
{
   int iLen = lstrlen(lpSearch);

   if( !lpLast )
      lpLast = lpSource + lstrlen(lpSource);

   do
   {
      if( lpLast == lpSource )
         return NULL;

      --lpLast;
      if( fCaseSensitive )
      {
         if( *lpLast != *lpSearch )
            continue;
      }
      else
      {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpLast, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0)))
            continue;
      }

      if( fCaseSensitive )
      {
         if( !_tcsncmp( lpLast, lpSearch, iLen ) )
            break;
      }
      else
      {
         if( !_tcsnicmp( lpLast, lpSearch, iLen ) )
            break;
      }

   } while( TRUE );

   return lpLast;
}

LPTSTR ForwardScan(LPTSTR lpSource, LPTSTR lpSearch, BOOL fCaseSensitive )
{
   int iLen = lstrlen(lpSearch);

   while( *lpSource )
   {
      if( fCaseSensitive )
      {
         if( *lpSource != *lpSearch )
         {
            lpSource++;
            continue;
         }
      }
      else
      {
         if (CharUpper ((LPTSTR)MAKELONG((WORD)*lpSource, 0)) != CharUpper ((LPTSTR)MAKELONG((WORD)*lpSearch, 0)))
         {
            lpSource++;
            continue;
         }
      }

      if( fCaseSensitive )
      {
         if( !_tcsncmp( lpSource, lpSearch, iLen ) )
            break;
      }
      else
      {
         if( !_tcsnicmp( lpSource, lpSearch, iLen ) )
            break;
      }

      lpSource++;
   }

   return *lpSource ? lpSource : NULL;
}
#endif

/*
 * Forward Search for szSearch
 *
 * Start search from the current selection or cursor if it is a new string.
 * Start from one char after the beginning of the current selection if we are
 * repeating a search.
 * If not found, search continues through the text in the consecutive cards.
 * If still not found, search continues from the beginning of the text in the
 * current card. If still not found, then report failure.
 */
void ForwardSearch(
    void)
{
    int i, index, iCard = iFirstCard;
    int SelStart, SelEnd;
    TCHAR *pStart;
    WORD Len;
    TCHAR cHold, buf[160];
    TCHAR *pMatch, *pLast;
    CARDHEADER CardHead;
    CARD Card;
    LPCARDHEADER Cards;
    HCURSOR hOldCursor;
    DWORD Sel;

    hOldCursor = SetCursor(hWaitCurs);   /* Putup the Hour glass cursor */
    GetWindowText(hEditWnd, szText, CARDTEXTSIZE);
    /* Get current selection */
    Sel = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);
    SelStart = LOWORD(Sel);
    SelEnd = HIWORD(Sel);
    /* if we are repeating a search and there exists a current selection
     * start from one char after the beginning of the selection. */
    if (fRepeatSearch && SelStart != SelEnd)
        pStart = szText + SelStart + 1;
    else /* not repeating a search or no selection exists */
        pStart = szText + SelStart;

#if !defined(WIN32)
    pMatch = (fCase ? StrStr(pStart, szSearch):
                      StrStrI(pStart, szSearch));
#else
   pMatch = ForwardScan( pStart, szSearch, fCase );
#endif

    if (pMatch)
        goto FS_DONE;

    /* Continue search through the text in the consecutive cards. */
    Cards = (LPCARDHEADER) GlobalLock(hCards);
    for (i = 0, iCard = iFirstCard+1; i < cCards-1; i++, iCard++)
    {
        if (iCard >= cCards)
            iCard = 0;
        CardHead = Cards[iCard];

        /* Stupidly and blindly, ReadCurCardData always
         * creates the object after it reads it.  This
         * is unnecessary.
         */
        ReadCurCardData(&CardHead, &Card, szText);
        if (Card.lpObject)
            PicDelete(&Card);

#if !defined(WIN32)
        pMatch = (fCase ? StrStr(szText, szSearch):
                      StrStrI(szText, szSearch));
#else
        pMatch = ForwardScan( szText, szSearch, fCase );
#endif

        if (pMatch)
        {
            GlobalUnlock(hCards);
            goto FS_DONE;
        }
    }

    GlobalUnlock(hCards);
    /* szText has been destroyed, reget the window text */
    iCard = iFirstCard;
    GetWindowText(hEditWnd, szText, CARDTEXTSIZE);

    /* Continue search from beginning of the text in the current card. */

    /* Null terminate the string at the last character included in the search */
    pLast = szText + SelStart + lstrlen(szSearch);
    if (fRepeatSearch && SelStart != SelEnd)
        pLast--;
    Len = lstrlen(szText);
    if ((pLast - szText) > Len)     /* beyond end of buffer? */
        pLast = szText + Len;
    cHold = *pLast;                 /* remember this char */
    *pLast = 0;                     /* replace with NUL */

#if !defined(WIN32)
    pMatch = (fCase ? StrStr(szText, szSearch):
#else
    pMatch = ForwardScan( szText, szSearch, fCase );
#endif

    *pLast = cHold;                 /* restore the char */

FS_DONE:
    SetCursor(hOldCursor);  /* Restore the old cursor */
    if (pMatch)
    {
        if (iCard != iFirstCard &&
            !ScrollCards(hCardWnd, SB_THUMBPOSITION, iCard))
        {
            MessageBox(hIndexWnd, NotEnoughMem, szCardfile, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
            return;
        }
        index = pMatch - szText;
#if defined(WIN32)
        SendMessage(hEditWnd, EM_SETSEL, (WPARAM)index, (DWORD)(index+lstrlen(szSearch)) );
#else
        SendMessage(hEditWnd, EM_SETSEL, 0, MAKELONG(index, index+lstrlen(szSearch)));
#endif
    }
    else
    {
        wsprintf(buf, TEXT("\"%s\""), szSearch);
        if( hDlgFind )
            EnableWindow(hDlgFind, FALSE);   /* Must disable since it's modeless */
        BuildAndDisplayMsg(ECANTFIND, buf);
        if( hDlgFind )
        {
           EnableWindow(hDlgFind, TRUE);
           SetFocus(hDlgFind);
        }
    }
}


/*
 * Reverse Search for szSearch
 *
 * Start search from start of the current selection if any in the edit control.
 * If not found, search continues through the text in the preceding cards.
 * If still not found, search continues from the end of the text in the
 * current card till the current selection. If still not found,
 * then report failure.
 */
void ReverseSearch(
    void)
{
    int i, index, iCard = iFirstCard;
    int SelStart, SelEnd;
    TCHAR *pStart, *pEnd;
    TCHAR buf[160];
    TCHAR *pMatch;
    CARDHEADER CardHead;
    CARD Card;
    LPCARDHEADER Cards;
    HCURSOR hOldCursor;
    DWORD Sel;

    hOldCursor = SetCursor(hWaitCurs);   /* Putup the Hour glass cursor */
    GetWindowText(hEditWnd, szText, CARDTEXTSIZE);
    /* Get current selection */
    Sel = SendMessage(hEditWnd, EM_GETSEL, 0, 0L);
    SelStart = LOWORD(Sel);
    SelEnd = HIWORD(Sel);

    /* Compute last char position where we want the string to be found.
     * This position won't be included in the search, so it will find the
     * previous occurrence */
    pEnd = szText + SelStart;

#if !defined(WIN32)
    pMatch = (fCase ? StrRStr(szText, pEnd, szSearch):
                      StrRStrI(szText, pEnd, szSearch));
#else
    pMatch = ReverseScan( szText, pEnd, szSearch, fCase );
#endif

    if (pMatch)
        goto FS_DONE;

    /* Continue search through the text in the consecutive cards. */
    Cards = (LPCARDHEADER) GlobalLock(hCards);
    for (i = 0, iCard = iFirstCard-1; i < cCards-1; i++, iCard--)
    {
        if (iCard < 0)
            iCard = cCards - 1;
        CardHead = Cards[iCard];

        /* Stupidly and blindly, ReadCurCardData always
         * creates the object after it reads it.  This
         * is unnecessary.
         */
        ReadCurCardData(&CardHead, &Card, szText);
        if (Card.lpObject)
            PicDelete(&Card);
        pEnd = szText + lstrlen(szText);

#if !defined(WIN32)
        pMatch = (fCase ? StrRStr(szText, pEnd, szSearch):
                          StrRStrI(szText, pEnd, szSearch));
#else
        pMatch = ReverseScan( szText, pEnd, szSearch, fCase );
#endif

        if (pMatch)
        {
            GlobalUnlock(hCards);
            goto FS_DONE;
        }
    }

    GlobalUnlock(hCards);
    /* szText has been destroyed, reget the window text */
    iCard = iFirstCard;
    GetWindowText(hEditWnd, szText, CARDTEXTSIZE);

    /* Continue search from end of the text in the current card, till the
     * current selection */
    pStart = szText + SelEnd - lstrlen(szSearch) + 1;
    if (pStart < szText)
        pStart = szText;
    pEnd = szText + lstrlen(szText);

#if !defined(WIN32)
    pMatch = (fCase ? StrRStr(pStart, pEnd, szSearch):
                      StrRStrI(pStart, pEnd, szSearch));
#else
    pMatch = ReverseScan( pStart, pEnd, szSearch, fCase );
#endif

FS_DONE:
    SetCursor(hOldCursor);  /* Restore the old cursor */
    if (pMatch)
    {
        if (iCard != iFirstCard &&
            !ScrollCards(hCardWnd, SB_THUMBPOSITION, iCard))
        {
            MessageBox(hIndexWnd, NotEnoughMem, szCardfile, MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
            return;
        }
        index = pMatch - szText;
#if defined( WIN32)
        SendMessage(hEditWnd, EM_SETSEL, (WPARAM)index, (DWORD)(index+lstrlen(szSearch)) );
#else
        SendMessage(hEditWnd, EM_SETSEL, 0, MAKELONG(index, index+lstrlen(szSearch)));
#endif
    }
    else
    {
        wsprintf(buf, TEXT("\"%s\""), szSearch);
        /* in case of "F3" back search.
         * When not to display "FindDialogBox"  because cannot set focus
         *                          1992 March t-yoshio
         */
        if( hDlgFind )
            EnableWindow (hDlgFind, FALSE);  /* Disable since it's modeless */
        BuildAndDisplayMsg (ECANTFIND, buf);
        if (hDlgFind)
        {
            EnableWindow (hDlgFind, TRUE);
            SetFocus (hDlgFind);
        }
    }
}

void DoGoto(
    TCHAR *pBuf)
{
    int i;
    int j;
    LPCARDHEADER Cards;
    int iNextFirst;
    TCHAR buf[160];

    Cards = (LPCARDHEADER) GlobalLock(hCards);
    for (i = 1, j = iFirstCard+1; i <= cCards; i++, j++)
    {
       if (j >= cCards)
           j = 0;
       iNextFirst = i;
#if !defined(WIN32)
       if (StrStrI(Cards[j].line, pBuf))
           break;
#else
       if (ForwardScan(Cards[j].line, pBuf, FALSE))
           break;
#endif
    }
    GlobalUnlock(hCards);

    if (i <= cCards)     /* found it */
    {
        if (CardPhone == CCARDFILE)
            ScrollCards(hCardWnd, SB_THUMBPOSITION, j);
        else
            SendMessage(hListWnd, LB_SETCURSEL, j, 0L);
        iFirstCard = j;
    }
    else
    {
        wsprintf(buf, TEXT("\"%s\""), pBuf);
        if( hDlgFind )
            EnableWindow(hDlgFind, FALSE);   /* Must disable since it's modeless */
        BuildAndDisplayMsg(ECANTFIND, buf);
        if( hDlgFind )
            EnableWindow(hDlgFind, TRUE);
    }
}
