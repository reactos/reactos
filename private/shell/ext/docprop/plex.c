/*---------------------------------------------------------------------------
FILE : PLEX.C
AUTHOR: STOLEN FROM EXCEL modified by NavPal
 This file contains routines used to manipulate the PL (pronounced:
 "plex") structures.
----------------------------------------------------------------------------*/
#include "priv.h"
#pragma hdrstop

#ifndef WINNT
#include "plex.h"
#include "debug.h"
#endif
/*-----------------------------------------------------------------------
|	FInRange
|		Simple little routine that tells you if a number lies within a
|		range.
|	
|	
|	Arguments:
|		w:			Number to check
|		wFirst:	First number in the range	
|		wLast:	Last number in the range
|		
|	Returns:
|		fTrue if the number is in range
|		
|	Keywords: in range check
-----------------------------------------------------------------------*/
BOOL FInRange(w, wFirst, wLast)
int w;
int wFirst, wLast;
{
	Assert(wLast >= wFirst);
	return(w >= wFirst && w <= wLast);
}
#ifdef DEBUG
/*----------------------------------------------------------------------------
|	FValidPl
|
|	Checks for a valid PL structure.
|
|	Arguments:
|		ppl		PL to check
|
|	Returns:
|		fTrue if the PL looks reasonable.
----------------------------------------------------------------------------*/
BOOL FValidPl(pvPl)
VOID *pvPl;
{
#define ppl ((PL *)pvPl)
	if (ppl== NULL ||
			ppl->cbItem == 0 ||
			ppl->iMac < 0 ||
			ppl->iMax < 0 ||
			ppl->iMax < ppl->iMac)
		return(fFalse);
	return(fTrue);
#undef ppl
}
#endif //DEBUG
/*----------------------------------------------------------------------------
|	CbPlAlloc
|
|	Returns amount of memory allocated to the given PL
|
|	Arguments:
|		ppl		PL to return info for.
|
|	Returns:
|		memory allocated to the PL
----------------------------------------------------------------------------*/
int CbPlAlloc(pvPl)
VOID *pvPl;
{
#define ppl ((PL *)pvPl)
	if (ppl == NULL)
		return(0);
#ifdef DEBUG
	Assert(FValidPl(ppl));
#endif // DEBUG
	return(WAlign(cbPL + (ppl->iMax * ppl->cbItem)));
#undef ppl
}
/*----------------------------------------------------------------------------
|	FreePpl
|
|	Frees a PL.
|
|	Arguments:
|		ppl		PL to free
|
|	Returns:
|		Nothing.
----------------------------------------------------------------------------*/
void FreePpl(pvPl)
VOID *pvPl;
{
#ifdef DEBUG
	Assert(FValidPl(pvPl));
#endif // DEBUG
	VFreeMemP(pvPl, (unsigned) CbPlAlloc(pvPl));
}
/*----------------------------------------------------------------------------
|	PplAlloc
|
|	Allocates and initializes a PL.
|
|	Arguments:
|		cbItem		sizeof structure in the PL
|		dAlloc		number of items to allocate at a time
|		iMax		number of items in initial allocation
|
|	Returns:
|		Pointer to PL.
|
|	Notes:
|		returns NULL if OOM
----------------------------------------------------------------------------*/
VOID *PplAlloc(cbItem, dAlloc, iMax)
unsigned cbItem;
int dAlloc;
unsigned iMax;
{
	PL *ppl;
	long cb;

	if (iMax > 32767) /* not too likely, but what the heck. */
		return(NULL);

	Assert((cbItem>=1 && cbItem<=65535u) && FInRange(dAlloc, 1, 31));

	cb = WAlign((long) cbPL + (long) cbItem * (long) iMax);

	ppl = (PL *)PvMemAlloc((unsigned) cb);
	if(ppl==NULL)
		return(NULL);
	FillBuf(ppl,0, (unsigned) cb);
	ppl->cbItem = cbItem;
	ppl->dAlloc = dAlloc;
	ppl->iMax = iMax;
	ppl->fUseCount = fFalse;
#ifdef DEBUG
	Assert(FValidPl(ppl));
#endif // DEBUG
	return(ppl);
}
/*----------------------------------------------------------------------------
|	IAddPl
|
|	Adds an item to a PL.
|
|	Arguments:
|		pppl		Pointer to PL.  May change if reallocated.
|		pv		New item to add.
|
|	Returns:
|		Index of new item.
|
|	Notes:
|		returns -1 if OOM
----------------------------------------------------------------------------*/
int IAddPl(ppvPl, pv)
VOID  **ppvPl;
VOID  *pv;
{
	int cbItem;
	int iMac;
	PL *ppl, *pplNew;

	ppl = *ppvPl;
#ifdef DEBUG
	Assert(FValidPl(ppl));
#endif // DEBUG
	cbItem = ppl->cbItem;
	iMac = ppl->iMac;
#ifdef UNUSED
	if (ppl->fUseCount)
		{
		int i;
		BYTE *pb;

		(*(int *)pv) = 1;

		// Search for an unused entry
		for (i = 0, pb = ppl->rg; i < iMac; i++, pb += cbItem)
			{
			if ((*(int *) pb) == 0)
				{
				bltbh(hpv, hpb, cbItem);
				return i;
				}
			}
		}
#endif
	if (iMac == ppl->iMax)
		{
		pplNew = PplAlloc(cbItem, ppl->dAlloc, iMac + ppl->dAlloc);
		if(pplNew==NULL)//OOM
			return(-1);
		pplNew->fUseCount = ppl->fUseCount;
		PbMemCopy(pplNew->rg,ppl->rg, iMac * cbItem);
	     /* pplNew->iMac = iMac;  /* This is not needed because hppl->iMac
					  will be over-written later */
		FreePpl(ppl);
		*ppvPl = ppl = pplNew;
		}
	PbMemCopy(&ppl->rg[iMac * cbItem],pv,cbItem);
	ppl->iMac = iMac + 1;
#ifdef DEBUG
	Assert(FValidPl(*ppvPl));
#endif // DEBUG
	return(iMac);
}
/*----------------------------------------------------------------------------
|	RemovePl
|
|	Removes an item from a PL.
|
|	Arguments:
|		ppl		PL to remove item from
|		i		index of item to remove
|
|	Returns:
|		fTrue if an item was removed (only fFalse for use count plexes).
----------------------------------------------------------------------------*/
BOOL RemovePl(pvPl, i)
VOID *pvPl;
int i;
{
	int iMac;
	int cbItem;
	BYTE *p;
#define ppl ((PL *)pvPl)

#ifdef DEBUG
	Assert(FValidPl(ppl) && i < ppl->iMac);
#endif // DEBUG
	iMac = ppl->iMac;
	cbItem = ppl->cbItem;
	p = &ppl->rg[i * cbItem];
#ifdef UNUSED
	if (ppl->fUseCount)
		{
		Assert((*(int HUGE *) hp) > 0);
		if (--(*(int HUGE *) hp) > 0)
			return fFalse;
		}
#endif
	if (i != iMac - 1)
		{
		PbMemCopy(p,p+cbItem,(iMac - i - 1) * cbItem);
		}
	ppl->iMac = iMac - 1;
#ifdef DEBUG
	Assert(FValidPl(ppl));
#endif // DEBUG
	return fTrue;
#undef ppl
}
/*----------------------------------------------------------------------------
|	ILookupPl
|
|	Searches a PL for an item.
|
|	Arguments:
|		ppl		PL to lookup into
|		p		item to lookup
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		-1 if not found.
----------------------------------------------------------------------------*/
int ILookupPl(pvPl, pvItem, pfnSgn)
VOID *pvPl;
VOID *pvItem;
int (*pfnSgn)();
{
	int i;
	BYTE *p;
#define ppl ((PL *)pvPl)

	if (ppl == NULL)
		return(-1);
#ifdef DEBUG
	Assert(FValidPl(ppl));
#endif // DEBUG
#ifdef UNUSED
	if (ppl->fUseCount)
		{
		for (i = 0, p = ppl->rg; i < ppl->iMac; i++, p += ppl->cbItem)
			{
			if (*(int *)p != 0 && (*(int (*)(void *,void *))pfnSgn)(p, pvItem) == sgnEQ)
				return(i);
			}
		}
	else
#endif
		{
		for (i = 0, p = ppl->rg; i < ppl->iMac; i++, p += ppl->cbItem)
			{
			if ((*(int (*)(void *, void *))pfnSgn)(p, pvItem) == sgnEQ)
				return(i);
			}
		}
	return(-1);
#undef ppl
}
/*----------------------------------------------------------------------------
|	PLookupPl
|
|	Searches a PL for an item
|
|	Arguments:
|		ppl		PL to search
|		pItem		item to search for
|		pfnSgn		comparison function
|
|	Returns:
|		Pointer to item, if found
|		Null, if not found
----------------------------------------------------------------------------*/
VOID *PLookupPl(pvPl, pvItem, pfnSgn)
VOID *pvPl;
VOID *pvItem;
int (*pfnSgn)();
{
	int i;

	if ((i = ILookupPl(pvPl, pvItem, pfnSgn)) == -1)
		return(NULL);
	return(&((PL *)pvPl)->rg[i * ((PL *)pvPl)->cbItem]);
}

