/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calfile2.c
 *
 */


/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOKANJI
#define NOSCROLL
#define NODRAWTEXT
#define NOVIRTUALKEYCODES
#include "memory.h"
#include "cal.h"

extern BOOL f24Time;


CHAR * APIENTRY PFileInPath(
    CHAR *sz);


/**** FCopyToNewFile - copy the active dates from the specified file
      (original or change) to the new file.
****/

BOOL APIENTRY FCopyToNewFile (
     INT  idFileSource,            /* The id of the source file. */
     DR   *pdr,                    /* Pointer to DR to use as a buffer. */
     DD   *pddFirst,               /* Pointer to locked tdd. */
     DD   *pddMax)                 /* Pointer beyond locked tdd. */
     {
     register BOOL fSwap;
     register DD *pddCur;
     DL   dl;

     /* If the source file does not exist, there is no work to be done,
        so return right away, indicating success.  (The original file
        doesn't exist if we're as yet untitled, and the change file
        doesn't exist if it was impossible to create it (and the user
        was told of the error when the attempt to create the change
        file failed).)
      */

     if (idFileSource == IDFILEORIGINAL && !vfOriginalFile
             || idFileSource == IDFILECHANGE && !vfChangeFile)
         return (TRUE);

     /* Try to reopen the source file, returning an error if the open fails. */
     if (!FReopenFile (idFileSource, OF_PROMPT | OF_CANCEL | OF_REOPEN | OF_READWRITE))
          goto error1;


     /* See if the destination file is available without swapping diskettes.
        If it is, leave the source and destination files both open.
        If it is not available, leave both files closed.
      */

     if (fSwap = !FReopenFile (IDFILENEW, OF_REOPEN | OF_READWRITE))
          {
          /* Don't worry about errors closing the files.
             If there's really a problem, it will be detected when we try
             to use the files below.
           */
          FCloseFile (idFileSource);
          FCloseFile (IDFILENEW);
          }



     /*  Make a pass through the tdd looking for dates that are
         in the source file.  Copy these dates into the destination file.
      */
     for (pddCur = pddFirst; pddCur < pddMax; pddCur++)
          {
          /* If this date has not already been transferred, and it's not
             a special DL, and it resides in the source file we're working
             on, copy the data for the date.
          */
          if (pddCur -> dlSave == DLNOCHANGE
                   && (dl = pddCur -> dl) < DLSPECIALLOW
                   && ((dl & DLFCHANGEFILEMASK) && idFileSource == IDFILECHANGE
                   || !(dl & DLFCHANGEFILEMASK) && idFileSource == IDFILEORIGINAL))
               {
               if (!FReadDrFromFile (fSwap, pdr, dl))
                    {
                    /* An error occurred,
                       so we tell the caller to give up the Save.
                    */
                    goto error1;
                    }

               /* Remember the current DL, and set the new one to be the
                  place where we are about to write the date to in the
                  new file.
               */
               pddCur -> dlSave = dl;
               pddCur -> dl = vobkEODNew;

               /* Try to write the date to the new file.  If an error
                  occurs, the Save will be aborted.  It may be due
                  to a disk full condition or some I/O error.
               */
               if (!FWriteDrToFile (fSwap, IDFILENEW, pdr))
                    goto error1;
               }
          }


     /* If we weren't swapping, we need to close the files (and it's harmless
        to call FCloseFile for an unopen file).
        We ignore errors closing the source file since no modifications
        were made to it, we are done with it, and an error here has no
        effect on the integrity of the new file (which is what the user
        really cares about now).  And anyway, what sort of error could
        occur when closing a file that has only been read from?
        As for closing the new file, if an error occurs, we return it
        as an I/O error, which will cause the Save to be aborted.
        (Can't ignore error closing the new file, since an error here
        means there may be something wrong with it.)  By the way, according
        to Chris Peters, close cannot cause a Disk Full error since it
        cannot cause any disk space allocation.  Close does update the
        directory entry though.  Therefore, the only the only
        kind of error that can occur during a close is an I/O error.
        Chris says most programmers do not bother to check for errors
        when closing files.  I still think it's the prudent thing to
        to when closing a file that one has written to.
      */

     FCloseFile (idFileSource);
     return (FCloseFile (IDFILENEW));


error1:   /* An error occurred - close all files, and return FALSE. */
     FCloseFile (idFileSource);
     FCloseFile (IDFILENEW);
     return (FALSE);
     }




