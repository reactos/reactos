#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

int fReadOnly = FALSE;

/* win.ini string for fValidate value */
TCHAR szValidateFileWrite[] = TEXT("ValidateFileWrite");
BOOL fValidate = TRUE;  /* TRUE if validating on save */

NOEXPORT BOOL      ValidCardFile   (TCHAR szFile[], WORD fType);
NOEXPORT BOOL      FileHasTextOnly (HANDLE hCards, HANDLE fhMain, HANDLE fhTemp);

/*
 * Calls GetSaveFileName(), the COMMDLG function.
 * If the filename returned is invalid, it prompts again.
 * If the given filename exists, it asks for overwrite permission.
 * return FALSE if dlg cancelled, TRUE otherwise.
 *
 * also returns requested file type to save if function TRUE.
 */

BOOL MyGetSaveFileName (TCHAR *szFile, WORD * pfType)
{
  BOOL     fResult;
  HANDLE   fh;

    while (TRUE)
    {
        OFN.lpstrDefExt       = szFileExtension;
        OFN.lpstrFilter       = szFilterSpec;
        OFN.lpstrCustomFilter = szCustFilterSpec;

        lstrcpy (szFile, FileFromPath (CurIFile));

        OFN.lpstrFile         = szFile;
        OFN.lpstrInitialDir   = szLastDir;
        OFN.lpstrTitle        = szSaveCaption;
        OFN.lpfnHook          = (LPOFNHOOKPROC) MakeProcInstance(HookProc, hIndexInstance);
        OFN.Flags             = OFN_PATHMUSTEXIST | OFN_ENABLEHOOK|OFN_OVERWRITEPROMPT;

        /* set reasonable default filter for file type */
        OFN.nFilterIndex      = fFileType;

        fValidDB = fValidate;
        fInSaveAsDlg = TRUE;
        LockData (0);
        fResult = GetSaveFileName (&OFN);
        UnlockData (0);
        fInSaveAsDlg = FALSE;
        FreeProcInstance (OFN.lpfnHook);

        if (!fResult)
            return FALSE;               /* SaveFile Dlg cancelled */

        if (OFN.nFilterIndex == ANSI_FILE)
           *pfType = ANSI_FILE;
        else if (OFN.nFilterIndex == OLD_FORMAT)
           *pfType = OLD_FORMAT;
        else
           *pfType = UNICODE_FILE;

        fValidate = fValidDB;

        if ((fh = MyOpenFile (szFile, NULL, OF_EXIST)) == INVALID_HANDLE_VALUE)
        {
           DWORD err = GetLastError();

           if (err == ERROR_ACCESS_DENIED)
           {
              BuildAndDisplayMsg(E_FILECANTWRITE, szFile);
              continue;
           }

           return TRUE;
        }

        /* New name is same as current file name, share won't let us open
         * the file again, so we shouldn't even try an open */
        if (lstrcmpi (szFile, CurIFile) == 0)
            return TRUE;
        fh = MyOpenFile (szFile, NULL, OF_READWRITE);
        if (fh == INVALID_HANDLE_VALUE)   /* file exists and it is read only */
        {
           DWORD err = GetLastError();

           if (err == ERROR_ACCESS_DENIED)
           {
              BuildAndDisplayMsg(E_FILECANTWRITE, szFile);
              continue;
           }

           return TRUE;
        }
        else
           MyCloseFile (fh);

        return TRUE;    /* OK to overwrite, return filename */
    }
}

NOEXPORT BOOL NEAR WriteCardfileFailed (
    int           errorID,
    HANDLE        fhDest,
    HANDLE        fhTemp,
    TCHAR        *DestFile,
    LPCARDHEADER  Cards)
{

    Hourglass (FALSE);

    MyCloseFile (fhDest);
    MyCloseFile (fhTemp);
    if (Cards)
        GlobalUnlock (hCards);
    Fdelete (DestFile);
    return IndexOkError (errorID);
}

/* WriteCardFile
 *
 */

