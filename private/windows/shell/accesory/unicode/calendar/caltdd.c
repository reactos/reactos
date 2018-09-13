/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** caltdd.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOCLIPBOARD
#define NOVIRTUALKEYCODES

#include "cal.h"


#define CDDMAX 512       /* Max number of DD in the tdd. */
#define CDDEXTRA 8       /* When grabbing more memory for the tdd,
                            allocate CDDEXTRA extra DDs.  This avoids
                            doing a ReAlloc for each DD needed, so
                            things happen faster.
                         */


/**** InitTdd - Initialize the tdd.  */

VOID APIENTRY InitTdd ()
{
	//- InitTdd: Fixed to take care of bug with LocalReAlloc of returning
	//- NULL when the original block is already of size zero.
	//- Now just free and allocate later.
	if (vcddAllocated != 0 && vhlmTdd)
	{
		LocalFree (vhlmTdd);
	}
	vcddUsed = vcddAllocated = 0;
	vhlmTdd = (LOCALHANDLE)NULL;
}


/**** FSearchTdd - Search the tdd for the specified DT.
      If found, return TRUE and the index of the matching entry.
      If not found, return FALSE and the index of where to insert (the
      first dd having a DT greater than the one searched for).
*/

BOOL APIENTRY FSearchTdd (
    DT   dt,            /* Input - Date to search for. */
    INT  *pitdd)        /* Output - index of match or insertion point if no
			   match.
			*/
     {

     /* Note - it's important that the indices be declared as signed
        ints since it's possible for itddHigh to go to -1 (if the
        item being searched for is less than the first entry in the
        table).  If it were necessary for the indices to be unsigned
        (to allow a larger table size), some coding changes would be
        necessary, but for this application ints will work fine since
        the table will not be allowed to exceed 32767 entries (in fact,
        the limit will be much lower).
     */
     register INT  itddLow;
     register INT  itddHigh;
     BOOL fFound;
     DD   *pddFirst;
     DT   dtTemp;
     BOOL fGreater;

     /* Lock down the tdd and get the address of the first dd in it. */
     pddFirst = TddLock ();

     /* Note - in case the tdd is empty, initialize the insertion point
        to 0 for the caller.  Also set fGreater to FALSE so if the tdd is
        empty, the 0 in itdd will get returned without being incremented.
     */
     *pitdd =  itddLow = 0;
     itddHigh = vcddUsed - 1;
     fFound = fGreater = FALSE;

     while (itddLow <= itddHigh && !fFound)
          {
          fGreater = FALSE;
          *pitdd = (itddLow + itddHigh) / 2;

          if (dt == (dtTemp = (pddFirst + *pitdd) -> dt))
               fFound = TRUE;

          else if (dt > dtTemp)
               {
               fGreater = TRUE;
               itddLow = *pitdd + 1;
               }

          else
               itddHigh = *pitdd - 1;
          }

     TddUnlock ();

     /* The search item was greater than the table item on the last
        comparison made.  Return the index of the next higher table
        entry, since this is the insertion point.  Note that if
        dt == dtTemp, the index is already that of the matching item,
        and if dt < dtTemp, the index is already that of the insertion
        point.
     */
     if (fGreater)
          (*pitdd)++;

     return (fFound);
     }