/*----------------------------------------------------------------------------
|	FLookupSortedPl
|
|	Searches a sorted PL for an item.
|
|	Arguments:
|		hppl		PL to lookup into
|		hpItem		Item to lookup
|		pi			Index of found item (or insertion location if not)
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		index of location to insert if not found.
----------------------------------------------------------------------------*/
int FLookupSortedPl(hpvPl, hpvItem, pi, pfnSgn)
VOID *hpvPl;
VOID *hpvItem;
int *pi;
int (*pfnSgn)();
{
	int sgn;
	unsigned iMin, iMid, iMac;
	int cbItem;
	BYTE *hprg;
	BYTE *hpMid;
#define hppl ((PL *)hpvPl)

	if ((hppl)==NULL)
		{
		*pi = 0;
		return(fFalse);
		}

#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	Assert(!hppl->fUseCount);

	sgn = 1;
	cbItem = hppl->cbItem;
	iMin = iMid = 0;
	iMac = hppl->iMac;
	hprg = hppl->rg;
	while (iMin != iMac)
		{
		iMid = iMin + (iMac-iMin)/2;
		Assert(iMid != iMac);

		hpMid = hprg + iMid*cbItem;
		if ((sgn = (*(int (*)(void *, void *))pfnSgn)(hpMid, hpvItem)) == 0)
			break;

		/* Too low, look in upper interval */
		if (sgn < 0)
			iMin = ++iMid;
		/* Too high, look in lower interval */
		else
			iMac = iMid;
		}

	/* Not found, return index of location to insert it */
	*pi = iMid;
	return(sgn == 0);
#undef hppl
}

