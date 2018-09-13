/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** caltqr.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOSCROLL
#define NODRAWTEXT
#define NOVIRTUALKEYCODES
#define NOCLIPBOARD

#include "cal.h"


/**** FSearchTqr */

BOOL APIENTRY FSearchTqr (TM tm)
     {

     BYTE *pbPrev;
     BYTE *pbCur;
     BYTE *pbNext;
     BYTE *pbFirst;
     BYTE *pbMax;
     BOOL fFound;
     DR   *pdr;

     pdr = PdrLockCur ();
     votqrPrev = votqrCur = OTQRNIL;
     votqrNext = pdr -> cbTqr;
     pbMax = (pbCur = pbFirst = PbTqrFromPdr (pdr)) + pdr -> cbTqr;
     fFound = FALSE;

     while (pbCur < pbMax)
          {
          pbNext = pbCur + ((PQR )pbCur) -> cb;

          if (((PQR )pbCur) -> tm == tm)
               {
               votqrCur = pbCur - pbFirst;
               votqrNext = pbNext - pbFirst;
               fFound = TRUE;
               break;
               }

          else if (((PQR )pbCur) -> tm > tm)
               {
               votqrNext = pbCur - pbFirst;
               break;
               }

          pbPrev = pbCur;
          pbCur = pbNext;

          }

     if (pbCur != pbFirst)
          votqrPrev = pbPrev - pbFirst;

     DrUnlockCur ();
     return (fFound);
     }


/**** StoreQd - Move the appointment description from the edit control
      into the tqr.
      No need to check for room in DR here since we have been monitoring
      key strokes and checking for room.
*/

VOID APIENTRY StoreQd ()

     {

     BYTE rgbQrBuf [CBQRHEAD + CCHQDMAX + 1];
     CHAR szqdOld [CCHQDMAX + 1];
     INT  cchqdOld;
     INT  cchqdNew;
     WORD otqr;
     BYTE *pb;
     BOOL fSame;
     register INT ln;

     /* Copy into register variable to save code. */
     ln = vlnCur;

     /* Fetch the text from the edit control. */
     cchqdNew = SendMessage (vhwnd3, WM_GETTEXT, CCHQDMAX + 1,
      (LONG)(LPSTR)(rgbQrBuf + CBQRHEAD));

     *szqdOld = '\0';
     cchqdOld = 0;
     pb = PbTqrLock ();
     if ((otqr = vtld [ln].otqr) != OTQRNIL)
          {
          cchqdOld = ((PQR )(pb + otqr)) -> cb - CBQRHEAD - 1;
          lstrcpy (szqdOld, (CHAR *)(pb + otqr + CBQRHEAD));
          }
     fSame = cchqdOld == cchqdNew
      && lstrcmp ((LPSTR)szqdOld, (LPSTR)(rgbQrBuf + CBQRHEAD)) == 0;
     DrUnlockCur ();
     if (fSame)
          {
          /* This thing hasn't changed, so don't waste alot of time. */
          return;
          }

     /* The QD has changed, so mark the DR and the file dirty. */
     PdrLockCur () -> fDirty = vfDirty = TRUE;
     DrUnlockCur ();

     if (otqr == OTQRNIL)
          {
          /* There was no previous QR.  Say there is no alarm set,
             say it's not a special time, and copy in the appointment
             time from the tld.
          */
          ((PQR )rgbQrBuf) -> fAlarm = ((PQR )rgbQrBuf) -> fSpecial = FALSE;
          ((PQR )rgbQrBuf) -> tm = vtld [ln].tm;
          }
     else
          {
          /* There was a previous QR.  Copy it's header information
             (we want the flags and the appointment time) into the
             new QR we're building.
          */
          BltByte (PbTqrLock () + otqr, rgbQrBuf, CBQRHEAD);
          DrUnlockCur ();

          /* Delete the old QR. */
          DeleteQr (otqr);

          /* In case we don't insert a new qr. */
          vtld [ln].otqr = OTQRNIL;

          /* Adjust down the otqrs in the tld beyond the current ln. */
          AdjustOtqr (ln, -((PQR )rgbQrBuf) -> cb);
          }

     /* Set the length of the new QR. */
     ((PQR )rgbQrBuf) -> cb = CBQRHEAD + cchqdNew + 1;

     if (cchqdNew != 0 || ((PQR )rgbQrBuf) -> fAlarm
      || ((PQR )rgbQrBuf) -> fSpecial)
          {
          if (otqr == OTQRNIL)
               {
               /* There was no previous QR, so search the tqr to find
                  out where to put the new one.  Since we know there
                  was no old QR, we know FSearchTqr will not find a
                  match - so we ignore its return value.
               */
               FSearchTqr (vtld [ln].tm);
               otqr = votqrNext;
               }

          FInsertQr (otqr, (PQR )rgbQrBuf);
          vtld [ln].otqr = otqr;

          /* Adjust up the otqrs in the tld beyond the current ln. */
          AdjustOtqr (ln, ((PQR )rgbQrBuf) -> cb);

          }
     }


