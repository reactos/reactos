#include "precomp.h"

/************************************************************************/
/*                                                                      */
/*  Windows Cardfile - Written by Mark Cliggett                         */
/*  (c) Copyright Microsoft Corp. 1985, 1994 - All Rights Reserved      */
/*                                                                      */
/************************************************************************/

BYTE chDKO[] = {0x44,0x4B,0x4F}; /* "DKO" - new Unicode cardfile signature.(OLE) */
BYTE chRRG[] = {0x52,0x52,0x47}; /* "RRG" - new ANSI cardfile signature.(OLE)    */
BYTE chMGC[] = {0x4D,0x47,0x43}; /* "MGC" - old cardfile signature.              */

HANDLE fhMain = INVALID_HANDLE_VALUE;


NOEXPORT void NEAR LinksExistCheck    (HANDLE fh, int nCardCount, WORD fType);
NOEXPORT int NEAR  AskOkToSaveChanges (TCHAR *szFile);
NOEXPORT BOOL NEAR SaveChanges        (TCHAR *szFile);
NOEXPORT int NEAR  ReadCardFile       (TCHAR *pName);
NOEXPORT BOOL NEAR DoMerge            (TCHAR *szFile);
NOEXPORT int NEAR  MergeCardFile      (TCHAR *pName);


BOOL MaybeSaveFile (int fSystemModal)
{
  int   result;
  TCHAR szFile[PATHMAX];

    /* put up a message box that says "Do you wish to save your edits?" */
    /* if so, save 'em */
    /* if returns FALSE, means it couldn't save, and whatever is happening */
    /* should not continue */
    if (!fReadOnly &&
        (fFileDirty || CurCardHead.flags & FDIRTY || SendMessage (hEditWnd, EM_GETMODIFY, 0, 0L)))
    {
        result = AskOkToSaveChanges (szFile);
        if (result == IDYES)
        {
            if (!SaveChanges (szFile))
                return FALSE;           /* didn't save */
        }
        else if (result == IDCANCEL)
            return (FALSE);
        else        /* result == IDNO */
        {
            if (CurCard.lpObject)
                PicDelete (&CurCard);

            if (fOLE && OLE_OK != OleRevertClientDoc (lhcdoc))
                ErrorMessage (W_FAILED_TO_NOTIFY);
        }
    }
    else if (CurCard.lpObject)
        PicDelete (&CurCard);

    /* Delete "Undo" object before exiting */
    DeleteUndoObject ();
    return (TRUE);
}

/*
 * returns user response for OK to Save cards.
 * returns MB_YES/MB_NO/MB_CANCEL
 */
NOEXPORT int NEAR AskOkToSaveChanges (TCHAR *szFile)
{
  TCHAR szString[100];
  TCHAR szMsg[200];
  int   result;

    LoadString (hIndexInstance, IOKTOSAVE, szString, CharSizeOf(szString));

    if (CurIFile[0])
        lstrcpy (szFile, FileFromPath(CurIFile));
    else
        lstrcpy (szFile, szUntitled);
    CharUpper (szFile);
    MergeStrings (szString, szFile, szMsg);
    result = MessageBox (hIndexWnd, szMsg, szNote, MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL);

    return (result);
}

/* saves changes to a file */
NOEXPORT BOOL NEAR SaveChanges (TCHAR *szFile)
{
  int  fGetName;
  WORD fType;
#ifndef OLE_20
  CHAR aszFile[PATHMAX];
#endif

    if (!SaveCurrentCard (iFirstCard))
        return FALSE;

    if (CurIFile[0])                 /* If CurIFile exists, */
    {
        lstrcpy (szFile, CurIFile);  /* use it.             */
        fGetName = FALSE;            /* no need to get a file name */
    }
    else
        fGetName = TRUE;             /* no filename exists, get one */

    fType = fFileType;

    /*
     * Various cases enumerated:
     * If CurIFile is present, use it for saving changes.
     * If CurIFile not present, prompt for a filename.
     * Whenever we successfully write a file we exit the loop.
     *
     * If WriteCardFile fails, prompt for a new filename and try again.
     * MyGetSaveFileName returns FALSE when the SaveFileName dlg is
     * cancelled.
     */
    while (TRUE)
    {
        if (fGetName)               /* get a new filename */
        {
            if (!MyGetSaveFileName (szFile, &fType))
            {
                /* SaveFileName dlg was cancelled */
                SetCurCard (iFirstCard);
                return (FALSE);
            }
        }

        /* save file, if can't save don't continue */
        /* NEVER SAVE AS OLD FORMAT in this case... */
#ifndef OLE_20
        WideCharToMultiByte (CP_ACP, 0, szFile, -1, aszFile, PATHMAX, NULL, NULL);

        if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, aszFile))