/*----------------------------------------------------------------------------
|	IAddNewPl
|
|	Adds an item to a PL, creating the PL if it's initially NULL.
|
|	Arguments:
|		phppl		pointer to PL
|		hp		pointer to item to add
|		cbItem		size of item
|
|	Returns:
|		the index of item added, if successful
|		-1, if out-of-memory
----------------------------------------------------------------------------*/
int IAddNewPl(phpvPl, hpv, cbItem)
VOID **phpvPl;
VOID *hpv;
int cbItem;
{
	int i;

#define phppl ((PL **)phpvPl)

	Assert(((*phppl)==NULL) || !(*phppl)->fUseCount);
	i = -1;
	if ((*phppl)==NULL)
		{
		*phppl = PplAlloc(cbItem, 5, 5);
		}
	if((*phppl)!=NULL)
		{
		Assert((*phppl)->cbItem == cbItem);
		i = IAddPl((VOID **)phppl, hpv);
		}
	return(i);
#undef phppl
}

/*----------------------------------------------------------------------------
|	IAddNewPlPos
|
|	Inserts an item into a plex at a specific position.
|
|	Arguments:
|		the index of the item added, if successful
|		-1 if out-of-memory
----------------------------------------------------------------------------*/
int IAddNewPlPos(phpvPl, hpv, cbItem, i)
VOID **phpvPl;
VOID *hpv;
int cbItem;
int i;
{
	BYTE *hpT;
#define phppl ((PL **)phpvPl)

	Assert(((*phppl)==NULL) || !(*phppl)->fUseCount);
	if (IAddNewPl((VOID **)phppl, hpv, cbItem) == -1)
		return(-1);
	Assert(i < (*phppl)->iMac);
	hpT = &(*phppl)->rg[i * cbItem];
//	bltbh(hpT, hpT + cbItem, ((*phppl)->iMac - i - 1) * cbItem);
//	bltbh(hpv, hpT, cbItem);
	PbMemCopy(hpT + cbItem, hpT, ((*phppl)->iMac - i - 1) * cbItem);
	PbMemCopy(hpT, hpv, cbItem);
#ifdef DEBUG
	Assert(FValidPl(*phppl));
#endif // DEBUG
	return(i);
#undef phppl
}

int IAddPlSort(phpvPl, hpv, pfnSgn)
VOID **phpvPl;
VOID *hpv;
int (*pfnSgn)();
{
	int i;
#ifdef DEBUG
	int iOld;
#endif
	Assert((*phpvPl)!=NULL);

	if (FLookupSortedPl(*phpvPl, hpv, &i, pfnSgn))
		return(-1);

#ifdef DEBUG
	iOld = i;
#endif
	i = IAddNewPlPos(phpvPl, hpv, (*(PL **)phpvPl)->cbItem, i);
#ifdef DEBUG
	Assert(i == -1 || i == iOld);
#endif
	return(i);
}


//---------------------------------------------------------------------------
//NO CHANGES BELOW THIS LINE PLEASE
//---------------------------------------------------------------------------
#ifdef FROMEXCEL
/*---------------------------------------------------------------------------
THIS CODE IS FROM EXCEL SOURCES. ITS KEPT HERE FOR REFERENCE
---------------------------------------------------------------------------*/
#pragma hdrstop("Excel.pre")
VSZASSERT
#include "new.h"


/* This file contains routines used to manipulate the PL (pronounced:
   "plex") structures.  They should probably be moved somewhere a little
   more appropriate, like maybe alloc.c */

