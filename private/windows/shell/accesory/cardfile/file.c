#include "precomp.h"

#ifdef UNICODE
#include <wchar.h>
#else
#include <string.h>
#endif

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

WORD fFileType = UNICODE_FILE;
int fNoTempFile = FALSE;
TCHAR TempFile[PATHMAX];

TCHAR SavedIndexLine[LINELENGTH+1];


/* Read text data for a card. */
int TextRead (HANDLE fh, TCHAR *szBuf, WORD fType)
{
    WORD nChars;

    *szBuf = (TCHAR) 0;

    /* get text size */
    if (!MyByteReadFile (fh, &nChars, sizeof(WORD)))
        return -1;

    /* truncate it if too long */
    if (nChars >= CARDTEXTSIZE)
        nChars = CARDTEXTSIZE - 1;

    /* read the text */
    if (fType == UNICODE_FILE)
    {
       if (!MyByteReadFile (fh, szBuf, ByteCountOf(nChars)))
          return -1;
    }
    else
    {
       if (!MyAnsiReadFile (fh, CP_ACP, szBuf, nChars))
          return -1;
    }
    szBuf[nChars] = (TCHAR) 0;

    return nChars;
}

/* append cardfile extension to the given filename */
void AppendExtension (TCHAR *pName, TCHAR *pBuf)
{
    TCHAR *p;
    TCHAR  ch;

    lstrcpy(pBuf, pName);
    p = pBuf + lstrlen(pBuf);
    while ((ch = *p) != TEXT('.') && ch != TEXT('\\') && ch != TEXT(':') && p > pBuf)
        p = CharPrev(pBuf, p);
    if (*p != TEXT('.'))
        wsprintf(pBuf, TEXT("%s.%s"), pName, szFileExtension);
    CharUpper(pBuf);
}

/*
 * write CardHead, Card object and Text
 */
int WriteCurCard (PCARDHEADER pCardHead, PCARD pCard, TCHAR *pText)
{
   HANDLE fh;
   DWORD  lEnd;
   WORD   nChars;
   int    Result;

    if ((fh = MyOpenFile (TempFile, NULL,
                          OF_SHARE_EXCLUSIVE | OF_READWRITE)) == INVALID_HANDLE_VALUE)
        return IndexOkError(EOPENTEMPSAVE);

    lEnd = MyFileSeek (fh, 0L, FILE_END);   /* seek to the end of the file */

    Hourglass (TRUE);
    nChars = lstrlen (pText);
    if (nChars >= CARDTEXTSIZE)
       nChars = CARDTEXTSIZE - 1;

    /* If an embedded object is open for editing, try to update */
    Result = UpdateEmbObject (&CurCard, MB_YESNO);
    if (Result != IDYES && !fInsertComplete)
    {
       PicDelete (pCard);
       fInsertComplete = TRUE;
    }

    if (!PicWrite (pCard, fh, FALSE) ||                  /* write object    */
        !MyByteWriteFile (fh, &nChars, sizeof(WORD)))    /* write text size */
    {
WriteError:
       MyCloseFile (fh);
       Hourglass (FALSE);
       return (IndexOkError (EDISKFULLSAVE));
    }

    if (!MyByteWriteFile (fh, pText, ByteCountOf(nChars)))   /* write text */
       goto WriteError;

    MyCloseFile (fh);
    pCardHead->flags |= FTMPFILE;
    pCardHead->lfData = lEnd;

    /* Make a copy of the current index line for restore purposes */
    lstrcpy (SavedIndexLine, pCardHead->line);

    Hourglass (FALSE);
    return (TRUE);
}

int ReadCurCardData (PCARDHEADER pCardHead, PCARD pCard, TCHAR *pText)
{
    HANDLE fh;
    BOOL fOld;
    WORD fType;

    /* !!!bozo check for deleteable objects here? */

    if (pCardHead->flags & FNEW)
        return TRUE;

    if (pCardHead->flags & FTMPFILE)
    {
        fh = MyOpenFile (TempFile, NULL, OF_SHARE_EXCLUSIVE | OF_READWRITE);
        fOld = FALSE;
        fType = UNICODE_FILE;
    }
    else
    {
        fh = fhMain;
        fOld = (fFileType == OLD_FORMAT);
        fType = fFileType;
    }

    if (fh == INVALID_HANDLE_VALUE)
        return BuildAndDisplayMsg (E_CANT_REOPEN_FILE,
                                   (pCardHead->flags & FTMPFILE)? TempFile: CurIFile);

    Hourglass(TRUE);
    if (MyFileSeek(fh, pCardHead->lfData, 0) == -1 ||
        !PicRead(pCard, fh, !fOLE || fOld) ||   /* read object  */
        TextRead(fh, pText, fType) < 0)                /* and the text */
    {
        if (pCardHead->flags & FTMPFILE)
            MyCloseFile(fh);
        IndexOkError(E_FAILED_TO_READ_CARD);
    }
    Hourglass(FALSE);
    if (pCardHead->flags & FTMPFILE)
        MyCloseFile(fh);

    /* Make a copy of the Index line for restoring latter */
    lstrcpy(SavedIndexLine, pCardHead->line);

    return TRUE;
}


/* return ptr to the first occurrence of the given char in the string */
/* return ptr to filename in the given path */
LPTSTR FileFromPath (LPTSTR lpStr)
{
    LPTSTR lp;

    lp = _tcsrchr(lpStr, TEXT('\\'));
    if (lp == NULL)
        return lpStr;
    else
        return (lp + 1);
}


/* create space in the array of hdrs for n more cards */
BOOL ExpandHdrs (int n)
{
  DWORD cBytes;

    cBytes = mylmul (cCards + n, sizeof(CARDHEADER));
    /* if reallocation fails, return failure */
    if( !GlobalReAlloc( hCards, cBytes, GMEM_MOVEABLE ) )
    {
        return IndexOkError (EINSMEMORY);
    }

    return TRUE;
}


BOOL MyIsTextUnicode (VOID)
{
    WORD          i;
    LPCARDHEADER  Cards;
    CARDHEADER    CardHeader;
    CARD          Card;
    BOOL          fDefCharUsed = FALSE;

    /* lock down the card headers */
    Cards = (LPCARDHEADER) GlobalLock (hCards);
    for (i = 0; i < cCards; i++)
    {
        CardHeader = Cards[i];

        if (*CardHeader.line)
        {
           WideCharToMultiByte (CP_ACP, 0, CardHeader.line, -1, NULL, -1, NULL, &fDefCharUsed);
           if (fDefCharUsed)
              goto Exit;
        }

        /* check the card data */
        ReadCurCardData (&CardHeader, &Card, szText);
        WideCharToMultiByte (CP_ACP, 0, szText, -1, NULL, -1, NULL, &fDefCharUsed);
        if (fDefCharUsed)
           goto Exit;
    }

Exit:
    GlobalUnlock (hCards);

    return (fDefCharUsed);
}