#else
        if (fOLE && OLE_OK != OleRenameClientDoc (lhcdoc, szFile))
#endif
            ErrorMessage (W_FAILED_TO_NOTIFY);

        if (WriteCardFile (szFile, fType))
            return TRUE;            /* successfully wrote the file */
        else
            fGetName = TRUE;        /* failed to write the file, try again */
    }
}

void MenuFileOpen (void)
{
  TCHAR szFile[PATHMAX];

    SaveCurrentCard (iFirstCard);
    if (MaybeSaveFile (FALSE))
    {
        OFN.lpstrDefExt       = szFileExtension;
        OFN.lpstrFilter       = szFilterSpec;
        OFN.lpstrCustomFilter = szCustFilterSpec;

        *szFile = (TCHAR) 0;

        OFN.lpstrFile         = szFile;
        OFN.lpstrInitialDir   = szLastDir;
        OFN.lpstrTitle        = szOpenCaption;
        OFN.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

        SetCurCard (iFirstCard); /* hold onto the current card */
        LockData (0);
        if (GetOpenFileName (&OFN))
            OpenNewFile (szFile);
        else if (CommDlgExtendedError ()) /* Assumes low memory. */
            IndexOkError (EINSMEMORY);
        UnlockData (0);
    }
}

int OpenNewFile (TCHAR szFile[])
{
#ifndef OLE_20
    CHAR  aszCardfile[60];
    CHAR  aszFile[PATHMAX];

    WideCharToMultiByte (CP_ACP, 0, szCardfile, -1, aszCardfile, 60, NULL, NULL);
    WideCharToMultiByte (CP_ACP, 0, szFile, -1, aszFile, PATHMAX, NULL, NULL);
#endif

    /* Register new document */
    if (lhcdoc)
        ReleaseClientDoc ();
#ifndef OLE_20
    if (fOLE && OLE_OK != OleRegisterClientDoc (aszCardfile,
                                                aszFile, 0L, &lhcdoc))
#else
    if (fOLE && OLE_OK != OleRegisterClientDoc (szCardfile,
                                                szFile, 0L, &lhcdoc))
#endif
    {
        ErrorMessage (W_FAILED_TO_NOTIFY);
        lhcdoc = (LHCLIENTDOC) NULL; // lhb tracks
        SendMessage (hIndexWnd, WM_COMMAND, NEW, 0L);
        return FALSE;
    }

    if (DoOpen (szFile))
    {
        SetCurCard (iFirstCard);
        SetNumOfCards ();
        return TRUE;
    }
    else
    {
        SendMessage (hIndexWnd, WM_COMMAND, NEW, 0L);
        return FALSE;
    }
}

int DoOpen (TCHAR *szFile)
{
    SetCursor (hWaitCurs);
    if (ReadCardFile (szFile))
    {
        SetCaption ();
        Fdelete (TempFile);
        MakeTempFile ();
        iTopCard = iFirstCard = 0;
        CurCardHead.flags = 0;
        InvalidateRect (hIndexWnd, NULL, TRUE);
        SetCursor (hArrowCurs);
        return TRUE;
    }
    else
    {
        SetCursor (hArrowCurs);
        return  FALSE;
    }
}

/*
 * Check cardfile signature
 * returns, -1 on error,
 *          TRUE if it is an old cardfile
 *          FALSE if it is not an old Cardfile
 */
BOOL CheckCardfileSignature (HANDLE fh, WORD * pfType)
{
  BYTE Signature[3];

    /* MGC is old, RRG is new file and DKO is Unicode file */
    MyByteReadFile(fh, Signature, 3);
    if (!memcmp(Signature, chMGC, 3))  /* old file? */
    {
        *pfType = OLD_FORMAT;
        idObjectMax = 0;
        return (TRUE);
    }
    else if (!memcmp (Signature, chDKO, 3) ||   /* Unicode file? */
             !memcmp (Signature, chRRG, 3))     /* or ANSI file? */
    {
        if (!memcmp (Signature, chDKO, 3))
           *pfType = UNICODE_FILE;
        else
           *pfType = ANSI_FILE;

        /* for new files OLE must be present */
        if (fOLE)
        {
            MyByteReadFile (fh, &idObjectMax, sizeof(DWORD));
            return (TRUE);
        }
        else
            IndexOkError (E_NEW_FILE_NOT_READABLE);  /* error */
    }
    else        /* not a valid cardfile */
        IndexOkError (ENOTVALIDFILE);

    return (FALSE);                                /* report failure */
}

/*
 * read in a file
 */