/*----------------------------------------------------------------------------
|	HpLookupPl
|
|	Searches a PL for an item
|
|	Arguments:
|		hppl		PL to search
|		hpItem		item to search for
|		pfnSgn		comparison function
|
|	Returns:
|		Pointer to item, if found
|		hpNull, if not found
----------------------------------------------------------------------------*/
VOID HUGE *HpLookupPl(hpvPl, hpvItem, pfnSgn)
VOID HUGE *hpvPl;
VOID HUGE *hpvItem;
int (*pfnSgn)();
{
	int i;

	if ((i = ILookupPl(hpvPl, hpvItem, pfnSgn)) == -1)
		return(hpNull);
	return(&((PL HUGE *)hpvPl)->rg[i * ((PL HUGE *)hpvPl)->cbItem]);
}

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, HpplAlloc)
#else
#pragma alloc_text(window2, HpplAlloc)
#endif
#else
#pragma alloc_text(plgboot, HpplAlloc)
#endif
/*----------------------------------------------------------------------------
|	HpplAlloc
|
|	Allocates and initializes a PL.
|
|	Arguments:
|		cbItem		sizeof structure in the PL
|		dAlloc		number of items to allocate at a time
|		iMax		number of items in initial allocation
|		dg		memory group to allocate from
|
|	Returns:
|		Pointer to PL.
|
|	Notes:
|		May DoJmp(penvMem).
----------------------------------------------------------------------------*/
/*<<mcmain*/
#pragma NATIVE_START
VOID HUGE *HpplAlloc(cbItem, dAlloc, iMax, dg)
unsigned cbItem;
int dAlloc;
unsigned iMax;
unsigned dg;
{
	PL HUGE *hppl;
	long cb;
	int dgShift;

	if (iMax > 32767) /* not too likely, but what the heck. */
		DoJmp(penvMem, alctNoMem);

	Assert((cbItem>=1 && cbItem<=65535u) && FInRange(dAlloc, 1, 31) &&
		FInRange(BLow(dg), 1, 255));

	cb = WAlign((long) cbPL + (long) cbItem * (long) iMax);
#ifndef LARGEALLOC
	if(cb >= cbHeapMax)
		{
		ErrorNoMem();
		DoJmp(penvMem, alctNoMem);
		}
#endif

	hppl = (PL HUGE *)HpAlloc((unsigned) cb, dg);
	bltcbh(0, hppl, (unsigned) cb);
	Assert(dg!=0);
	for (dgShift = 0; !(dg & 1); dg >>= 1, dgShift++)
		;
	Assert(dgShift<8);
	hppl->cbItem = cbItem;
	hppl->dAlloc = dAlloc;
	hppl->iMax = iMax;
	hppl->fUseCount = fFalse;
	hppl->dgShift = dgShift;
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	return(hppl);
}
#pragma NATIVE_END
/*<<plex*/


#ifdef MAC
#pragma alloc_text(window, HpplUseCountAlloc)
#else
#pragma alloc_text(window2, HpplUseCountAlloc)
#endif
VOID HUGE *HpplUseCountAlloc(cbItem, dAlloc, iMax, dg)
unsigned cbItem;
int dAlloc;
unsigned iMax;
unsigned dg;
{
	PL HUGE *hppl;

	hppl = HpplAlloc(cbItem, dAlloc, iMax, dg);
	hppl->fUseCount = fTrue;
	return hppl;
}


/*----------------------------------------------------------------------------
|	FreeHppl
|
|	Frees a PL.
|
|	Arguments:
|		hppl		PL to free
|
|	Returns:
|		Nothing.
----------------------------------------------------------------------------*/
/*<<mcmain*/
#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, FreeHppl)
#else
#pragma alloc_text(window2, FreeHppl)
#endif
#endif
#pragma NATIVE_START
FreeHppl(hpvPl)
VOID HUGE *hpvPl;
{
#ifdef DEBUG
	Assert(FValidPl(hpvPl));
#endif // DEBUG
	FreeHp(hpvPl, CbPlAlloc(hpvPl));
}
#pragma NATIVE_END
/*<<plex*/

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, ILookupPl)
#else
#pragma alloc_text(window2, ILookupPl)
#endif
#else
#pragma alloc_text(swpmisc, ILookupPl)
#endif
/*----------------------------------------------------------------------------
|	ILookupPl
|
|	Searches a PL for an item.
|
|	Arguments:
|		hppl		PL to lookup into
|		hp		item to lookup
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		-1 if not found.
----------------------------------------------------------------------------*/
#pragma NATIVE_START
int ILookupPl(hpvPl, hpvItem, pfnSgn)
VOID HUGE *hpvPl;
VOID HUGE *hpvItem;
int (*pfnSgn)();
{
	int i;
	BYTE HUGE *hp;
#define hppl ((PL HUGE *)hpvPl)

	if (SbFromHp(hppl) == sbNull)
		return(-1);
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	if (hppl->fUseCount)
		{
		for (i = 0, hp = hppl->rg; i < hppl->iMac; i++, hp += hppl->cbItem)
			{
			if (*(int HUGE *)hp != 0 && (*(int (*)(HP, HP))pfnSgn)(hp, hpvItem) == sgnEQ)
				return(i);
			}
		}
	else
		{
		for (i = 0, hp = hppl->rg; i < hppl->iMac; i++, hp += hppl->cbItem)
			{
			if ((*(int (*)(HP, HP))pfnSgn)(hp, hpvItem) == sgnEQ)
				return(i);
			}
		}
	return(-1);
#undef hppl
}
#pragma NATIVE_END