/**** FGrowTdd - Grow the tdd by the specified number of DD at the specified
      place.  If can't grow that much, put up an error message then return
      FALSE.  If successful, return TRUE.
*/
BOOL APIENTRY FGrowTdd (
    INT  itdd,               /* INPUT - Where the insertion occurs. */
    INT  cdd)                /* INPUT - How many DD to insert. */
{

	DD   *pdd;
	register  INT cddUsedNew;
	register  INT cddAllocatedNew;

	if ((cddUsedNew = vcddUsed + cdd) > CDDMAX)
	{
		/* Can't make it that big. */
		AlertBox (vszTooManyDates, (TCHAR *)NULL,
				MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
		return (FALSE);
	}

	if (cddUsedNew > vcddAllocated)
	{
		/* We must allocate some more memory to the tdd.  Allocate
		   more than we need right now to avoid always having to allocate
		   for each new DD.
		*/
		cddAllocatedNew = cddUsedNew + CDDEXTRA;


		//- GrowTdd: Fixed to call LocalAlloc instead of LocalReAlloc because
		//- of bug in LocalReAlloc with zero size allocation.
		if (vcddAllocated == 0 || vhlmTdd == 0)
		{
			if ((vhlmTdd = LocalAlloc (LMEM_MOVEABLE,
					cddAllocatedNew * sizeof (DD))) == (LOCALHANDLE)NULL)
			{
				/* Could not get the requested memory. */
				AlertBox (vszOutOfMemory, (TCHAR *)NULL,
						MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
				return (FALSE);
			}
		}
		else
		{
			if ((vhlmTdd = LocalReAlloc (vhlmTdd, cddAllocatedNew * sizeof (DD),
					LMEM_MOVEABLE)) == (LOCALHANDLE)NULL)
			{
				/* Could not get the requested memory. */
				AlertBox (vszOutOfMemory, (TCHAR *)NULL,
						MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
				return (FALSE);
			}
		}

		vcddAllocated = cddAllocatedNew;
	}

	/* Block transfer up all DD at or beyond the insertion point. */
	pdd = TddLock () + itdd;
	BltByte ((BYTE *)pdd, (BYTE *)(pdd + cdd),
			(WORD)(sizeof (DD) * (vcddUsed - itdd)));
	TddUnlock ();

	vcddUsed = cddUsedNew;
	return (TRUE);
}


/**** ShrinkTdd - Shrink the tdd by the specified number of DD. */

VOID APIENTRY ShrinkTdd (
    INT  itdd,               /* The index of the first DD to be deleted. */
    INT  cdd)                /* The number of DD to be deleted. */
{

	register DD   *pdd;
	register INT  cddAllocatedNew;

	/* Lock the tdd, and get a pointer to the deletion point. */
	pdd = TddLock () + itdd;

	/* Block transfer down all dd beyond the deletion point. */
	BltByte ((BYTE *)(pdd + cdd), (BYTE *)pdd,
			(WORD)(sizeof (DD) * (vcddUsed - (itdd + cdd))));

	/* Adjust the count of dd. */
	vcddUsed -= cdd;

	TddUnlock ();

	if (vcddAllocated > (cddAllocatedNew = vcddUsed + CDDEXTRA))
	{
		/* There's more than CDDEXTRA free DDs now, so free up the
		   extra ones.
		*/
		//- ShrinkTdd: Fixed to handle bug in LocalReAlloc when trying to do
		//- realloc of size zero.
		if ((vcddAllocated = cddAllocatedNew) == 0)
		{

			if (vhlmTdd)
				vhlmTdd = LocalFree (vhlmTdd);
		}
		else
		{
			if (vhlmTdd)
				vhlmTdd = LocalReAlloc (vhlmTdd, cddAllocatedNew * sizeof (DD),
						LMEM_MOVEABLE);
		}
	}
}


/**** BltByte - Block transfer a range of bytes either up or down. */

BYTE * APIENTRY BltByte (
    BYTE *pbSrc,
    BYTE *pbDst,
    WORD cb)
{

	register BYTE *pbMax;

	pbMax = pbDst + cb;

	if (pbSrc >= pbDst)
	{
		/* Transferring down (from high to low addresses).
		   Start at the beginning of the block and work
		   towards higher addresses to avoid overwrite.
		*/
		while (cb-- != 0)
			*pbDst++ = *pbSrc++;
	}
	else
	{
		/* Transferring up (from low to high addresses).
		   Start at the end of the block and work towards lower
		   addresses to avoid overwrite.
		*/
		pbSrc += cb;
		pbDst = pbMax;
		while (cb-- != 0)
			*--pbDst = *--pbSrc;
	}

	/* Return a pointer to the first byte following those moved to the
	   destination.
	*/
	return (pbMax);
}


/**** DeleteEmptyDd - delete DD from tdd if the DD is "empty".
      The DD is "empty" if the date is not marked and it has
      no longer has any data (on disk or in memory).  Note that
      it cannot have any alarms if there are no Qrs for it, so
      there is no need to check cAlarms.  If it is empty, get rid of the DD.
*/

VOID APIENTRY DeleteEmptyDd (INT itdd)
{

	register DD *pdd;
	register BOOL fEmpty;

	pdd = TddLock () + itdd;
	fEmpty = !pdd -> fMarked && pdd -> dl == DLNIL && pdd -> idr == IDRNIL;
	TddUnlock ();
	if (fEmpty)
		ShrinkTdd (itdd, 1);
}


/**** TddLock */

DD   * APIENTRY TddLock ()

{
	return ((DD *)LocalLock (vhlmTdd));
}


/**** TddUnlock */

VOID APIENTRY TddUnlock ()

{
	LocalUnlock (vhlmTdd);
}