NOEXPORT int NEAR ReadCardFile (TCHAR *pName)
{
  HANDLE       fh;
  LPCARDHEADER Cards;
  WORD         i;
  TCHAR        szFile[PATHMAX];
  WORD         cNewCards;
  ULONG        fhHeaderLoc;

    lstrcpy (szFile, pName);

    /* If trying to open the currently open file, close it to avoid sharing
     * violation */
    if (fhMain != INVALID_HANDLE_VALUE && lstrcmpi(CurIFile, szFile) == 0)
    {
        MyCloseFile (fhMain);
        fhMain = INVALID_HANDLE_VALUE;
    }

    fReadOnly = FALSE;
    fh = MyOpenFile (szFile, NULL, OF_SHARE_DENY_WRITE | OF_READWRITE);
    if (fh == INVALID_HANDLE_VALUE)
    {
        if ((fh = MyOpenFile (szFile, NULL, OF_READ)) == INVALID_HANDLE_VALUE)
            return FALSE;

        /* check cardfile signature */
        if (!CheckCardfileSignature (fh, &fFileType))
        {
            CloseHandle(fh);
            return FALSE;
        }
        fReadOnly = TRUE;
        BuildAndDisplayMsg (W_OPENFILEFORREADONLY, szFile);
    }

    else
    {
        /* check cardfile signature */
        if (!CheckCardfileSignature (fh, &fFileType))
        {
            CloseHandle(fh);
            return FALSE;
        }
    }

    Hourglass (TRUE);

    /* read the number of cards in the file */
    MyByteReadFile (fh, &cNewCards, sizeof(WORD));
    if (!ExpandHdrs (cNewCards - cCards) ||
        !(Cards = (LPCARDHEADER) GlobalLock (hCards)))
    {
        MyCloseFile (fh);
        Hourglass (FALSE);
        return FALSE;
    }

    /* save first header location */
    if ((fhHeaderLoc = MyFileSeek (fh, 0L, 1)) == -1)
        return FALSE;

    cCards = cNewCards;

    // Lack of packing on WIN32...
    for (i = 0; i < cNewCards; i++)
    {
        MyByteReadFile (fh, &(CurCardHead.reserved), 6);
        MyByteReadFile (fh, &(CurCardHead.lfData), 4);
        MyByteReadFile (fh, &(CurCardHead.flags), 1);
        if (fFileType == UNICODE_FILE)
           MyByteReadFile (fh, CurCardHead.line, ByteCountOf(LINELENGTH+1));
        else
           MyAnsiReadFile (fh, CP_ACP, CurCardHead.line, LINELENGTH+1);
        Cards[i] = CurCardHead;
    }

    GlobalUnlock (hCards);

    /* go back to first header location */
    if (MyFileSeek (fh, fhHeaderLoc, 0) != -1)
        LinksExistCheck (fh, cCards, fFileType);

    fFileDirty = FALSE;
    if (fhMain != INVALID_HANDLE_VALUE)    /* If there exists an open file, close it. */
        MyCloseFile (fhMain);
    fhMain = fh;
    lstrcpy (CurIFile, szFile);
    Hourglass (FALSE);
    InitPhoneList (hListWnd, iFirstCard);

    return TRUE;
}

void MenuFileMerge (void)
{
  TCHAR szFile[PATHMAX];

    OFN.lpstrDefExt       = szFileExtension;
    OFN.lpstrFilter       = szFilterSpec;
    OFN.lpstrCustomFilter = szCustFilterSpec;
    *szFile = (TCHAR) 0;
    OFN.lpstrFile         = szFile;
    OFN.lpstrInitialDir   = szLastDir;
    OFN.lpstrTitle        = szMergeCaption;
    OFN.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    LockData (0);
    if (!fNoTempFile && GetOpenFileName (&OFN))
    {
        DoMerge (szFile);
        SetNumOfCards ();
    }
    UnlockData (0);
}

NOEXPORT BOOL NEAR DoMerge (TCHAR *szFile)
{

#if defined(WIN32)
    if (GetFileAttributes (szFile) == 0xFFFFFFFF)
        return IndexOkError(EINVALIDFILE);
#else
    if (MyOpenFile (szFile, szFile, OF_PARSE) == INVALID_HANDLE_VALUE)
        return IndexOkError (EINVALIDFILE);
#endif

    SetCursor (hWaitCurs);
    /* In phone mode no need to save the current card.
     * In card mode should save the current card */
    if (CardPhone == PHONEBOOK || SaveCurrentCard (iFirstCard))
    {
        if (MergeCardFile (szFile))
        {
            iTopCard = iFirstCard = 0;
            if (CardPhone == CCARDFILE)
                SetCurCard (iFirstCard);
            InvalidateRect (hIndexWnd, (LPRECT)NULL, TRUE);
        }
    }
    SetCursor (hArrowCurs);
}

/*
 * read in a file and merge it with the currently open file
 */