/*----------------------------------------------------------------------------
|	FLookupSortedPl
|
|	Searches a sorted PL for an item.
|
|	Arguments:
|		hppl		PL to lookup into
|		hpItem		Item to lookup
|		pi			Index of found item (or insertion location if not)
|		pfnSgn		Comparison function
|
|	Returns:
|		index of item, if found.
|		index of location to insert if not found.
----------------------------------------------------------------------------*/
#ifdef MAC
#pragma alloc_text(window, FLookupSortedPl)
#else
#pragma alloc_text(window2, FLookupSortedPl)
#endif
#pragma NATIVE_START
int FLookupSortedPl(hpvPl, hpvItem, pi, pfnSgn)
VOID HUGE *hpvPl;
VOID HUGE *hpvItem;
int *pi;
int (*pfnSgn)();
{
	int sgn;
	unsigned iMin, iMid, iMac;
	int cbItem;
	BYTE HUGE *hprg;
	BYTE HUGE *hpMid;
#define hppl ((PL HUGE *)hpvPl)

	if (SbFromHp(hppl) == sbNull)
		{
		*pi = 0;
		return(fFalse);
		}

#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	Assert(!hppl->fUseCount);

	sgn = 1;
	cbItem = hppl->cbItem;
	iMin = iMid = 0;
	iMac = hppl->iMac;
	hprg = hppl->rg;
	while (iMin != iMac)
		{
		iMid = iMin + (iMac-iMin)/2;
		Assert(iMid != iMac);

		hpMid = hprg + iMid*cbItem;
		if ((sgn = (*(int (*)(HP, HP))pfnSgn)(hpMid, hpvItem)) == 0)
			break;

		/* Too low, look in upper interval */
		if (sgn < 0)
			iMin = ++iMid;
		/* Too high, look in lower interval */
		else
			iMac = iMid;
		}

	/* Not found, return index of location to insert it */
	*pi = iMid;
	return(sgn == 0);
#undef hppl
}
#pragma NATIVE_END

/*----------------------------------------------------------------------------
|	DeletePl
|
|	Removes an item from a PL.  The resulting PL is compacted.
|
|	Arguments:
|		phppl		pointer to PL to remove item from
|		i		index of item to remove
|
|	Returns:
|		Nothing.
----------------------------------------------------------------------------*/
DeletePl(phpvPl, i)
VOID HUGE **phpvPl;
int i;
{
#ifdef DEBUG
	Assert(FValidPl(*phpvPl));
#endif // DEBUG
	RemovePl(*phpvPl, i);
	FCompactPl(phpvPl, (*(PL HUGE **)phpvPl)->iMac == 0);
#ifdef DEBUG
	Assert(SbOfHp(*phpvPl) == sbNull || FValidPl(*phpvPl));
#endif // DEBUG
}