/**** FSaveFile - the guts of Save and Save As. ****/
BOOL APIENTRY FSaveFile (
    CHAR *szFileSpec,             /* Name of the file to save to. ANSI STRING. */
    BOOL fOverwrite)              /* TRUE means the old copy of the file with
                                     same name in the same directory as the new
                                     file should be overwritten by the new one
                                     if the Save is successful.
                                   */
     {
     DD   *pddFirst, *pddMax, *pddCur;
     WORD idr;
     INT  FileHandle, fp;
     DR   *pdr;
     BOOL fOk;
     DL   dl;

     /* Show the hour glass cursor. */
     HourGlassOn ();

     if ((fp = M_lopen((LPSTR)szFileSpec, 2)) < 0)   /* fgd 9/20/89  */
         fp = M_lcreat((LPSTR)szFileSpec, 0);                 /* _lopen & _lcreat */
                                                             /* use ANSI chrs */
     if (fp >  0)
        M_lclose(fp);
     else
        {
        AlertBox (vrgsz[IDS_NOCREATE], szFileSpec,
                  MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
        HourGlassOff();
        return FALSE;
        }

     /* Force edits to be recorded prior to flushing the DRs.  Note that
        leaving the focus as is will work OK since the DR corresponding
        to vidrCur remains in memory during the Save.  This means that
        if the focus is changed later, the correct date will still be
        around for use by StoreQd or StoreNotes.  Since Save does not
        change the mode (day or month) we want the focus to stay where
        it was, so we don't do a CalSetFocus ((HWND)NULL) here.
      */
     RecordEdits ();


     /* Create the new file as a temporary - we will rename it when the
        save is finished.  Note that we must force the temp file to
        be on the drive we want the new file on, since, of course, we can't
        rename across drives.  Surprizingly though, we can rename across
        directories.
      */
     if (!FCreateTempFile (IDFILENEW, GetDrive (szFileSpec) | TF_FORCEDRIVE))
          goto error1;


     /* Calculate the number of BK required to hold the header and the
        index.  Set the end of data of the new file in
        order to reserve the required space.  Note that we don't actually
        write the header or the index until after the dates have been
        successfully written to the new file.  This is done for two reasons:

        1) By not writing the magic number until the end, we reduce the
           risk of ever trying to use a file that has the correct magic
           number but which is corrupt.  (In other words, if an error
           causes the Save to be aborted, and for some reason the new
           file cannot be deleted, it probably won't look like a valid
           calendar file (although I suppose the disk space allocated
           to the file could contain the correct magic number - but I
           am not going to to bother to write something into the first byte
           of the file just to handle this unbelievably pathological case.

        2) More importantly - we cannot write the index now because
           it does not yet contain the new DLs.  These will be set as
           the dates are moved into the new file.

        Note that adding CBBK - 1 prior to dividing by CBBK has the
        effect of rounding up to the next bk.
      */
     vobkEODNew = 1 + (vcddUsed * sizeof (DD) + CBBK - 1) / CBBK;



     /* Mark each DD to indicate that its data has not yet been copied to
        the new file.
      */
     for (pddMax = (pddCur = TddLock ()) + vcddUsed; pddCur < pddMax; pddCur++)
          pddCur -> dlSave = DLNOCHANGE;

     TddUnlock ();


     /* Now we flush any dates that are in memory to the new file. */
     fOk = FFlushDr ();


     /* The tdd may have become smaller during FFlushDr since we may
        have deleted some empty DDs.  Lock it, and set up First and Max
        pointers.  These pointers are needed for error recovery, so we
        must do this prior to bailing out if FFlushDr failed.
      */
     pddMax = (pddFirst = TddLock ()) + vcddUsed;


     /* Bail out if FFlushDr encountered an error trying to write out
        one of the DRs.
      */
     if (!fOk)
         goto error2;


     /* Find a free DR to use as a buffer, and lock it.  */
     idr = IdrFree ();
     pdr = PdrLock (idr);


     /* Copy the dates from the change file to the new file, then
        copy the dates from the original file to the new file.
      */
     fOk = FCopyToNewFile (IDFILECHANGE, pdr, pddFirst, pddMax)
      && FCopyToNewFile (IDFILEORIGINAL, pdr, pddFirst, pddMax);

     /* Make sure the DR we used as a buffer looks free, then
        unlock it.
     */
     pdr -> dt = DTNIL;
     DrUnlock (idr);

     /* If dates were copied OK, then try to write the header.  If either
        operation failed, abort the Save.
     */
     if (!fOk || !FWriteHeader (pddFirst))
          goto error2;

     /* Finished with the tdd - unlock it. */
     TddUnlock ();

     /* Reconnect the DRs with their DDs. */
     Reconnect (TRUE);


      /* Delete the file being overwritten.  Note that this is the
         file on the same device and directory with the same name
         as what our new file is to be named - it is not necessarily
         the original file.  However, we use the reo of the original
         file since it is available at this point.  So we call
         OpenFile to get the user to swap diskettes if necessary,
         and then we delete the file.
         If an error occurs during the delete, ignore it.
         The new file is in good shape, the only problem is that
         we won't be able to rename it to it's correct name since
         we couldn't delete the old copy.  This will get detected
         when we attempt the rename (below), and the user will be
         told about the problem then.  Bear in mind that if we can't
         delete the old file, we probably can't access it at all,
         and since it could be the original file, it would be a bad
         idea to abort the Save now.
       */
      if ((FileHandle = MOpenFile ((LPSTR)szFileSpec,
                            (OFSTRUCT FAR *)&OFStruct [IDFILEORIGINAL],
                            OF_PROMPT | OF_CANCEL | OF_READWRITE))
           != -1)
           {
           M_lclose (FileHandle);
           FDosDelete (OFStruct [IDFILEORIGINAL].szPathName);
           }

     /* Rename the new file. */
     fOk = FReopenFile (IDFILENEW, OF_REOPEN | OF_PROMPT | OF_CANCEL | OF_READWRITE);
     FCloseFile (IDFILENEW);

     if (!fOk || FDosRename (OFStruct [IDFILENEW].szPathName, szFileSpec))
          {
          /* Could not rename the file.  Tell the user that his data is
             in the temp file with the funny name.  Also change szFileSpec
             to point to the temporary file name since this is what we will
             be using now.
             This should rarely, if ever, occur.  What could
             have gone wrong?  An I/O error perhaps while trying to
             rewrite the directory entry?  Anyway, we do our best
             to recover from the situation.
           */

          /* delete the file we created which the user wanted to save as.
           * bug fix. we were leaving it in existence with 0 length.
           * don't delete if we were just doing a save.
           * 21-Jun-1987. davidhab
           */

          if (!fOverwrite)
              FDosDelete(szFileSpec);

          szFileSpec = OFStruct [IDFILENEW].szPathName;

          AlertBox (vszRenameFailed, szFileSpec,
                    MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
          }

     /* The new file is now the original file.  We need to set up
        OFStruct [IDFILEORIGINAL] accordingly.  The simplest way to do this
        is to call OpenFile using szFileSpec - this will set up the reo.
        Call with OF_EXIST since this means the file will not be left open.
        Don't bother to check for errors since all we care about at this
        point is getting the reo set up, and this will presumably happen
        no matter what.
      */
     MOpenFile ((LPSTR)szFileSpec, (OFSTRUCT FAR *)&OFStruct [IDFILEORIGINAL], OF_EXIST);

     /* Delete and recreate the change file.  If the delete fails, ignore
        it.  If the new one can't be created CreateChangeFile will tell
        the user about the problem.  (Note that it may be possible to
        create the new change file even if the old one can't be deleted
        since a different temporary file name will be used.)
        The point is, the Save has successfully completed, and not being
        able to delete the old change file or create the new change file
        should not abort the Save at this point.
        The only problem is, the user may think the Save was not successful
        if he sees the error message generated by CreateChangeFile.  I am
        assuming that the chances of this failure occuring are extremely
        remote, and in any case the result is not catastrophic.
      */
     CreateChangeFile ();

     /* Everything is clean now. */
     vfDirty = FALSE;

     /* Set the new title. */
     SetTitle (szFileSpec);

     /* The waiting is over. */
     HourGlassOff ();

     return (TRUE);


error2:   /* Error while trying to flush the DRs, copy the dates,
             or write the header.
           */
     /* Set the DLs back to their old values. */
     for (pddCur = pddFirst; pddCur < pddMax; pddCur++)
          {
          if ((dl = pddCur -> dlSave) != DLNOCHANGE)
               pddCur -> dl = dl;
          }

     /* Unlock the tdd. */
     TddUnlock ();

     /* Try to delete the new file.  If an error occurs, ignore it.
        We are already going to tell the user that a problem has occurred,
        and there is nothing we can do about it if the new file (which
        has a funny name) can't be deleted.
      */
     FDeleteFile (IDFILENEW);

     /* delete the file we created which the user wanted to save as.
      * bug fix. we were leaving it in existence with 0 length.
      * don't delete if we were doing just a save.
      * 21-Jun-1987. davidhab
      */
     if (!fOverwrite)
         FDosDelete(szFileSpec);


     /* Reconnect the DRs with their DDs. */
     Reconnect (FALSE);

error1:   /* Error attempting to create the new file. */
     /* Tell the user that the Save failed. */
     AlertBox (vszSaveFailed, (CHAR *)NULL,
               MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);

     /* The waiting is over. */
     HourGlassOff ();

     return (FALSE);

     }




/**** Reconnect - reconnect DRs with their DDs, and make sure the
      selected date is in memory.
****/

VOID APIENTRY Reconnect (BOOL fSaveOk)
     {
     register WORD idr;
     register DR *pdr;
     INT  itdd;
     HWND   hwndFocus;

     /* Look at all the DRs. */
     for (idr = 0; idr < CDR; idr++)
          {
          /* Get a pointer to the DR. */
          pdr = PdrLock (idr);

          /* Skip over DRs that are not in use. */
          if (pdr -> dt != DTNIL)
               {
               /* Reconnect it to its DD.  Note that there must be a
                  DD for it since when FFlushDr deleted a DD it had
                  already marked the DR as free (which this one is not).
               */
               FSearchTdd (pdr -> dt, &itdd);
               (TddLock () + itdd) -> idr = idr;
               TddUnlock ();

               /* If the Save succeeded then the DR is now clean.  If
                  it failed we leave the dirty flag alone (it could be
                  be either dirty or clean, depending on it's state
                  before the Save was attempted).
               */
               if (fSaveOk)
                    pdr -> fDirty = FALSE;
               }

          DrUnlock (idr);
          }

     /* We need to make sure the selected date is in memory.  We know
        that this call to FGetDateDr cannot fail since the date is
        already in one of the DRs we just reconnected, or it has no
        data associated with it.  In either case, it cannot require
        a disk read so no error can occur.  It may no longer be in the
        tdd (FFlushDr may have deleted it), but in that case FGetDateDr
        will build a new DD for it, and we know there will be room for
        the new DD since it was in the tdd before the Save started
        and the DD could only have gotten smaller not larger.
        Also note that FGetDateDr will not have to kick out another
        date since the selected date was in a DR before the Save and
        we have not reassigned any of them, so no disk writes will
        be done either.  So no errors can occur.
     */
     hwndFocus = GetFocus();
     FGetDateDr (DtFromPd3 (&vd3Sel));
     CalSetFocus(hwndFocus);
     }




/**** GetDrive - extract the drive letter from a file spec.
      if none specified, return the current drive.
****/

INT  APIENTRY GetDrive (CHAR *szFileSpec)
     {
     CHAR *pch;

     /* Skip leading spaces. */
     pch = szFileSpec;
     SkipSpace (&pch);

     /* If the second character is a colon, the first character is a drive
        letter.  Otherwise return the current drive.
      */
#ifdef DBCS
     if( IsDBCSLeadByte(*pch) )
         return GetCurDrive();
     else
         return (*(pch + 1) == ':' ? *pch : GetCurDrive ());
#else
     return (*(pch + 1) == ':' ? *pch : GetCurDrive ());
#endif
     }





/**** FFlushDr - iterate the DRs to:
      1) free up empty ones (no data) and get rid of the associated DD
         if it too is empty.
      2) write out non-empty dirty DRs to the new file.

      Return TRUE if no errors, FALSE if an error
      occurs while trying to flush one of the DRs.
****/
BOOL APIENTRY FFlushDr ()
     {
     WORD idr;
     register DR *pdr;
     BOOL fOk;
     INT  itdd;
     register DD *pdd;

     /* Look at all the DRs - stop if an error occurs while trying
        to write one of them to the new file.
      */

     for (idr = 0, fOk = TRUE; idr < CDR && fOk; idr++)
          {
          /* Get a pointer to the DR. */
          pdr = PdrLock (idr);

          /* Skip over DRs that are not in use. */
          if (pdr -> dt != DTNIL)
               {
               /* Break the connection between the DD and the DR.
                  Note - the owner of the DR must be in the
                  tdd, so don't bother checking the return value
                  of FSearchTdd.
               */
               FSearchTdd (pdr -> dt, &itdd);
               (pdd = TddLock () + itdd) -> idr = IDRNIL;

               if (pdr -> cbNotes + pdr -> cbTqr == 0)
                    {
                    /* The DR is empty.  Free it up. */
                    pdr -> dt = DTNIL;

                    /* Say the date is no longer on disk either.  Since
                       there is no longer any data associated with the
                       date, even if the Save fails we want the DL to
                       be DLNIL, so make both dlSave and dl DLNIL.
                    */
                    pdd -> dlSave = pdd -> dl = DLNIL;

                    /* The DD may now be empty (if the data is not marked).
                       If this is the case, get rid of
                       the DD since we don't want to write it out to
                       the new file.  Even if the Save fails this is OK.
                       If it's the selected date it will get reinserted
                       into the DD, and if it's not the selected date it
                       it not needed and will get reinserted if the user
                       ever switches to it.
                       Note that it's OK to call DeleteEmptyDd even though
                       the tdd is locked since if it does a ReAlloc it
                       will only be to make the tdd smaller.
                    */
                    DeleteEmptyDd (itdd);
                    }
               else
                    {
                    /* The DR is not empty - see if it's dirty. */
                    if (pdr -> fDirty)
                         {
                         /* Remember old disk location, set new one. */
                         pdd -> dlSave = pdd -> dl;
                         pdd -> dl = vobkEODNew;

                         /* Write it to the new file.  Note that we
                            intentionally leave fDirty TRUE.  This is
                            necessary for putting things back the way
                            they were if the Save fails.
                         */
                         fOk = FWriteDrToFile (TRUE, IDFILENEW, pdr);
                         }
                    }

               /* Unlock the tdd. */
               TddUnlock ();
               }

          /* Unlock the DR. */
          DrUnlock (idr);
          }

     return (fOk);

     }


/**** FCloseFile - close he specified file if it's open.  Return TRUE
      if the file is successfully closed. Assume this happens since
      M_lclose does not give us an error return anyway.

      Note -
      According to Chris Peters, close cannot cause a Disk Full error since
      it cannot cause any disk space allocation.  Close does update the
      directory entry though.  Therefore, the only the only
      kind of error that can occur during a close is an I/O error.
      Chris says most programmers do not bother to check for errors
      when closing files.  I still think it's the prudent thing to
      to when closing a file that one has written to.
****/

BOOL APIENTRY FCloseFile (INT idFile)
    {
    INT FileHandle;

     if ((FileHandle = hFile[idFile]) == -1)	// Q: no valid handle?
          return (TRUE);						//   Y: no need to close file
     else
          {
          hFile[idFile] = -1;					
          M_lclose(FileHandle);					// close file,
          return TRUE;							// return success always
          }
     }




/**** FWriteHeader - write out the file header and index.
      Return TRUE if successful, FALSE if an error occurs.
****/

BOOL APIENTRY FWriteHeader (DD*pddFirst)
{
    BYTE bkBuf [CBBK];
    register BYTE *pb;
    register BOOL fOk;
    INT FileHandle;

     /* Reopen the file, seek to the beginning of the second BK, and write
        the index (tdd).
     */
     fOk=FReopenFile(IDFILENEW, OF_REOPEN | OF_PROMPT | OF_CANCEL | OF_READWRITE)
          && M_llseek ((FileHandle = hFile [IDFILENEW]), (LONG)CBBK, 0) != -1
          && FWriteFile (FileHandle, (BYTE *)pddFirst, (WORD)(vcddUsed * sizeof (DD)));

	if (fOk)
	{
		//- Save Header: Need a 16 bit word to copy the header info into.
		WORD  wTemp;

		/* Build a header bk containing the magic number, the
		   size of the index, and the options.
		   Unused bytes are set to 0 and are reserved for future use.
		*/
		FillBuf (bkBuf, CBBK, 0);

		pb = BltByte ((BYTE *)vrgbMagic,       (BYTE *)bkBuf, CBMAGIC);

		//- Save Header: Copy to temporary 16 bit storage then assign.
		wTemp = (WORD)vcddUsed;
		pb = BltByte ((BYTE *)&wTemp,       (BYTE *)pb, CBCDD);

		wTemp = (WORD)vcMinEarlyRing;
		pb = BltByte ((BYTE *)&wTemp, 	(BYTE *)pb, CBMINEARLYRING);

		wTemp = (WORD)vfSound;
		pb = BltByte ((BYTE *)&wTemp,        (BYTE *)pb, CBFSOUND);

		wTemp = (WORD)vmdInterval;
		pb = BltByte ((BYTE *)&wTemp,    	(BYTE *)pb, CBMDINTERVAL);

		wTemp = (WORD)vcMinInterval;
		pb = BltByte ((BYTE *)&wTemp,  	(BYTE *)pb, CBMININTERVAL);

		wTemp = (WORD)vfHour24;
		pb = BltByte ((BYTE *)&wTemp,       	(BYTE *)pb, CBFHOUR24);

		pb = BltByte ((BYTE *)&vtmStart,      (BYTE *)pb, CBTMSTART);

		/* Write the header into the first BK of the file. */
		fOk = M_llseek (FileHandle, (LONG)0, 0) != -1
				&& FWriteFile (FileHandle, bkBuf, CBBK);
	}

     /* Close the file.  Note that calling FCloseFile is Ok even if the
        file was not successfully opened.  Also note that the FCloseFile
        call must be separate from the other operations and must be done
        before checking fOk since we want the file to be closed regardless
        of whether an error has occurred.
     */
    return (FCloseFile (IDFILENEW) && fOk);
}




/**** FWriteFile - write to file.
      If a disk full error occurs, put up a message box for the user.
      If some other type of error occurs (I/O error), assume that the user
      has already seen the INT 24 Abort, Retry, Ignore dialog, so no need
      to put up another message.  Return FALSE if error occurs, TRUE if
      write is successful.
****/

BOOL APIENTRY FWriteFile (
     INT  FileHandle,         /* Handle of file to write to. */
     BYTE *pb,           /* Pointer to bytes to be writeen. */
     WORD cb)            /* COunt of bytes to write. */
     {
     /* Do the write.  Return TRUE if it's successful.
        Note that due to some ambiquity about what write returns
        if it runs out of disk space (the C manual talks about -1 for
        an error, but at the same time says the count of bytes written
        could be less than specified but positive if the write runs
        out of disk space), we call it a bad write if the return value
        (count of bytes written) is not cb (the number of bytes we say to
        write).  This works for both cases.
      */
     if ((WORD)_lwrite (FileHandle, (LPSTR)pb, cb) == cb)
          return (TRUE);

     /* Put up disk full message if that's the error that occurred. */
     /* Assume out of disk space. */
#ifdef DISABLE
     if (_errno == ENOSPC)
          {
#endif
          /* Need to make this system modal since the file is still
             open and it's against the rules to relinquish control with
             files open.
          */
          AlertBox (vszDiskFull, (CHAR *)NULL,
               MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
#ifdef DISABLE
          }
#endif

     return (FALSE);

     }




/**** FDeleteFile - delete the specified file.  Return TRUE if successful,
      FALSE if an error occurs.
****/
BOOL APIENTRY FDeleteFile (INT idFile)
    {
    register BOOL fOk;

     /* Make sure the file exists, prompting for it if necessary.  Then
        try to delete it.
     */
     fOk=FReopenFile(idFile, OF_REOPEN | OF_PROMPT | OF_CANCEL |  OF_READWRITE);

     FCloseFile (idFile);
     return (fOk && FDosDelete (OFStruct [idFile].szPathName));
     }




/**** FReopenFile - */
BOOL APIENTRY FReopenFile (
     INT  idFile,
     WORD wFlags)
     {
     hFile[idFile]= MOpenFile ((LPSTR)vrgsz [IDS_GIVEMEFIRST + idFile],
                         (OFSTRUCT FAR *)&OFStruct[idFile], wFlags);

     return (hFile[idFile]!=-1);
     }




/**** SetTitle ****/
VOID APIENTRY SetTitle (CHAR *sz)
     {
     /* We know that the sz we are being passed has been through
        DlgCheckFilename at some point, so it has to contain a valid
        filename and extension and no longer has any trailing blanks.
        Therefore we know that this buffer is large enough.
     */
     CHAR szWindowText [CCHSZWINDOWTEXTMAX];

     /* Set the flag indicating if there is an open calendar file. */
     if (vfOriginalFile = sz != vszUntitled)
          {
          /* Convert file name to upper case.  Note - "(untitled)" should
             not be converted to upper case.
          */
          lstrcpy( vszFileSpec, sz);
          AnsiUpper ((LPSTR)vszFileSpec);
          }

     else
        lstrcpy (vszFileSpec, sz);


     /* Strip the path name, build the title string,
        and tell Windows about it.
     */
     lstrcat (lstrcpy (szWindowText, vszCalendarDash), PFileInPath (vszFileSpec));
     SetWindowText (vhwnd0, (LPSTR)szWindowText);
     }




/**** FCondClose - conditionally close the specified file. */

BOOL APIENTRY FCondClose (
     BOOL fClose,        /* FALSE means don't close the file, TRUE means do. */
     INT  idFile)        /* The id of the file to close. */
     {
     /* If we're not supposed to close the file, just return TRUE.
         If we are, return the result of FCloseFile.
     */
     return (!fClose || FCloseFile (idFile));
     }




/**** CleanSlate ****/

VOID APIENTRY CleanSlate (BOOL fShowDate)
     {
     register WORD idr;

     /* Show the hour glass cursor. */
     HourGlassOn ();

     /* Take the focus away so that the current edits get recorded now
        and not later when the slate is suppose to be clean.  (We don't
        care about the current edits since we are about to throw away
        everything, but if we were to leave the focus on an edit control,
        its contents would get stored when the focus gets set later.  This
        would make the file dirty with data that's supposed to
        be gone.)
      */
     CalSetFocus ((HWND)NULL);


     /* Say everything is clean. */
     vfDirty = FALSE;

     /* Say there is no next alarm, and forget about any unacknowledged
        ones.
      */
     vftAlarmNext.dt = vftAlarmFirst.dt = DTNIL;

     /* Mark all DR as available. */
     for (idr = 0; idr < CDR; idr++)
          {
          PdrLock (idr) -> dt = DTNIL;
          DrUnlock (idr);
          }

     /* Get rid of all entries in the tdd. */
     InitTdd ();

     /* Set all options to their default values. */
     vcMinEarlyRing = 0;
     vfSound = TRUE;
     vmdInterval = MDINTERVAL60;
     vcMinInterval = 60;
     InitTimeDate(vhInstance, 0);
     vfHour24 = f24Time;
     vtmStart = 7 * 60;       /* changed from 8 to 7 11/27/88 */

     /* Say there is no current file. */
     SetTitle (vszUntitled);

     /* Delete old change file (if there is one), and create the new
        one.  If a Save has just been done, the change file has
        already been recreated, but I guess this is OK.  We must
        recreate the change file now because we don't know whether
        the the user said to ignore old edits (in which case the
        Save didn't get done and the old change file is still around).
        This CreateChangeFile is also needed here for the call from
        CalInit, in which case it creates the change file for the
        first time.
     */
     CreateChangeFile ();

     /* Go into day mode for today.  Set the selected month to an
        impossible one - this will cause SwitchToDate (called by
        DayMode) to call SetUpMonth.  Note that the SwitchToDate call
        cannot fail since it will not attempt to read from a file.
     */
     if (fShowDate)
          {
          vd3Sel.wMonth = MONTHDEC + 1;
	  DayMode (&vd3Cur);
          }
     /* The waiting is over. */
     HourGlassOff ();
     }




#if 0
  /* old version commented out - L.Raman */
/**** OpenCal - open a calendar file. ****/

VOID APIENTRY OpenCal ()
     {
     CHAR szNewFileSpec [CCHFILESPECMAX];
     INT wResult;
     CHAR szExtension[8];

     lstrcpy((LPSTR)szExtension, (LPSTR)"\\*");
     lstrcat((LPSTR)szExtension, (LPSTR)vrgsz[IDS_FILEEXTENSION]);

     wResult = cDlgOpen (vhInstance, vhwnd0, IDD_OPEN,	IDCN_EDIT,
	    	             IDCN_LISTBOX, IDCN_PATH, szExtension, CCHFILESPECMAX,
    	    	         szNewFileSpec, (OFSTRUCT *)&OFStruct[IDFILEORIGINAL], &hFile[IDFILEORIGINAL]);


     switch (wResult)
        {
	case 1: /* hit ok, legal name */
            LoadCal ();
            break;

	case 2:  /*hit ok, illegal name */
            AlertBox (vszBadFileName, szNewFileSpec, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
            break;

	case -1:  /*out of memory   */
            AlertBox (vszOutOfMemory, (CHAR *) NULL, MB_SYSTEMMODAL | MB_OK | MB_ICONHAND);
	    break;
         }

     }
#endif

VOID APIENTRY OpenCal ()
{
     CHAR szNewFileSpec [CCHFILESPECMAX] = "";
     extern CHAR szLastDir[];

     /* set up the variable fields of the OPENFILENAME struct. (the constant
      * fields have been sel in CalInit()
      */
     vOFN.lpstrFile	    = szNewFileSpec;
     vOFN.lpstrInitialDir   = szLastDir;
     vOFN.lpstrTitle	    = vszOpenCaption;
     vOFN.Flags 	    = vfOpenFileReadOnly ? OFN_READONLY : 0L;

     /* All long pointers should be defined immediately before the call.
      * L.Raman - 2/12/91
      */
     vOFN.lpstrFilter	    = (LPSTR)vszFilterSpec;
     vOFN.lpstrCustomFilter = (LPSTR)vszCustFilterSpec;
     vOFN.lpstrDefExt 	    = vszFileExtension + 1;   /* point to "CAL" */

     if ( GetOpenFileName ((LPOPENFILENAME)&vOFN))
     {
	 /* set the read-only flag depending on the state of Flags field */
	 vfOpenFileReadOnly = (vOFN.Flags & OFN_READONLY ? TRUE : FALSE);
	 hFile [IDFILEORIGINAL] = MOpenFile ( vOFN.lpstrFile,
					     (LPOFSTRUCT)&OFStruct [IDFILEORIGINAL],
					     OF_PROMPT+OF_CANCEL);
	 LoadCal ();
     }

     /* GetOpenFilename doesn't return if the supplied filename is bad */
}


/**** LoadCal ****/

VOID APIENTRY LoadCal ()
     {
     INT i;
     INT FileHandle;
     BYTE bkBuf [CBBK];
     BOOL fOk;
     WORD cb;
     INT  cdd;
     DD * pdd;
     D3   d3First;
     BOOL fLoadToday = TRUE;  /* Intilaise it; Fix for a Bug by SANKAR */
     CHAR szNewFileSpec [CCHFILESPECMAX];
     //- Save Header: Need temporary 16 bit buffer.
     WORD wTemp;

     /* Show the hour glass cursor. */
     HourGlassOn ();

     /* Start with a clean slate.  Note that if the load fails, we will
        end up in untitled mode, and this is OK since even if there is
        another file open, it has either been saved or the user said not
        to save the changes.  In other words, the user has said it is
        Ok to switch to another file, so there is no reason to be concerned
        about what happens to the current file if the new file can't be
        loaded.
     */
     CleanSlate(FALSE);

     if ((FileHandle = hFile[IDFILEORIGINAL]) == -1)
		goto loadfin;

     /* try opening file in READWRITE mode. If it fails, try opening it
	in READ mode. If it works the file must have been marked read only
	Set vfOpenFileReadOnly so that Calendar knows about this and does
	not attempt to save in the end */

     /* the file must be closed before a BLOCKWRITE call can successfully
        be executed.  The call to FCloseFile() was moved from inside the
        following if conditional to in front of it.  1 Sept 1989 clarkc */

     FCloseFile (IDFILEORIGINAL);
     lstrcpy((LPSTR)szNewFileSpec, (LPSTR)OFStruct[IDFILEORIGINAL].szPathName);

// on win32, don't do the OemToAnsi -- ianja
//     OemToAnsi((LPSTR)OFStruct[IDFILEORIGINAL].szPathName, (LPSTR)szNewFileSpec);


     if((FileHandle = MOpenFile (szNewFileSpec,
		       (OFSTRUCT FAR *)&OFStruct[IDFILEORIGINAL],
                        OF_READWRITE|BLOCKWRITE))== -1)
         {
         FileHandle = MOpenFile (szNewFileSpec,
		       (OFSTRUCT FAR *)&OFStruct[IDFILEORIGINAL],
		       OF_READ);
	 vfOpenFileReadOnly = TRUE;
         }
     /*
      *  The opened file's handle must be stored sothat the file will get
      *  closed when FCloseFile() is called latter in this function;
      * Otherwise the file handle in hFile[] will be -1 and the file will
      * remain open cause "sharing violations" when run under SHARE.EXE.
      * Fix for Bugs #5135, #5305 and #1848  --SANKAR-- 10-13-89
      */
     hFile[IDFILEORIGINAL] = FileHandle;

     if (!(fOk = M_llseek (FileHandle, (LONG)0, 0) != -1
            && _lread (FileHandle, bkBuf, CBBK) == CBBK))
        goto error1;


     /* Check the magic number. */

       for (i = 6; i < CBMAGIC; i++)
	  {
	  if (bkBuf [i] != vrgbMagic [i])
	       {
               /* File is open so this must be system modal. */
		AlertBox (vszNotCalFile, (CHAR *)NULL,
                MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
               goto error0;
               }
	  }

     /* Try to make the tdd large enough to hold the DDs from the file.
        Note that if successful, FGrowTdd will set vcddUsed to cdd since
        CleanSlate had set it to 0 by calling InitTdd.
     */
     BltByte ((BYTE *)(bkBuf + OBCDD), (BYTE *)&wTemp, CBCDD);
     cdd = (INT)wTemp;
     if (!FGrowTdd (0, cdd))
          {
          /* Couldn't grow the tdd.  The error message has already been
             displayed by FGrowTdd.
          */
          goto error0;
          }

     /* Set up the rest of the items from the header. */
     //- Get Header: Store the values in temporary 16-bit buffer.
     BltByte ((BYTE *)(bkBuf + OBMINEARLYRING),(BYTE *)&wTemp,  CBMINEARLYRING);
     vcMinEarlyRing = (INT)wTemp;

     BltByte ((BYTE *)(bkBuf + OBFSOUND),      (BYTE *)&wTemp, CBFSOUND);
     vfSound = (INT)wTemp;

     BltByte ((BYTE *)(bkBuf + OBMDINTERVAL),  (BYTE *)&wTemp, CBMDINTERVAL);
     vmdInterval = (INT)wTemp;

     BltByte ((BYTE *)(bkBuf + OBMININTERVAL), (BYTE *)&wTemp, CBMININTERVAL);
     vcMinInterval = (INT)wTemp;

     BltByte ((BYTE *)(bkBuf + OBFHOUR24),     (BYTE *)&wTemp, CBFHOUR24);
     vfHour24 = (INT)wTemp;

     BltByte ((BYTE *)(bkBuf + OBTMSTART),     (BYTE *)&vtmStart, CBTMSTART);

     /* Set format of time display according to vfHour24 */
     InitTimeDate(vhInstance, vfHour24 ? GTS_24HOUR : GTS_12HOUR);

     /* Read in the tdd, and close the file.  Ignore errors on close
        (very unlikely) because if the data has been successfully read
        there is no reason to give up now.  If there's a real problem
        we should find out about it when we try to read dates from the
        file later on.
     */
     cb = cdd * sizeof (DD);
     fOk = _lread (FileHandle, (LPSTR)TddLock (), cb) == (DWORD)cb;
     TddUnlock ();
     FCloseFile (IDFILEORIGINAL);

     if (!fOk)
          goto error2;

     /* if can't load today, goto first record in file. 27-Oct-1987. davidhab */
     fLoadToday = TRUE;
     if (!FGetDateDr (vftCur.dt))
        {
        fLoadToday = FALSE;
        pdd = TddLock();

        if (!FGetDateDr(pdd->dt))
            {
            TddUnlock();
            goto error2;
            }

        GetD3FromDt(pdd->dt, &d3First);
        TddUnlock();
        }

     /* Arm the first alarm >= the current time.
        Also see if the first alarm must go off immediately.
      */
     GetNextAlarm (&vftCur, &vftCur, TRUE, (HWND)NULL);
     AlarmCheck ();

     /* The load went Ok - make this the open calendar file. */
     SetTitle ((CHAR *)OFStruct [IDFILEORIGINAL].szPathName);

loadfin:
     /* Go into day mode for today.  Set the selected month to an
        impossible one - this will cause SwitchToDate (called by
        DayMode) to call SetUpMonth.  Note that the SwitchToDate call
        cannot fail since we have already read in the data for today
        if there is any.
      */
     vd3Sel.wMonth = MONTHDEC + 1;
     if (fLoadToday)
         DayMode (&vd3Cur);
     else
         DayMode (&d3First);

     /* The waiting is over. */
     HourGlassOff ();
     return;



error2:   /* Error occurred while attempting to read the tdd or the data
             for today.
          */
     /* Get rid of the partially loaded garbage. */
     CleanSlate (FALSE);


error1:   /* Error occurred attempting to read the header - nothing
             has been changed, so no need to call CleanSlate again.
          */
     /* Error attempting to read file.  File is still open, so this
        alert must be system modal.
     */
     MessageBeep(0);

     AlertBox (vszCannotReadFile, (CHAR *)NULL,
      MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);

error0:   /* Not a calendar file, or couldn't make tdd big enough.  The
             error message has already been displayed so the only thing
             we must do is close the file.  It's OK for other cases
             (where the file has already been closed) to come through
             here since FCloseFile properly handles an already closed file.
          */
     /* Make sure the file is closed. */
     FCloseFile (IDFILEORIGINAL);

     goto loadfin;
     }

/* ** Given filename which may or maynot include path, return pointer to
      filename (not including path part.) */
CHAR * APIENTRY PFileInPath(register CHAR *sz)
    {
    register CHAR *pch;

    /* Strip path/drive specification from name if there is one */
    pch = (CHAR *)AnsiPrev((LPSTR)sz, (LPSTR)(sz + lstrlen((LPSTR)sz)));
    while (pch > sz)
        {
	pch = (CHAR *)AnsiPrev((LPSTR)sz, (LPSTR)pch);
        if (*pch == '\\' || *pch == ':')
            {
	    pch = (CHAR *)AnsiNext((LPSTR)pch);
	    break;
            }
        }
    return(pch);
    }