int WriteCardFile (TCHAR *pName, WORD fType)
{
  WORD         i;
  INT          nChars;
  int          fSameFile;
  CARDHEADER   CardHeader;
  BOOL         fOld;
  LPCARDHEADER Cards = NULL;
  HANDLE       fh;
  INT          fSourceType;              // type of each line (UNICODE_FILE,ANSI_FILE etc)

  /* destination file vars */
  TCHAR        DestFile[PATHMAX];
  WORD         wCards;
  HANDLE       fhDest;
  DWORD        oHdr;         /* offset to Hdr in fhDest */
  DWORD        oCard;        /* offset to Card in fhDest */

  HANDLE       fhTemp;       /* temp file containing modified cards */

    // validate file types being passed
    if( (fType!=UNICODE_FILE) && (fType!=ANSI_FILE) && (fType!=OLD_FORMAT))
    {
        OutputDebugString(TEXT("Bad File type\n"));
        return( BuildAndDisplayMsg( E_FILEUPDATEFAILED, pName ) );
    }

    /*
     * Set destination file name.
     * If saving to source file, an intermediate temp file is needed
     * else can save directly to the given file.
     */
    if (fSameFile = !lstrcmp (pName, CurIFile))
    {
        TCHAR szTemp[PATHMAX];

        GetTempPath (PATHMAX - 1, szTemp);

        if (!GetTempFileName (szTemp, szFileExtension, 0, DestFile))
            return BuildAndDisplayMsg (ECANTMAKEFILE, pName);
    }
    else
        lstrcpy (DestFile, pName);

    /* Open Destination file */
    fhDest = MyOpenFile (DestFile, NULL, OF_SHARE_DENY_WRITE | OF_READWRITE | OF_CREATE);

    /* Open the temp file containing modified/new cards */
    fhTemp = MyOpenFile (TempFile, NULL, OF_SHARE_EXCLUSIVE | OF_READWRITE);

    if (fhDest == INVALID_HANDLE_VALUE || fhTemp == INVALID_HANDLE_VALUE)
    {
        TCHAR szFormat[100];
        TCHAR szMsg[200];

        /* close valid file handles */
        if (fhTemp != INVALID_HANDLE_VALUE)
            MyCloseFile (fhTemp);
        if (fhDest != INVALID_HANDLE_VALUE)
            MyCloseFile (fhDest);

        LoadString (hIndexInstance, E_FILESAVE, szFormat, CharSizeOf(szFormat));
        wsprintf (szMsg, szFormat, pName,
                  (fhDest == INVALID_HANDLE_VALUE) ? DestFile : TempFile);
        MessageBox (hIndexWnd, szMsg, szNote, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if ((fSameFile && fReadOnly) || /* saving to same file which was read in as read only */
        fNoTempFile ||              /* no temp file ? */
        MyFileSeek (fhDest, 0L, 0) == -1)   /* truncate destination file */
    {
        CloseHandle (fhDest);
        CloseHandle (fhTemp);
        return BuildAndDisplayMsg (ECANTMAKEFILE, pName);
    }

    Hourglass (TRUE);
    MyByteWriteFile (fhDest, TEXT(""), 0);

    /* Write cardfile identifier(DKO is Unicode and RRG is ANSI) and
     * number of cards */
    if (fType == UNICODE_FILE)
    {
        if (!MyByteWriteFile (fhDest, chDKO, 3))
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
    }
    else if (fType == ANSI_FILE)
    {
       if (!MyByteWriteFile (fhDest, chRRG, 3))
           return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
    }
    else if (fType == OLD_FORMAT)
    {
       if (!MyByteWriteFile (fhDest, chMGC, 3))
           return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
    }

    if (fType != OLD_FORMAT && !MyByteWriteFile (fhDest, &idObjectMax, sizeof(DWORD)))
        return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);

    wCards = (WORD)cCards;
    if (!MyByteWriteFile (fhDest, &wCards, sizeof(WORD)))
        return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);

    if (fType == UNICODE_FILE)
       oCard = MyFileSeek (fhDest, 0L, 1) +
               (DWORD)((WORD)cCards * (WORD)SIZEOFCARDHEADERW);
    else
       oCard = MyFileSeek (fhDest, 0L, 1) +
               (DWORD)((WORD)cCards * (WORD)SIZEOFCARDHEADERA);

    /* lock down the card headers */
    Cards = (LPCARDHEADER) GlobalLock (hCards);
    for (i = 0; i < cCards; i++)
    {
        /* write out card hdr */
        CardHeader = Cards[i];
        CardHeader.flags &= (~FTMPFILE);    /* no more in the temp file */
        CardHeader.lfData = oCard;          /* set new offset */

        if (!MyByteWriteFile (fhDest, &(CardHeader.reserved), 6))
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        if (!MyByteWriteFile (fhDest, &(CardHeader.lfData), 4))
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        if (!MyByteWriteFile (fhDest, &(CardHeader.flags), 1))
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);

        if (fType == UNICODE_FILE)
        {
           if (!MyByteWriteFile (fhDest, CardHeader.line, ByteCountOf(LINELENGTH+1)))
              return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        }
        else
        {
           if (!MyAnsiWriteFile (fhDest, CP_ACP, CardHeader.line, LINELENGTH+1))
               return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        }

        oHdr = MyFileSeek (fhDest, 0L, 1);      /* store offset for the next hdr */

        /* pick up card data from src or temp file */
        if (Cards[i].flags & FTMPFILE)
        {
            fSourceType= UNICODE_FILE;     // the temp file is always UNICODE
            fh = fhTemp;
        }
        else
        {
            fSourceType= fFileType;        // original source never changes
            fh = fhMain;
        }

         /* If we're in the compatibility zone, punt */
         fOld = (fSourceType == OLD_FORMAT);

        /* read card data and write it out */
        if (MyFileSeek (fh, Cards[i].lfData, 0) == -1 ||
            !PicRead (&CurCard, fh, !fOLE || fOld))
        {
#ifdef WIN32
            CurIFile[2] = TEXT('\0') ;
            return WriteCardfileFailed (
                          (GetDriveType (CurIFile) == DRIVE_REMOVABLE)
                                           ? E_FLOPPY_WITH_SOURCE_REMOVED
                                           : E_FAILED_TO_READ_CARD,
                                           fhDest, fhTemp, DestFile, Cards);
#else
            return WriteCardfileFailed (
                          (GetDriveType (CurIFile[0]  - TEXT('A')) == DRIVE_REMOVABLE)
                                           ? E_FLOPPY_WITH_SOURCE_REMOVED
                                           : E_FAILED_TO_READ_CARD,
                                           fhDest, fhTemp, DestFile, Cards);
#endif
        }

        if (MyFileSeek (fhDest, oCard, 0) == -1 ||
            !PicWrite (&CurCard, fhDest, fType == OLD_FORMAT))
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        if (CurCard.lpObject)
            PicDelete (&CurCard);

        /* read text size and text, write it out */
        if ((nChars = TextRead (fh, szText, fSourceType)) < 0 ||
            !MyByteWriteFile (fhDest, &nChars, (DWORD) sizeof(WORD)))
        {
            return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        }

        if (fType == UNICODE_FILE)
        {
            if (!MyByteWriteFile (fhDest, szText, ByteCountOf(nChars)))
               return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        }
        else
        {
            if (!MyAnsiWriteFile (fhDest, CP_ACP, szText, nChars))
               return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);
        }

        oCard = MyFileSeek (fhDest, 0L, 1); /* store offset for the next card data */
        MyFileSeek (fhDest, oHdr, 0);       /* ready to read the next hdr */
    }

    MyFileSeek (fhDest, 3, 0);       /* put in correct value of idObjectMax */
    if (fType != OLD_FORMAT && !MyByteWriteFile (fhDest, &idObjectMax, sizeof(DWORD)))
        return WriteCardfileFailed (EDISKFULLFILE, fhDest, fhTemp, DestFile, Cards);

    MyCloseFile (fhDest);                    /* close destination file */

    /* check if destination file is a valid cardfile */
    /* Cardfile corruption problem:
     * Cardfile occasionally writes out corrupted files.
     * During File.Save:
     * If the file is corrupted then the rename is skipped, preserving the
     * original source file from being updated to a corrupted one.
     *
     * During File.SaveAs:
     * If it is corrupted, the file is deleted.
     */
    if (!ValidCardFile (DestFile, fType))
    {
        Fdelete (DestFile);   /* no use creating a corrupted file */
        if (fSameFile)
            BuildAndDisplayMsg (E_FILEUPDATEFAILED, CurIFile);
        else
            BuildAndDisplayMsg (E_FILEWRITEFAILED, DestFile);
        Hourglass (FALSE);
        GlobalUnlock (hCards);
        return TRUE;          /* avoid putting up the SaveAs dialog */
    }

    if (fhMain != INVALID_HANDLE_VALUE)          /* close source file if any */
        MyCloseFile (fhMain);
    if (fSameFile)
    {
        TCHAR HackBuffer[PATHMAX];
        TCHAR HackDest[PATHMAX];

        MyCloseFile (fhTemp);

        lstrcpy (HackDest, DestFile);
        lstrcpy (HackBuffer, pName);

        if (Fdelete (HackBuffer))
        {
            Fdelete (HackDest);  /* remove temp file */
            BuildAndDisplayMsg (E_FILEWRITEFAILED, CurIFile); /* delete old src file */
            fhMain = MyOpenFile (CurIFile, NULL, OF_SHARE_DENY_WRITE | OF_READ);
            return FALSE;
        }

        // We MUST copy the file and delete the source because rename does not
        // work if the user has their TMP path set to another drive!!
        if (CopyFile (HackDest, HackBuffer, TRUE))  // File if already exists
           DeleteFile (HackDest);
    }
    else
    {
        MyFileSeek (fhTemp, 0L, 0);             /* truncate temp file and close it */
        MyByteWriteFile (fhTemp, TEXT(""), 0);
        MyCloseFile (fhTemp);
    }

    fReadOnly = FALSE;  /* we just wrote out the new file */
    fhMain = MyOpenFile (pName, NULL, OF_SHARE_DENY_WRITE | OF_READ);
    lstrcpy (CurIFile, pName);

    /* check cardfile signature */
    if (!CheckCardfileSignature (fhMain, &fType))
    {
        MyCloseFile (fhMain);
        fhMain = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    /* read the number of cards in the file */
    MyByteReadFile (fhMain, &wCards, sizeof(WORD));
    cCards = (INT) wCards;

#if !defined WIN32
    for (i = 0; i < cCards; ++i)
    {
        MyByteReadFile (fhMain, &CardHeader, sizeof(CARDHEADER));
        Cards[i] = CardHeader;
    }
#else

   // Lack of packing on WIN32...
    for (i = 0; i < cCards; i++)
    {
        MyByteReadFile (fhMain, &(CardHeader.reserved), 6);
        MyByteReadFile (fhMain, &(CardHeader.lfData), 4);
        MyByteReadFile (fhMain, &(CardHeader.flags), 1);
        if (fType == UNICODE_FILE)
           MyByteReadFile (fhMain, CardHeader.line, ByteCountOf(LINELENGTH+1));
        else
           MyAnsiReadFile (fhMain, CP_ACP, CardHeader.line, LINELENGTH+1);
        Cards[i] = CardHeader;
    }
#endif
    fFileDirty = FALSE;

    fFileType = fType;

    GlobalUnlock (hCards);
    Hourglass (FALSE);

    return TRUE;
}