/*----------------------------------------------------------------------------
|	RemovePl
|
|	Removes an item from a PL.
|
|	Arguments:
|		hppl		PL to remove item from
|		i		index of item to remove
|
|	Returns:
|		fTrue if an item was removed (only fFalse for use count plexes).
----------------------------------------------------------------------------*/
BOOL RemovePl(hpvPl, i)
VOID HUGE *hpvPl;
int i;
{
	int iMac;
	int cbItem;
	BYTE HUGE *hp;
#define hppl ((PL HUGE *)hpvPl)

#ifdef DEBUG
	Assert(FValidPl(hppl) && i < hppl->iMac);
#endif // DEBUG
	iMac = hppl->iMac;
	cbItem = hppl->cbItem;
	hp = &hppl->rg[i * cbItem];

	if (hppl->fUseCount)
		{
		Assert((*(int HUGE *) hp) > 0);
		if (--(*(int HUGE *) hp) > 0)
			return fFalse;
		}
			
	if (i != iMac - 1)
		{
		bltbh(hp + cbItem, hp, (iMac - i - 1) * cbItem);
		}
	hppl->iMac = iMac - 1;
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	return fTrue;
#undef hppl
}

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, IAddPl)
#else
#pragma alloc_text(window2, IAddPl)
#endif
#else
#pragma alloc_text(swpmisc, IAddPl)
#endif
/*----------------------------------------------------------------------------
|	IAddPl
|
|	Adds an item to a PL.
|
|	Arguments:
|		phppl		Pointer to PL.  May change if reallocated.
|		hp		New item to add.
|
|	Returns:
|		Index of new item.
|
|	Notes:
|		May DoJmp(penvMem) when reallocating.
----------------------------------------------------------------------------*/
/*<<mcmain*/
#pragma NATIVE_START
int IAddPl(phpvPl, hpv)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
{
	int cbItem;
	int iMac;
	PL HUGE *hppl, HUGE *hpplNew;

	hppl = *phpvPl;
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	cbItem = hppl->cbItem;
	iMac = hppl->iMac;

	if (hppl->fUseCount)
		{
		int i;
		BYTE HUGE *hpb;

		(*(int HUGE *)hpv) = 1;

		// Search for an unused entry
		for (i = 0, hpb = hppl->rg; i < iMac; i++, hpb += cbItem)
			{
			if ((*(int HUGE *) hpb) == 0)
				{
				bltbh(hpv, hpb, cbItem);
				return i;
				}
			}
		}

#ifdef DEBUG
	if (iMac == hppl->iMax || fShakeMem)
#else
	if (iMac == hppl->iMax)
#endif
		{
		hpplNew = HpplAlloc(cbItem, hppl->dAlloc, iMac + hppl->dAlloc, 0x01<<hppl->dgShift);
		hpplNew->fUseCount = hppl->fUseCount;
		bltbh(hppl->rg, hpplNew->rg, iMac * cbItem);
	     /* hpplNew->iMac = iMac;  /* This is not needed because hppl->iMac
					  will be over-written later */
		FreeHppl(hppl);
		*phpvPl = hppl = hpplNew;
		}
	bltbh(hpv, &hppl->rg[iMac * cbItem], cbItem);
	hppl->iMac = iMac + 1;
#ifdef DEBUG
	Assert(FValidPl(*phpvPl));
#endif // DEBUG
	return(iMac);
}
#pragma NATIVE_END
/*<<plex*/


/* Not used for Mac */
#ifndef MAC
/*----------------------------------------------------------------------------
|	ILookAddPl
|
|	Searces a PL for an item and adds the item if it's not already there.
|
|	Arguments:
|		phppl	Pointer to PL.  May change if reallocated.
|		hp	Item to find/add.
|		pfnSgn	Comparison function
|
|	Returns:
|		Index of item found/added.
|
|	Notes:
|		The PL must already be created.
|		May DoJmp(penvMem) when reallocating.
----------------------------------------------------------------------------*/
#ifdef DDE
int ILookAddPl(phpvPl, hpv, pfnSgn)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
int (*pfnSgn)();
{
	int i;
#define phppl ((PL HUGE **)phpvPl)

	Assert(!(*phppl)->fUseCount);
	if ((i = ILookupPl(*phpvPl, hpv, pfnSgn)) == -1)
		i = IAddPl(phpvPl, hpv);
	return(i);
#undef phppl
}
#endif
#endif

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, IAddNewPl)
#else
#pragma alloc_text(window2, IAddNewPl)
#endif
#endif
/*----------------------------------------------------------------------------
|	IAddNewPl
|
|	Adds an item to a PL, creating the PL if it's initially NULL.
|
|	Arguments:
|		phppl		pointer to PL
|		hp		pointer to item to add
|		cbItem		size of item
|		dg		data group to alloc plex in
|
|	Returns:
|		the index of item added, if successful
|		-1, if out-of-memory
----------------------------------------------------------------------------*/
/*<<mcmain*/
#pragma NATIVE_START
int IAddNewPl(phpvPl, hpv, cbItem, dg)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
int cbItem;
int dg;
{
	int i;
	ENV *penvSav, env;
#ifdef DEBUG
	BOOL fShakeMemSav = 2;
	extern BOOL fShakeMem;
#endif

#define phppl ((PL HUGE **)phpvPl)

	Assert(FNullHp(*phppl) || !(*phppl)->fUseCount);
	i = -1;
	penvSav = penvMem;
	if (SetJmp(penvMem = &env) == 0)
		{
		if (SbOfHp(*phppl) == sbNull)
			{
			*phppl = HpplAlloc(cbItem, 5, 5, dg);
#ifdef DEBUG
			/* Turn off shake mem in this case, since we don't accomplish
				any additional memory checking when we've just allocated
				the plex from scratch anyway. */

			Assert(fShakeMem != 2);
			fShakeMemSav = fShakeMem;
			fShakeMem = FALSE;
#endif /* DEBUG */
			}
		Assert((*phppl)->cbItem == cbItem);
		i = IAddPl((VOID HUGE **)phppl, hpv);
		}
	penvMem = penvSav;
#ifdef DEBUG
	if (fShakeMemSav != 2)
		fShakeMem = fShakeMemSav;
#endif /* DEBUG */
	return(i);
#undef phppl
}
#pragma NATIVE_END
/*<<plex*/

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, IAddNewPlPos)
#else
#pragma alloc_text(window2, IAddNewPlPos)
#endif
#endif
/*----------------------------------------------------------------------------
|	IAddNewPlPos
|
|	Inserts an item into a plex at a specific position.
|
|	Arguments:
|		the index of the item added, if successful
|		-1 if out-of-memory
----------------------------------------------------------------------------*/
/*<<mcmain*/
#pragma NATIVE_START
int IAddNewPlPos(phpvPl, hpv, cbItem, i, dg)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
int cbItem;
int i;
int dg;
{
	BYTE HUGE *hpT;
#define phppl ((PL HUGE **)phpvPl)

	Assert(FNullHp(*phppl) || !(*phppl)->fUseCount);
	if (IAddNewPl((VOID HUGE **)phppl, hpv, cbItem, dg) == -1)
		return(-1);
	Assert(i < (*phppl)->iMac);
	hpT = &(*phppl)->rg[i * cbItem];
	bltbh(hpT, hpT + cbItem, ((*phppl)->iMac - i - 1) * cbItem);
	bltbh(hpv, hpT, cbItem);
#ifdef DEBUG
	Assert(FValidPl(*phppl));
#endif // DEBUG
	return(i);
#undef phppl
}
#pragma NATIVE_END
/*<<plex*/

