/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
 ****** calfile.c
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

#include "cal.h"


/**** CreateChangeFile *****/
VOID APIENTRY CreateChangeFile ()
     {
     /* If there is already a change file, delete it, ignoring any errors
        since we will be creating a new one, and that's the important one.
     */
     DeleteChangeFile ();

     /* Set the end-of-data of the change file to block 0. */
     vobkEODChange = 0;

     /* By passing the drive letter as 0 we tell GetTempFileName
        to decide where to put the temp file.
     */
     if (!(vfChangeFile = FCreateTempFile (IDFILECHANGE, 0)))
          {
          /* Post error saying edits will not be recorded. */
		  OutputDebugString ("Message Box Is Broken\n");

          //AlertBox (vszNoCreateChangeFile, (CHAR *)NULL,
           //MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
          }
     }




/**** DeleteChangeFile - delete the change file if there is one. ****/
VOID APIENTRY DeleteChangeFile ()
     {
     if (vfChangeFile)
          {
          /* Ignore errors since callers don't care about them. */
          vfChangeFile = FALSE;
          FDeleteFile (IDFILECHANGE);
          }
     }




/**** FCreateTempFile ****/

BOOL APIENTRY FCreateTempFile (
     INT  idFile,
     INT  iDrive)        /* 0 means let GetTempFileName decide where to put
                            the temp file.
                            Otherwise, this is the drive letter, and it should
                            also have the TF_FORCEDRIVE bit set to make sure that
                            the temp file is created on the specified drive.
                          */
{
    CHAR szFileSpec [CCHFILESPECMAX];
    INT  FileHandle;

	/* Create a temp file with a unique name.
	 * 0 for the third parameter means GetTempFileName should
	 * produce a unique file name and create the file.
	 * GetTempFileName returns the random number it used, which Steve Wood
	 * guarantees is 0 iff the call fails (he does not allow the random
	 * number to be 0).  If the file is created OK by GetTempFileName,
	 * open it to set up the reopen buffer.
	 */
	if (MGetTempFileName (iDrive, "CAL", 0, szFileSpec) == 0)
	{
		return (FALSE);
	}

	if ((FileHandle = MOpenFile (szFileSpec, &OFStruct [idFile],
			OF_READWRITE )) != -1)
	{
		/* File is OK.  Close it, and return TRUE. */
		M_lclose (FileHandle);
		return (TRUE);
	}

	/* GetTempFileName failed or OpenFile failed. */
	return (FALSE);

}


/**** FFreeUpDr - Free up the specified DR. ****/