NOEXPORT int NEAR MergeCardFile (TCHAR *pName)
{
  HANDLE   fh;
  WORD     i;
  TCHAR    szFile[PATHMAX];
  WORD     cMergeCards;
  WORD     tSize;
  DWORD    oHdr;         /* file offset for the card header */
  ULONG    fhHeaderLoc;
  WORD     fType;

    /* open file */
    lstrcpy (szFile, pName);
    if ((fh = MyOpenFile (szFile, NULL,
                          OF_SHARE_DENY_WRITE | OF_READ)) == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!CheckCardfileSignature (fh, &fType))
    {
        MyCloseFile (fh);
        return FALSE;
    }

    /* read the number of cards in the file */
    MyByteReadFile (fh, &cMergeCards, sizeof(WORD));

    /* save first header location */
    if ((fhHeaderLoc = MyFileSeek (fh, 0L, 1)) == -1)
        return FALSE;

    Hourglass (TRUE);

    if (!ExpandHdrs (cMergeCards))
    {
        MyCloseFile (fh);
        Hourglass (FALSE);
        return FALSE;
    }

    fFileDirty = TRUE;
    for (i = 0; i < cMergeCards; ++i)
    {
        MyByteReadFile (fh, &(CurCardHead.reserved), 6);
        MyByteReadFile (fh, &(CurCardHead.lfData), 4);
        MyByteReadFile (fh, &(CurCardHead.flags), 1);
        if (fType == UNICODE_FILE)
           MyByteReadFile (fh, CurCardHead.line, ByteCountOf(LINELENGTH+1));
        else
           MyAnsiReadFile (fh, CP_ACP, CurCardHead.line, LINELENGTH+1);

        oHdr = MyFileSeek (fh, 0L, 1);  /* remember offset for the next header */

        /* seek to card data and read it */
        if (oHdr == -1 ||
            MyFileSeek (fh, CurCardHead.lfData, 0) == -1 ||
            !PicRead (&CurCard, fh, fType == OLD_FORMAT) ||  /* read object, */
            TextRead (fh, szText, fType) < 0)                /* then text    */
        {
            IndexOkError (E_FAILED_TO_READ_CARD);
            goto end;
        }

        /* Keep adding the current card, to the top */
        if (tSize = WriteCurCard (&CurCardHead, &CurCard, szText))
            iFirstCard = AddCurCard ();

        if (CurCard.lpObject)
            PicDelete (&CurCard);

        /* go to the next header */
        MyFileSeek (fh, oHdr, 0);
        if (!tSize)
        {
            fFileDirty = FALSE; /* skip merging */
            goto end;
        }
    }

    /* go back to first header location */
    if (MyFileSeek (fh, fhHeaderLoc, 0) == -1)
        goto end;

    LinksExistCheck (fh, cMergeCards, fType );

end:
    Hourglass (FALSE);
    MyCloseFile (fh);

    InitPhoneList (hListWnd, iFirstCard);
    return TRUE;
}

// LinksExistCheck
//
// Reads through file seeing if the user has to update links.
// We give the user one popup if so and exit.
//
// fh         - handle of file to check
// nCardCount - number of cards in file
// fType      - type of cardfile
//
// assume fh position is set to first header to read
//

void NEAR PASCAL LinksExistCheck (HANDLE fh, int nCardCount, WORD fType)
{
    CARDHEADER CardHeadTmp;
    ULONG      fhHeaderLoc;
    CARD       CardTmp;

    Hourglass (TRUE);

    while (nCardCount--)
    {
        MyByteReadFile (fh, &(CardHeadTmp.reserved), 6);
        MyByteReadFile (fh, &(CardHeadTmp.lfData), 4);
        MyByteReadFile (fh, &(CardHeadTmp.flags), 1);
        if (fType == UNICODE_FILE)
           MyByteReadFile (fh, CardHeadTmp.line, ByteCountOf(LINELENGTH+1));
        else
           MyAnsiReadFile (fh, CP_ACP, CardHeadTmp.line, LINELENGTH+1);

        /* save next header location */
        if ((fhHeaderLoc = MyFileSeek (fh, 0L, 1)) == -1)
            goto end;

        /* goto data location */
        if (MyFileSeek (fh, CardHeadTmp.lfData, 0) == -1)
            goto end;

        if (!PicRead (&CardTmp, fh, fType == OLD_FORMAT))
            goto end;
        if (CardTmp.lpObject)
        {
            if (CardTmp.otObject == LINK)
            {
               TCHAR szMsg[190];

                LoadString (hIndexInstance, IDS_UPDATELINK, szMsg, CharSizeOf(szMsg));
                MessageBox (hIndexWnd, szMsg, szCardfile, MB_OK);
                PicDelete (&CardTmp);
                goto end;
            }
            PicDelete (&CardTmp);
        }

        /* goto next header location */
        if (MyFileSeek (fh, fhHeaderLoc, 0) == -1)
            goto end;
    }

end:
    Hourglass (FALSE);
}