#ifdef MAC
#pragma alloc_text(window, IAddPlSort)
#else
#pragma alloc_text(window2, IAddPlSort)
#endif
int IAddPlSort(phpvPl, hpv, pfnSgn)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
int (*pfnSgn)();
{
	int i;
#ifdef DEBUG
	int iOld;
#endif
	Assert(!FNullHp(*phpvPl));

	if (FLookupSortedPl(*phpvPl, hpv, &i, pfnSgn))
		return(-1);

#ifdef DEBUG
	iOld = i;
#endif
	i = IAddNewPlPos(phpvPl, hpv, (*(PL HUGE **)phpvPl)->cbItem, i, 1<<(*(PL HUGE **)phpvPl)->dgShift);
	Assert(i == -1 || i == iOld);
	return(i);
}


/*----------------------------------------------------------------------------
|	FCompactPl
|
|	Squeezes unused memory out of a PL
|
|	Arguments:
|		phppl		Plex to compact
|		fFull		fTrue if maximal compaction should be made
|
|	Returns:
|		fTrue if some memory was freed.
----------------------------------------------------------------------------*/
BOOL FCompactPl(phpvPl, fFull)
VOID HUGE **phpvPl;
BOOL fFull;
{
	int iMac, cbItem, cbFree, dAlloc;
	BYTE HUGE *hp;
	PL HUGE *hppl = *phpvPl;
	BOOL fFreed = fFalse;

	if (SbOfHp(hppl) != sbNull)
		{
#ifdef DEBUG
		Assert(FValidPl(hppl));
#endif // DEBUG
		if ((iMac = hppl->iMac) == 0 && fFull)
			{
			FreeHppl(hppl);
			fFreed = fTrue;
			*phpvPl = hpNull;
			}
		else
			{
			if (!fFull)
				{
				dAlloc = hppl->dAlloc;
				iMac = (iMac + dAlloc) / dAlloc * dAlloc;
				}
			hp = &hppl->rg[iMac * (cbItem = hppl->cbItem)];
			if (IbOfHp(hp) & 1)
				{
				/* can't free on an odd boundary */
				Assert((cbItem & 1) && (iMac & 1));
				iMac++;
				hp += cbItem;
				}
			Assert((IbOfHp(hp) & 1) == 0);
			/* must free at least 4 bytes, and it had better
			   be an even number */
			if ((cbFree = WAlign((hppl->iMax - iMac) * cbItem)) >= 4)
				{
				CbChopHp(hp, cbFree);
				fFreed = fTrue;
				hppl->iMax = iMac;
				}
			}
		}
#ifdef DEBUG
	Assert(SbOfHp(*phpvPl) == sbNull || FValidPl(*phpvPl));
#endif // DEBUG
	return(fFreed);
}

/*----------------------------------------------------------------------------
|	CbPlAlloc
|
|	Returns amount of memory allocated to the given PL
|
|	Arguments:
|		hppl		PL to return info for.
|
|	Returns:
|		memory allocated to the PL
----------------------------------------------------------------------------*/
/*<<mcmain*/
#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, CbPlAlloc)
#else
#pragma alloc_text(window2, CbPlAlloc)
#endif
#endif
#pragma NATIVE_START
int CbPlAlloc(hpvPl)
VOID HUGE *hpvPl;
{
#define hppl ((PL HUGE *)hpvPl)
	if (SbFromHp(hppl) == sbNull)
		return(0);
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	return(WAlign(cbPL + (hppl->iMax * hppl->cbItem)));
#undef hppl
}
#pragma NATIVE_END
/*<<plex*/