BOOL APIENTRY FFreeUpDr (
     DR   *pdr,          /* Pointer to the DR to be written out. */
     DL   *pdl)          /* OUTPUT - DL indicating where the occupant was put. */
     {
     DL   dlNew;

     if (!pdr -> fDirty)
          {
          /* It's not dirty so don't change the date's location. */
          *pdl = DLNOCHANGE;
          return (TRUE);
          }

     /* The DR is dirty.  However, it may be empty, in which case
        we tell the caller it is not on disk.
     */
     if (pdr -> cbNotes + pdr -> cbTqr == 0)
          {
          *pdl = DLNIL;
          return (TRUE);
          }

     /* It's not empty. */
     if (!vfChangeFile)
          {
          /* Tell the user that the edits are not being recorded.
             We already warned the turkey when we couldn't create
             the change file, but he didn't listen.  Now just
             throw away his edits.
          */
          AlertBox (vszNoChangeFile,
           (CHAR *)NULL, MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
          *pdl = DLNOCHANGE;
          return (TRUE);
          }

     /* Set up the new DL using vobkEODChange before FWriteDrToFile
        changes it.
     */
     dlNew = DLFCHANGEFILEMASK | vobkEODChange;
     if (FWriteDrToFile (TRUE, IDFILECHANGE, pdr))
          {
          /* The write was successful - tell the caller about the
             new DL.
          */
          *pdl = dlNew;
          return (TRUE);
          }
     else
          {
          /* An error occured while attempting to write the date. */
          AlertBox (vszErrorWritingChanges, (CHAR *)NULL,
           MB_APPLMODAL | MB_OK | MB_ICONHAND);
          return (FALSE);
          }
     }




/**** FWriteDrToFile *****/
BOOL APIENTRY FWriteDrToFile (
     BOOL fOpenClose,    /* If TRUE, the file must be opened and then closed.
                            If FALSE, the file is already open and should be
                            left open.
                          */
     INT  idFile,        /* Which file to write to. */
     DR   *pdr)          /* Which DR to write from. */
     {
     INT  *pobkEOD;
     INT  cbkTransfer;
     INT  cbTransfer;
     INT  FileHandle;
     BOOL fOk;

     /* Set up a pointer to the appropriate EOD. */
     pobkEOD = &vobkEODNew;

     if (idFile == IDFILECHANGE)
          pobkEOD = &vobkEODChange;

     /* Try to reopen the file. */
     if (!fOpenClose || FReopenFile (idFile, OF_PROMPT | OF_CANCEL | OF_REOPEN | OF_READWRITE))
          {
          /* Make a local copy of the file handle to save code below. */
          FileHandle = hFile [idFile];

          /* Calculate the minimum number of BKs we must
             write out.  Do this by taking the count of
             bytes in use in the DR and adding the count
             of bytes in a BK minus 1 in order to round up
             to the next BK.  Then divide by the count
             of bytes in a BK to get the number of BKs
             to be written.
           */
          cbkTransfer = (pdr -> cbNotes + pdr -> cbTqr
           + CBDRHEAD + CBBK - 1) / CBBK;

          /* Clear the reserved word. */
          pdr -> wReserved = 0;

          /* Seek to the current end of data, write the
             current DR, and close the file.
          */
          cbTransfer = CBBK * cbkTransfer;
          fOk = M_llseek (FileHandle, (LONG)(CBBK*(*pobkEOD)), 0) != -1
                       && FWriteFile (FileHandle, (BYTE *)pdr, cbTransfer);

          if (FCondClose (fOpenClose, idFile) && fOk)
               {
               /* The DR has been successfully written to the file.
                  Update the EOD of the file.
               */
               *pobkEOD += cbkTransfer;
               return (TRUE);
               }
          }

     return (FALSE);

     }




/**** FReadDrFromFile ****/

BOOL APIENTRY FReadDrFromFile (
     BOOL fOpenClose,      /* If TRUE, the file must be opened and then closed.
                              If FALSE, the file is already open and should be
                              left open.
                              */
     DR   *pdr,               /* Where to read it into. */
     DL   dl)                 /* File location of date. */
     {
     INT  idFile;
     INT  FileHandle;
     WORD cbData;
     OBK  obk;
     BOOL fOk;

     /* Assume we will be reading from the original file. */
     idFile = IDFILEORIGINAL;

     /* Separate the block offset, and switch to the change file if the
        change file flag is set in the DL.
     */
     obk = dl & DLOBKMASK;
     if (dl & DLFCHANGEFILEMASK)
          idFile = IDFILECHANGE;

     /* Try to reopen the file. */
     if (fOpenClose && !FReopenFile (idFile, OF_PROMPT | OF_CANCEL | OF_REOPEN
                                     | OF_READ))
          return (FALSE);



     /* Reopen was successful - seek to the beginning of the DR, and
        read its header in order to know how much data there is.
      */

     FileHandle = hFile [idFile];
     if (M_llseek (FileHandle, (LONG)(CBBK * obk), 0) == -1
              || M_lread (FileHandle, (LPSTR)pdr, CBDRHEAD) != CBDRHEAD)
     {
          FCondClose (fOpenClose, idFile);
          return (FALSE);
     }

     /* Header was successfully read.  Now read in the rest. */
     cbData = pdr -> cbNotes + pdr -> cbTqr;
     fOk = (WORD)M_lread (FileHandle, (LPSTR)pdr + CBDRHEAD, cbData) == cbData;

     /* Close the file. */
     return (FCondClose (fOpenClose, idFile) && fOk);
     }




/**** FGetDateDr ****/

BOOL APIENTRY FGetDateDr (DT dtTarget)
     {
     DR   *pdr;
     INT  itdd;
     DD   *pdd;
     DL   dlTarget;
     WORD  idrTarget;
     WORD  idrNew;
     WORD  idrKickOut;
     DL   dlKickOut;
     DT   dtKickOut;
     HWND hwndFocus;

     /* If this routine succeeds in getting the requested date, the
        focus is left NULL.  If it fails, the focus is set back to its
        previous window.  Remember who has the focus now, then set it
        NULL to record the current edits into the current date. This MUST be
        done before switching dates so the data goes into the correct
        date.  It's important to leave the focus NULL in the success case
        so that the data doesn't get recorded again (into the wrong date)
        when the caller changes the focus later.
     */
     hwndFocus = GetFocus ();
     CalSetFocus ((HWND)NULL);

     /* See if this date is already in the tdd. */
     if (!FSearchTdd (dtTarget, &itdd))
          {
          /* Not found, try to insert it. */
          if (!FGrowTdd (itdd, 1))
               {
               /* Cannot grow tdd to include the new date.  FGrowTdd has
                  already put up the error message.
               */
               CalSetFocus (hwndFocus);
               return (FALSE);
               }

          /* Put the date into the new entry, say it is not marked,
             it has no alarms, it's not on disk, and it's not in memory.
          */
          pdd = TddLock () + itdd;
          pdd -> dt = dtTarget;
          pdd -> fMarked = FALSE;
          pdd -> cAlarms = 0;
          pdd -> dl = DLNIL;
          pdd -> idr = IDRNIL;
          TddUnlock ();
          }

     /* At this point itdd is the index of the target date
        within the tdd.  See if the target date is already in memory.
     */
     dlTarget = (pdd = TddLock () + itdd) -> dl;
     idrTarget = pdd -> idr;
     TddUnlock ();
     if ((WORD)idrTarget != IDRNIL)
          {
          /* The target date is already in memory.  Make the DR it
             is stored in the current DR, and return TRUE.
          */
          vidrCur = idrTarget;
          return (TRUE);
          }

     /* Find a free DR to put the target date in.  */
     idrNew = IdrFree ();

     /* In order to comply with the rule that there is always one
        free DR, we have to kick out a date if the 2 DRs other
        than idrNew are both in use.  Here's how we decide which, if
        any, date must be kicked out of memory:
        - Look at each DR:
          - If it's idrNew, or it's in use for today, skip it.
        So we end up either kicking nothing out (if we find a second
        free DR), or kicking out a date that's not today.
        Since the same date can't be in two DRs
        at the same time, we know that we will either find another
        free DR or one that contains a date other than today, so this
        loop will terminate.
        Note - 9/2/85 - MLC - I originally kept today in memory at all
        times because in month mode I always displayed the notes for
        today, regardless of which day was selected.  Some time ago I
        changed month mode so it shows the notes for the selected date.
        I decided to still keep today around since I assume the user
        will be refering to it more than any other date, so it seemed
        better than just keeping the two most recently accessed dates
        around.
     */
     idrKickOut = CDR;
     do
          {
          idrKickOut--;
          dtKickOut = PdrLock (idrKickOut) -> dt;
          DrUnlock (idrKickOut);
          }
     while (idrKickOut == idrNew || dtKickOut == vftCur.dt);

     if (dtKickOut != DTNIL)
          {
          /* We must kick out a date to free up a DR. */
          pdr = PdrLock (idrKickOut);
          if (!FFreeUpDr (pdr, &dlKickOut))
               {
               /* If we just created the DD for the target date,
                  it is still empty, so we will get rid of it if so.
               */
               DrUnlock (idrKickOut);
               DeleteEmptyDd (itdd);
               CalSetFocus (hwndFocus);
               return (FALSE);
               }
          pdr -> fDirty = FALSE;
          DrUnlock (idrKickOut);
          }

     pdr = PdrLock (idrNew);
     if (dlTarget == DLNIL)
          {
          /* No previous data for this date so create an empty DR for it. */
          pdr -> cbNotes = pdr -> cbTqr = 0;
          }
     else
          {
          if (!FReadDrFromFile (TRUE, pdr, dlTarget))
               {
               /* Mark the DR as still not in use. */
               pdr -> dt = DTNIL;
               DrUnlock (idrNew);

               /* If we just created the DD for the target date,
                  it is still empty, so we will get rid of it if so.
               */
               DeleteEmptyDd (itdd);
               CalSetFocus (hwndFocus);
               return (FALSE);
               }
          }

     /* Could be here if no previous data or
        where date was successfully read from file.
        Therefore, set the dt, since in the first case it has not
        been set.
     */
     pdr -> dt = dtTarget;
     pdr -> fDirty = FALSE;
     DrUnlock (idrNew);
     (TddLock () + itdd) -> idr = idrNew;
     TddUnlock ();

     if (dtKickOut != DTNIL)
          {
          /* We kicked out a date.  Mark that DR free. */
          PdrLock (idrKickOut) -> dt = DTNIL;
          DrUnlock (idrKickOut);

          /* Search for the DD of the date we kicked out.  It's OK to
             ignore the return value of FSearchTdd since the date we
             kicked out must be in the tdd.
          */
          FSearchTdd (dtKickOut, &itdd);

          /* Say the kicked out date is no longer in memory, and change
             the DL if the date's location has changed.
          */
          (pdd = TddLock () + itdd) -> idr = IDRNIL;
          if (dlKickOut != DLNOCHANGE)
               pdd -> dl = dlKickOut;
          TddUnlock ();

          /* Get rid of the DD of the kicked out date if it's "empty". */
          DeleteEmptyDd (itdd);
          }

     vidrCur = idrNew;
     return (TRUE);
     }
