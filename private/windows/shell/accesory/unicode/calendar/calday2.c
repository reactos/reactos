/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** calday2.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOCLIPBOARD

#include "cal.h"


/**** SetDayScrollRange */

VOID APIENTRY SetDayScrollRange ()

     {

     /* Set the range so that the minimum thumb position corresponds to
        having the first TM of the day at the top of the window and
        the maximum thumb position corresponds to having the last TM
        of the day at the bottom of the window.  We want to always be
        able to work from the first TM in the window, so we observe
        that the last TM of the day is at the bottom of the window
        when the last windowful (or page) of TMs is up, and this
        is when the (ctmDay - vcln)th TM is at the top of the window.
        For example, if there are 100 TMs (0 - 99), and we can display
        10 TMs in the window (vcln == 10), we are maxed out when
        90 through 99 are in the window.
        Note that calling ItmFromTm with TMNILHIGH returns the count
        of TMs in the day.
     */
     SetScrollRange (vhwnd2B, SB_VERT, 0, ItmFromTm (TMNILHIGH) - vcln, FALSE);

     }


/**** AdjustDayScrollRange - adjust the scroll bar range by the specified
      number of TMs.
*/

VOID APIENTRY AdjustDayScrollRange (INT ctm)
     {

     INT  itmMin;
     INT  itmMax;

     GetScrollRange (vhwnd2B, SB_VERT, (LPINT)&itmMin, (LPINT)&itmMax);
     SetScrollRange (vhwnd2B, SB_VERT, 0, itmMax + ctm, FALSE);

     }


/**** SetDayScrollPos - position the thumb. */

VOID APIENTRY SetDayScrollPos (register INT itm)
     {

     /* If called with itm == -1, position the thumb based on the TM
        at the top of the window.
     */
     if (itm == -1)
          itm = ItmFromTm (vtld [0].tm);

     SetScrollPos (vhwnd2B, SB_VERT, itm, TRUE);

     }


/**** AdjustDayScrollPos - adjust the position of the thumb relative
      to its old position.
*/

VOID APIENTRY AdjustDayScrollPos (INT ctm)
			/* Positive to increase thumb position (towards
                            bottom of window), negative to decrease thumb
                            (towards top of window).
                         */

     {

     SetDayScrollPos (GetScrollPos (vhwnd2B, SB_VERT) + ctm);

     }



/**** ItmFromTm - map a TM to an index within the range of TMs for the day.
      If tmToMap does not exist within the day (not a regular time and
      not in the tqr), we return the index of the next
      highest TM.  Therefore, to find out how many TMs there are in
      the day, call ItmFromTm with tmToMap == TMNILHIGH.  SetDayScrollRange
      depends on this.

      Note - tmToMap must be in the closed interval 0 through TMNILHIGH.
*/

INT  APIENTRY ItmFromTm (TM tmToMap)
     {

     INT  itm;

     itm = -1;

     MapTmAndItm (&tmToMap, &itm);
     return (itm);

     }


/**** TmFromItm - map TM to an index within the range of TMs for this date. */

TM   APIENTRY TmFromItm (INT  itm)
     {

     TM   tm;

     MapTmAndItm (&tm, &itm);
     return (tm);

     }


/**** MapTmAndItm - map TM to itm and vice versa.
      To map a TM to an itm:  *ptmMap == TM to map,  *pitmMap == -1.
      To map an itm to a TM:: *ptmMap == don't care, *pitmMap == itm to map.

*/

VOID APIENTRY MapTmAndItm (
    TM   *ptmMap,
    INT  *pitmMap)

     {

     register TM tmCur;
     register TM tmFromQr;
     TM   tmMap;
     DR   *pdr;
     PQR	pqrCur;
     PQR	pqrMax;
     INT  itm;
     BOOL fMapTmToItm;

     tmMap = ((fMapTmToItm = *pitmMap) == -1) ? *ptmMap : TMNILHIGH;

     /* Lock the DR, and get First and Max pointers for the tqr. */
     pdr = PdrLockCur ();

     pqrMax = (PQR)((BYTE*)(pqrCur = (PQR)PbTqrFromPdr(pdr)) + pdr->cbTqr);

     /* Find the first QR time. */
     tmFromQr = TmFromQr (&pqrCur, pqrMax);

     /* Starting at the first possible TM of the day (0), keep going
        until we find a TM greater than or equal to the one we're mapping.
     */
     for (itm = (INT)(tmCur = 0); tmCur < tmMap && itm != *pitmMap; itm++)
          {
          if ((tmCur = TmNextRegular (tmCur)) >= tmFromQr)
               {
               /* The QR TM is less than or equal to the next
                  regular TM, so use the one from the QR in order to
                  skip over it.

                  Note - At this point, both tmCur and tmFromQr could
                  be TMNILHIGH (because there aren't anymore QRs
                  and there are no more regular times.  This works OK
                  since we end up using TMNILHIGH which will terminate
                  terminate the loop since TMNILHIGH is the highest value
                  the caller is permitted to pass.  Calling TmFromQr
                  with pqrCur == pqrMax works OK too, so no problem there.
               */
               if ((tmFromQr = TmFromQr (&pqrCur, pqrMax)) < tmCur)
                    tmCur = tmFromQr;
               }
          }

     DrUnlockCur ();

     /* Pass back the mapped value to the caller. */
     if (fMapTmToItm)
          *pitmMap = itm;
     else
          *ptmMap = tmCur;

     }


/**** TmFromQr - return TM of the current QR or TMNILHIGH if there
      isn't one.  Also update pqrCur to point past the last QR we inspect.

      Note - Guaranteed to do the right thing if called with
      pqrCur == pqrMax.  In this case we return TMNILHIGH and pqrCur
      is unchanged.
*/

TM   APIENTRY TmFromQr (
    PQR	*ppqrCur,          /* Input - pqrCur points to the current QR.
				Output - pqrCur points to the next QR or
				is equal to pqrMax if no next QR.
			     */
    PQR	pqrMax)

     {

     register PQR pqr;
     register TM tm;

     /* Assume there are no more QRs. */
     tm = TMNILHIGH;

     if ((pqr = *ppqrCur) < pqrMax)
          {
          tm = pqr -> tm;
          *ppqrCur = (PQR)((BYTE*)pqr + pqr->cb);
          }

     return (tm);

     }


/**** TmNextRegular - return the next regular TM or TMNILHIGH if there
      is no next regular TM.
*/

TM   APIENTRY TmNextRegular (register TM tm)
     {

     register TM tmNext;

     /* Calculate the next regular appointment time. */
     tmNext = tm + vcMinInterval - (tm % vcMinInterval);

     /* Return TMNILHIGH if beyond the end of the day. */
     if (tmNext > TMLAST)
          tmNext = TMNILHIGH;

     return (tmNext);

     }