#ifndef GRAF
#ifdef MAC
#pragma alloc_text(window, ResizePl)
#else
#pragma alloc_text(window2, ResizePl)
#endif
#endif
/*----------------------------------------------------------------------------
|	ResizePl
|
|	Grows the given plex large enough to contain the given number
|	of items.
|
|	Arguments:
|		phppl		plex to reallocate - will change if
|				the plex is grown.
|		iMac		number of items the plex must hold.
|
|	Note:
|		DoJmp(penvMem) if out-of-memory.
----------------------------------------------------------------------------*/
/*<<mcmain*/
#pragma NATIVE_START
ResizePl(phpvPl, iMac, iIns)
VOID HUGE **phpvPl;
int iMac, iIns;
{
	int iMax;
	int cbItem;
	PL HUGE *hppl;
#define phppl ((PL HUGE **)phpvPl)

	hppl = *phppl;
	Assert(!hppl->fUseCount);		// NYI
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	cbItem = hppl->cbItem;
	if (hppl->iMax < iMac)
		{
		iMax = (iMac / (int) hppl->dAlloc + 1) * hppl->dAlloc;
		*phppl = HpplAlloc(cbItem, hppl->dAlloc, iMax,
				0x01<<hppl->dgShift);
#ifdef USECOUNTRESIZE
		(*phppl)->fUseCount = hppl->fUseCount;
#endif		
		if (iIns >= 0)
			{
			bltbh(hppl->rg, (*phppl)->rg, iIns * cbItem);
			bltbh(hppl->rg + iIns * cbItem,
				(*phppl)->rg + (iIns+(iMac-hppl->iMac)) * cbItem,
				(hppl->iMac-iIns) * cbItem);
			}
		else
			bltbh(hppl->rg, (*phppl)->rg, hppl->iMax * cbItem);
		FreeHppl(hppl);
		}
	else if (iIns >= 0)
		{
		bltbh(hppl->rg + iIns * hppl->cbItem,
			hppl->rg + (iIns+(iMac-hppl->iMac)) * cbItem,
			(hppl->iMac-iIns) * cbItem);
		}
	(*phppl)->iMac = iMac;
#ifdef DEBUG
	Assert(FValidPl(*phppl));
#endif // DEBUG
#undef phppl
}
#pragma NATIVE_END
/*<<plex*/


#ifdef MAC
#pragma alloc_text(window, IIncUseCountPl)
#else
#pragma alloc_text(window2, IIncUseCountPl)
#endif
int IIncUseCountPl(hpvPl, i)
VOID HUGE *hpvPl;
int i;
	{
#define hppl ((PL HUGE *)hpvPl)
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	Assert(i < hppl->iMac);
	return ++(*(int HUGE *)&hppl->rg[i*hppl->cbItem]);
#undef hppl
	}

#pragma NATIVE_START
int IDecUseCountPl(hpvPl, i)
VOID HUGE *hpvPl;
int i;
	{
#define hppl ((PL HUGE *)hpvPl)
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	Assert(i < hppl->iMac);
	Assert((*(int HUGE *)&hppl->rg[i*hppl->cbItem]) != 0);
	return --(*(int HUGE *)&hppl->rg[i*hppl->cbItem]);
#undef hppl
	}
#pragma NATIVE_END

int IAddUseCountPl(phpvPl, hpv)
VOID HUGE **phpvPl;
VOID HUGE *hpv;
	{
	BYTE HUGE *hpb;
	int cbItem;
	int i;
	int iMac;
	PL HUGE *hppl;

	hppl = *phpvPl;
#ifdef DEBUG
	Assert(FValidPl(hppl));
#endif // DEBUG
	Assert(hppl->fUseCount);
	cbItem = hppl->cbItem;
	iMac = hppl->iMac;
	// Search for an unused entry
	for (i = 0, hpb = hppl->rg; i < iMac; i++, hpb += cbItem)
		{
		if ((*(int HUGE *) hpb) == 0)
			{
			bltbh(hpv, hpb, cbItem);
			(*(int HUGE *) hpb) = 1;
			return i;
			}
		}
	return IAddPl(phpvPl, hpv);
	}

#ifdef DEBUG
/*----------------------------------------------------------------------------
|	FValidPl
|
|	Checks for a valid PL structure.
|
|	Arguments:
|		hppl		PL to check
|
|	Returns:
|		fTrue if the PL looks reasonable.
----------------------------------------------------------------------------*/
BOOL FValidPl(hpvPl)
VOID HUGE *hpvPl;
{
#define hppl ((PL HUGE *)hpvPl)
	if (SbOfHp(hppl) == sbNull ||
			hppl->cbItem == 0 ||
			hppl->iMac < 0 ||
			hppl->iMax < 0 ||
			hppl->iMax < hppl->iMac)
		return(fFalse);
	return(fTrue);
#undef hppl
}
#endif

/*----------------------------------------------------------------------------
END OF THE CODE FROM EXCEL
----------------------------------------------------------------------------*/
#endif