NOEXPORT BOOL ValidCardfileFailed(
    HANDLE fh)
{
    MyCloseFile (fh);
    return FALSE;
}

/*
 * Returns TRUE if PicRead is successful on all the cards in the file
 * FALSE otherwise.
 */
NOEXPORT BOOL NEAR ValidCardFile(
    TCHAR szFile[],
    WORD fType)
{
    int      i;
    HANDLE   fh;
    BYTE     Signature[3];
    WORD     nCards;
    DWORD    nObjectMax;
    DWORD    cBytes;
    HANDLE   hHdrs;
    CARD     Card;
    LPCARDHEADER Cards;

    if (!fValidate)
        return TRUE;

    /* open file */
    fh = MyOpenFile (szFile, NULL, OF_READ | OF_SHARE_DENY_WRITE);
    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    /* try to read the cardfile */

    /* read the cardfile signature */
    MyByteReadFile (fh, Signature, 3);
    if ((fType == OLD_FORMAT && memcmp (Signature, chMGC, 3)) ||
        (fType == ANSI_FILE && memcmp (Signature, chRRG, 3))  ||
        (fType == UNICODE_FILE && memcmp (Signature, chDKO, 3)))
        return ValidCardfileFailed(fh);

    if (fType != OLD_FORMAT)
    {
        /* read in the max id */
        MyByteReadFile(fh, &nObjectMax, sizeof(DWORD));
        if (nObjectMax != idObjectMax)
            return ValidCardfileFailed(fh);
    }

    /* read the number of cards in the file */
    MyByteReadFile (fh, &nCards, sizeof(WORD));

    cBytes = mylmul(nCards, SIZEOFCARDHEADER);

    if (nCards != cCards ||
#if !defined (WIN32)
        cBytes >= 0x0000FFFF ||
#endif
        !(hHdrs = GlobalAlloc(GHND, (DWORD)(nCards * sizeof(CARDHEADER)))) ||
        !(Cards = (LPCARDHEADER) GlobalLock(hHdrs)))
        return ValidCardfileFailed(fh);

    /* read all the header into memory */
#if !defined (WIN32)
    for(i = 0; i < nCards; i++)
        MyByteReadFile(fh, &Cards[i], sizeof(CARDHEADER));
#else
   // Lack of packing on WIN32...
    for (i = 0; i < nCards; i++)
    {
        MyByteReadFile (fh, &(Cards[i].reserved), 6);
        MyByteReadFile (fh, &(Cards[i].lfData), 4);
        MyByteReadFile (fh, &(Cards[i].flags), 1);
        if (fType == UNICODE_FILE)
           MyByteReadFile (fh, Cards[i].line, ByteCountOf(LINELENGTH+1));
        else
           MyAnsiReadFile (fh, CP_ACP, Cards[i].line, LINELENGTH+1);
    }
#endif

    /* read each card data */
    for (i = 0; i < nCards; i++)
    {
        /* seek to card data */
        if (MyFileSeek (fh, Cards[i].lfData, 0) == -1 ||
            !PicRead (&Card, fh, !fOLE || (fType == OLD_FORMAT)) ||
            TextRead (fh, szText, fType) < 0)
        {
            GlobalUnlock (hHdrs);
            GlobalFree (hHdrs);
            return ValidCardfileFailed(fh);
        }
        if (Card.lpObject)
            PicDelete (&Card);
    }

    GlobalUnlock (hHdrs);
    GlobalFree (hHdrs);
    MyCloseFile (fh);
    return TRUE;
}

#ifndef WIN32
#define RINT int
#else
#define RINT short
#endif