/**** AdjustOtqr */

VOID APIENTRY AdjustOtqr (
    INT  ln,        /* otqr starting with vtld [ln + 1] are adjusted. */
    INT  cb)        /* The amount to adjust the otqr by (can be negative). */
     {
     while (++ln < vcln)
          {
          if (vtld [ln].otqr != OTQRNIL)
               vtld [ln].otqr += cb;
          }
     }


/**** DeleteQr - delete qr from tqr. */

VOID APIENTRY DeleteQr (WORD otqr)
     {

     BYTE *pbTqr;
     BYTE *pbSrc;
     BYTE *pbDst;
     WORD cb;
     DR   *pdr;

     pdr = PdrLockCur ();
     pbDst = (pbTqr = PbTqrFromPdr (pdr)) + otqr;
     pbSrc = pbDst + (cb = ((PQR )pbDst) -> cb);
     BltByte (pbSrc, pbDst, (WORD)(pbTqr + pdr -> cbTqr - pbSrc));
     pdr -> cbTqr -= cb;
     DrUnlockCur ();

     }


/**** FInsertQr - insert qr into tqr. */

BOOL APIENTRY FInsertQr (
    WORD otqr,
    PQR	pqr)
     {

     BYTE *pbTqr;
     register BYTE *pbSrc;
     BYTE *pbDst;
     WORD cb;
     register DR   *pdr;
     BOOL fOk;

     /* Make sure there's enough room. */
     pdr = PdrLockCur ();
     if ((WORD)(fOk = cb = pqr->cb) <= (WORD)(CBTQRMAX - pdr -> cbTqr))
          {
          /* Zero the unused bits. */
          pqr -> reserved = 0;

          pbSrc = (pbTqr = PbTqrFromPdr (pdr)) + otqr;
          pbDst = pbSrc + cb;
          BltByte (pbSrc, pbDst, (WORD)(pbTqr + pdr -> cbTqr - pbSrc));
          BltByte ((BYTE *)(pqr), pbSrc, cb);
          pdr -> cbTqr += cb;
          }
     else
          {
          /* Tell the user the date is full. */
          AlertBox (vszDateIsFull, (CHAR *)NULL,
           MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
          }

     DrUnlockCur ();
     return (fOk);

     }


/**** PbTqrLock - Lock the current DR, returning a pointer to the first
      byte of the embedded Tqr.
*/

BYTE * APIENTRY PbTqrLock ()

     {

     register DR *pdr;

     pdr = PdrLockCur ();
     return (PbTqrFromPdr (pdr));

     }


/**** PdrLockCur - Lock the current DR, returning a pointer to it. */

DR   * APIENTRY PdrLockCur ()

     {

     return (PdrLock (vidrCur));

     }


/**** DrUnlockCur - Unlock the current DR. */

VOID APIENTRY DrUnlockCur ()

     {

     DrUnlock (vidrCur);

     }


/**** PdrLock - lock and return a pointer to the specified DR. */

DR   * APIENTRY PdrLock (WORD idr)
     {
     return ((DR *)LocalLock (vrghlmDr [idr]));
     }


/**** DrUnlock - unlock the specified DR. */

VOID APIENTRY DrUnlock (WORD idr)
     {
     LocalUnlock (vrghlmDr [idr]);
     }


/**** PbTqrFromPdr - Given a pdr, return a pointer to the first byte
      of the embedded tqr.
*/

BYTE * APIENTRY PbTqrFromPdr (DR*pdr)
     {
     return ((BYTE *)pdr + CBDRHEAD + pdr -> cbNotes);
     }


/**** StoreNotes - store the notes into the DR.
      No need to check for room in DR here since we have been monitoring
      key strokes and checking for room.
*/

VOID APIENTRY StoreNotes ()

     {

     BYTE rgbEcNotes [CBNOTESMAX];
     CHAR *szNew;
     CHAR *szOld;
     register INT  cbNew;
     INT  cbMore;
     register DR *pdr;
     BOOL fFormatted;

     /* Set up a pointer to the old notes, using the null string if there
        were no old notes.  Also assume the new notes are empty.
     */
     szNew = szOld = "";
     pdr = PdrLockCur ();
     if (pdr -> cbNotes != 0)
          szOld = (CHAR *)((BYTE *)pdr + CBDRHEAD);

     /* Format the new text.  This inserts <CR,CR,LF> where soft line breaks
        are.  (Hard line breaks are represented by <CR,LF>.)
     */
     fFormatted = SendMessage (vhwnd2C, EM_FMTLINES, TRUE, 0L);

     /* Find out how long the text is (not including the string
        terminator).
     */
     cbNew = SendMessage (vhwnd2C, WM_GETTEXTLENGTH, 0, 0L);

     if (cbNew != 0)
          {
          /* String is not null - will need an extra byte to store the zero
             terminator (which is why we ++cbNew here).
          */
          SendMessage (vhwndFocus, WM_GETTEXT, ++cbNew,
           (LONG)(LPSTR)(szNew = rgbEcNotes));
          }

     /* If the notes have not changed don't mark the DR dirty or store the
        new (unchanged) text.  (For speed, don't strcmp if length not same.)
     */
     if ((cbMore = cbNew - pdr -> cbNotes) != 0 || lstrcmp ((LPSTR)szOld, (LPSTR)szNew) != 0)
          {
          /* Adjust the hole for the notes if necessary by moving the tqr
             up or down.  Note that the same length case works OK too.
          */
          BltByte (PbTqrFromPdr (pdr), PbTqrFromPdr (pdr) + cbMore,
           pdr -> cbTqr);

          /* Copy in the new string.  Note - can't copy to szOld since
             it is not in the DR if the old string was null.
          */
          BltByte ((BYTE *)szNew, (BYTE *)pdr + CBDRHEAD, cbNew);

          /* Set the new length. */
          pdr -> cbNotes = cbNew;

          /* Mark the DR and the file dirty. */
          pdr -> fDirty = vfDirty = TRUE;
          }

     DrUnlockCur ();

     /* Remove soft line breaks. */
     if (fFormatted)
	  SendMessage (vhwnd2C, EM_FMTLINES, FALSE, 0L);

     }


/**** SetNotesEc - Set the text of the notes edit control. */

VOID APIENTRY SetNotesEc ()

     {
     register CHAR *szNotes;
     register DR *pdr;


     szNotes = "";
     if ((pdr = PdrLockCur ()) -> cbNotes != 0)
          szNotes = (CHAR *)((BYTE *)pdr + CBDRHEAD);
     SetEcText(vhwnd2C, szNotes);
     DrUnlockCur ();

     /* Length of text in edit ctl will probably be different than cbNotes
        since formatting info has been removed.  Set selection to end of text
        based on new length. */

     }


/**** EcNotification - handle notifications from edit controls.
      Note - all of the vcbEcTextMax code assumes that the data in the
      DR is up-to-date.  This is the case as long as my assumption that
      you can't get an EN_SETFOCUS for the same edit control without
      having seen an EN_KILLFOCUS in the intervening time.
      (The KILLFOCUS will bring the DR up-to-date because that's when
      we store the data.)
*/

INT  vcbEcTextMax = 32767;    /* This is the maximum number of bytes we can
                                 allow the text of the edit control to grow to.
                                 This does NOT include the zero terminator,
                                 only the space used by the text itself.
                                 We reserve space for the terminator outside
                                 of this count.
                                 Keep it set very large when not otherwise
                                 set up so an EN_CHANGE (which can come in
                                 from a WM_SETTEXT for example) won't
                                 erroneously trigger a DR full situation.
                              */

VOID APIENTRY EcNotification (
    WORD idec,          /* ID of the edit control. */
    WORD en)            /* The notification code. */
     {

     register DR *pdr;
     WORD otqr;
     register BOOL fQd;

     if ((fQd = idec == IDECQD) || idec == IDECNOTES)
          {
          switch (en)
               {
               case EN_SETFOCUS:
                    if (fQd)
                         {
                         vhwndFocus = vhwnd3;

                         /* Assuming there is no QR for this appointment time,
                            we will need room to insert one (CBQRHEAD + 1 (for
                            the string terminator)).  Subtract this from the
                            available space for the tqr to grow to determine
                            the maximum
                            size we can allow for the edit control text.
                            (The available space is given by CBTQRMAX -
                            cbTqr.)
                         */
                         pdr = PdrLockCur ();
                         vcbEcTextMax = CBTQRMAX - CBQRHEAD - 1 - pdr -> cbTqr;
                         if ((otqr = vtld [vlnCur].otqr) != OTQRNIL)
                              {
                              /* There is already a QR for this appointment
                                 time, so the space it's occupying will also
                                 be available.
                              */
                              vcbEcTextMax +=
                               ((PQR )(PbTqrFromPdr (pdr) + otqr)) -> cb;
                              }
                         DrUnlockCur ();

                         /* Don't allow vcbEcTextMax to be negative (which
                            it could be right now if there's no QR for this
                            appointment and the amount of free space for
                            the tqr is less than CBQRHEAD + 1).  By setting it
                            to zero we will prevent any characters from
                            being typed, and the pruning process will work
                            correctly (which it wouldn't for a negative
                            value).
                         */
                         if (vcbEcTextMax < 0)
                              vcbEcTextMax = 0;
                         }
                    else
                         {
                         vhwndFocus = vhwnd2C;

                         /* The notes edit control is not allowed to grow
                            beyond CBNOTESTESTMAX bytes.
                         */
                         vcbEcTextMax = CBNOTESTEXTMAX;
                         }

                    break;

               case EN_KILLFOCUS:
                    /* Put big value in while not in use. */
                    vcbEcTextMax = 32767;

                    if (fQd)
                         {
                         /* Leaving appointment edit control - store away
                            the text if it has changed.
                         */
                         StoreQd ();
                         }
                    else
                         {
                         /* Leaving notes edit control - store away the
                            text if it has changed.
                         */
                         StoreNotes ();
                         }

                    break;

               case EN_ERRSPACE:
                    AlertBox (vszOutOfMemory, (CHAR *) NULL,
                     MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                    break;

               case EN_MAXTEXT:
                    AlertBox (vszTextTruncated, (CHAR *) NULL,
                     MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
                    break;

               case EN_CHANGE:
                    /* If this change is not for the edit control that has
                       the focus, ignore it.  (This happens for example
                       when we have set the focus to a QD and then set the
                       text of the notes - as when switching to a new date.)
                       If we didn't ignore it we would be comparing the
                       wrong lengths against each other.
                       Also check out of here if we haven't exceeded the
                       maximum text length yet.
                    */
                    if (vhwndFocus == (fQd ? vhwnd3 : vhwnd2C)
                        && ((INT)SendMessage (vhwndFocus, WM_GETTEXTLENGTH,0,0L) > vcbEcTextMax))
                        PruneEcText ();

                    /* Fall into the return. */
               }
          }
     }


/**** PruneEcText - truncate edit control text since it won't fit in the DR.
      This is a separate routine so that the large buffer on the stack
      would not be allocated in EcNotification.  This is because EcNotification
      gets called recursively (because WM_SETTEXT causes an EN_CHANGE) and
      it also calls StoreNotes which also uses a large buffer on the stack.
*/

VOID APIENTRY PruneEcText ()

     {

     BYTE rgbEcTextBuf [CBNOTESMAX];
     register BYTE *pbCur;
     INT  cbEcText;
     INT  cbTemp;

     /* That last change made the text too big to fit in
        the DR.  Truncate the text, and tell the user about it.
     */
     SendMessage (vhwndFocus, WM_GETTEXT, CBNOTESMAX, (LONG)(LPSTR)rgbEcTextBuf);

     /* Truncate what doesn't fit, being careful not
        to truncate in the middle of a multi-byte
        character (Kanji).
        Note that this loop is guaranteed to execute at
        least once (since 0 has to be <= vcbEcTectMax), so
        cbEcText will definitely get set.
     */
     for (pbCur = rgbEcTextBuf;
         (cbTemp = pbCur - rgbEcTextBuf) <= vcbEcTextMax;
         pbCur = (BYTE *)AnsiNext ((LPSTR)pbCur))
             cbEcText = cbTemp;

     /* cbEcText now contains the count of bytes in the
        the characters that would fit (not including the
        string terminator).  Put in the terminator.
     */
     rgbEcTextBuf [cbEcText] = '\0';

     /* Put the shortened text back into the edit control.
        This will put the caret
        at the end of the text and redisplay.  (Putting
        the caret at the end is good since it draws the
        user's attention to where the chopping occurred.)

        Note that this WM_SETTEXT will cause an EN_CHANGE, but the
        text will no longer be too long so we will not end up
        recursing into PruneEcText (which we want to avoid due
        to the large buffer on the stack).
     */
     SetEcText(vhwndFocus, rgbEcTextBuf);

     /* Tell the user the bad news. */
     AlertBox (vszDateIsFull, vszTextTruncated,
      MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);

     }
