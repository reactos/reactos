/* $Id: atom.c,v 1.12 2001/03/31 01:17:29 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <kernel32/atom.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
//#include <stdlib.h>

/*
 * System global and local atom tables.
 * What does "global" mean? The scope is
 * the attached process or ANY process?
 * In the second case, we need to call 
 * csrss.exe.
 */

#define iswlower(c)	(c >= L'a' && c <= L'z')

#if 1

static ATOMTABLE GlobalAtomTable;
static ATOMTABLE LocalAtomTable;

/* internal functions */
ATOM GLDeleteAtom(ATOMTABLE *at, ATOM nAtom);
ATOM AWGLAddAtom( ATOMTABLE *at, const WCHAR *lpString);
ATOM AWGLFindAtom(ATOMTABLE *at, const WCHAR *lpString);
UINT AWGLGetAtomName(ATOMTABLE *at,ATOM atom, WCHAR *lpString, int nSize);

static ATOMENTRY *GetAtomPointer(ATOMTABLE *,int);
static ATOMID    AtomHashString(const WCHAR *,int *);

#define ATOMBASE	 0xcc00

int unicode2ansi( char *ansi,const WCHAR *uni, int s);
int ansi2unicode( WCHAR *uni,const char *ansi, int s);


ATOM STDCALL
GlobalDeleteAtom(ATOM	nAtom)
{
  return GLDeleteAtom(&GlobalAtomTable, nAtom);
}


BOOL STDCALL
InitAtomTable(DWORD nSize)
{
  /* nSize should be a prime number */
	
  if ( nSize < 4 || nSize >= 512 )
    {
      nSize = 37;
    }
  
  if (LocalAtomTable.lpDrvData == NULL)
    {
      LocalAtomTable.lpDrvData =
	RtlAllocateHeap(GetProcessHeap(),
			(HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY),
			(nSize * sizeof (ATOMENTRY)));
    }
  
  return TRUE;
}


ATOM STDCALL
DeleteAtom(ATOM nAtom)
{
  return GLDeleteAtom(&LocalAtomTable, nAtom);
}

ATOM STDCALL
GlobalAddAtomA(LPCSTR lpString)
{
  UINT BufLen = strlen(lpString);
  WCHAR* lpBuffer;
  ATOM atom;
  
  lpBuffer = 
    (WCHAR *) RtlAllocateHeap(GetProcessHeap(),
			      (HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY),
			      (BufLen * sizeof (WCHAR)));
  ansi2unicode(lpBuffer, lpString, BufLen);
  atom = AWGLAddAtom(&GlobalAtomTable, lpBuffer);
  RtlFreeHeap(GetProcessHeap(), 0, lpBuffer);
  return(atom);
}

ATOM STDCALL
GlobalAddAtomW(LPCWSTR lpString)
{
  return AWGLAddAtom(&GlobalAtomTable, lpString);	
}


ATOM STDCALL
GlobalFindAtomA(LPCSTR lpString)
{
  ATOM 	a;
  UINT	BufLen = strlen(lpString);
  WCHAR *lpBuffer = (WCHAR *)RtlAllocateHeap(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,BufLen*sizeof(WCHAR));
  ansi2unicode(lpBuffer, lpString,BufLen);
  a = AWGLFindAtom(&GlobalAtomTable, lpBuffer);
  RtlFreeHeap(GetProcessHeap(),0,lpBuffer);
  return a;
}


ATOM STDCALL
GlobalFindAtomW(const WCHAR *lpString)
{
  return AWGLFindAtom(&GlobalAtomTable, lpString);	
}



UINT STDCALL
GlobalGetAtomNameA(ATOM nAtom,
		   char *lpBuffer,
		   int nSize)
{	
  WCHAR *lpUnicode = (WCHAR *)RtlAllocateHeap(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,nSize *sizeof(WCHAR));
  UINT x = AWGLGetAtomName(&GlobalAtomTable,nAtom, lpUnicode,nSize);	
  unicode2ansi(lpBuffer,lpUnicode,nSize);
  RtlFreeHeap(GetProcessHeap(),0,lpUnicode);
  return x;
}

UINT STDCALL
GlobalGetAtomNameW(ATOM nAtom,
		   WCHAR * lpBuffer,
		   int nSize)
{
  return AWGLGetAtomName(&GlobalAtomTable, nAtom, lpBuffer, nSize);	
}


ATOM STDCALL
AddAtomA(const char *lpString)
{
  UINT	BufLen = strlen(lpString);
  WCHAR *lpBuffer = (WCHAR*)RtlAllocateHeap(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,BufLen*2);
  ATOM a;
  ansi2unicode(lpBuffer, lpString,BufLen);
  a = AWGLAddAtom(&LocalAtomTable, lpBuffer);
  RtlFreeHeap(GetProcessHeap(),0,lpBuffer);
  return a;	
}


ATOM STDCALL
AddAtomW(const WCHAR * lpString)
{
  return AWGLAddAtom(&LocalAtomTable, lpString);
}

ATOM STDCALL
FindAtomA(const char *lpString)
{
  UINT	BufLen = strlen(lpString);
  WCHAR *lpBuffer = (WCHAR *)RtlAllocateHeap(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,BufLen*2);
  ATOM a;
  ansi2unicode(lpBuffer, lpString,BufLen);
  a = AWGLFindAtom(&LocalAtomTable, lpBuffer);
  RtlFreeHeap(GetProcessHeap(),0,lpBuffer);
  return a;
}

ATOM STDCALL
FindAtomW(const WCHAR * lpString)
{
  return AWGLFindAtom(&LocalAtomTable, lpString);
}

UINT STDCALL
GetAtomNameA(ATOM nAtom,
	     char  *lpBuffer,
	     int nSize)
{
  LPWSTR lpUnicode = (WCHAR *)RtlAllocateHeap(GetProcessHeap(),HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,nSize *2);
  UINT x = AWGLGetAtomName(&GlobalAtomTable, nAtom,lpUnicode,nSize);	
  unicode2ansi(lpBuffer,lpUnicode,nSize);
  RtlFreeHeap(GetProcessHeap(),0,lpUnicode);
  return x;
}

UINT STDCALL
GetAtomNameW(ATOM nAtom,
	     WCHAR * lpBuffer,
	     int nSize)
{
  return AWGLGetAtomName(&LocalAtomTable,nAtom,lpBuffer,  nSize);
}

ATOM
GLDeleteAtom(ATOMTABLE *at, ATOM nAtom)
{
  ATOMENTRY *lp;
  
  /* a free slot has q == 0 && refcnt == 0 */
  if((lp = GetAtomPointer(at,nAtom - ATOMBASE))) {
    if(lp->idsize)
      lp->refcnt--;
    
    if(lp->refcnt == 0) {
      RtlFreeHeap(GetProcessHeap(),0,at->AtomTable);
      at->AtomTable = NULL;
      RtlFreeHeap(GetProcessHeap(),0,at->AtomData);
      at->AtomData = NULL;
      return lp->q = 0;
    }
  }
  return nAtom;
}


ATOM
AWGLAddAtom(ATOMTABLE *at, const WCHAR *lpString)
{
  ATOM 		atom;
  ATOMID		q;
  LPATOMENTRY   	lp,lpfree;
  int		index,freeindex;
  int		atomlen;
  int		newlen;
	  	
  /* if we already have it, bump refcnt */
  if((atom = AWGLFindAtom(at, lpString ))) 
    {
      lp = GetAtomPointer(at,atom - ATOMBASE);
      if(lp->idsize) lp->refcnt++;
      return atom;
    }

  /* add to a free slot */
  q = AtomHashString(lpString,&atomlen);
  
  lpfree = 0;
  freeindex = 0;
  
  for(index = 0;(lp = GetAtomPointer(at,index));index++) 
    {
      if(lp->q == 0 && lp->refcnt == 0) 
	{	
	  if(lp->idsize > atomlen) 
	    {
	      if ((lpfree == 0) ||
		  (lpfree->idsize > lp->idsize)) 
		{
		  lpfree = lp;
		  freeindex = index;
		}
	    }
	}
    }
  /* intatoms do not take space in data, but do get new entries */
  /* an INTATOM will have length of 0 			      */
  if(lpfree && atomlen) 
    {
      lpfree->q = q;
      lpfree->refcnt = 1;
      lstrcpynW(&at->AtomData[lpfree->idx],lpString,atomlen);
      return freeindex + ATOMBASE;
    }
  
  /* no space was available, or we have an INTATOM		*/
  /* so expand or create the table 				*/
  if(at->AtomTable == 0) 
    {
      at->AtomTable = 
	(ATOMENTRY*) RtlAllocateHeap(GetProcessHeap(),
				     HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
				     sizeof(ATOMENTRY));	
      at->TableSize = 1;
      lp = at->AtomTable;
      index = 0;
    } 
  else 
    {
      at->TableSize++;
      at->AtomTable = 
	(ATOMENTRY*) RtlReAllocateHeap(GetProcessHeap(),0,
				       (LPVOID) at->AtomTable,
				       at->TableSize * sizeof(ATOMENTRY));
      lp = &at->AtomTable[at->TableSize - 1];
    }
  
  /* set in the entry */
  lp->refcnt = 1;
  lp->q      = q;
  lp->idsize = atomlen;
  lp->idx    = 0;

  /* add an entry if not intatom... */
  if(atomlen) {
    newlen = at->DataSize + atomlen;
    
    if(at->AtomData == 0) {
      at->AtomData = 
	(WCHAR*)RtlAllocateHeap(GetProcessHeap(),
				HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
				newlen*2);
      if (at->AtomData == NULL)
	{
	  return(0);
	}
      lp->idx = 0;
    } else {
      
      at->AtomData =
	(WCHAR*)RtlReAllocateHeap(GetProcessHeap(), 0, at->AtomData, newlen*2);
      if (at->AtomData == NULL)
	{
	  return(0);
	}
      lp->idx = at->DataSize;
    }
    
    lstrcpyW(&at->AtomData[lp->idx],lpString);
    at->DataSize = newlen;
  }	
  
  return index + ATOMBASE;
}










ATOM
AWGLFindAtom(ATOMTABLE *at, const WCHAR *lpString)
{

  ATOMID		q;
  LPATOMENTRY   	lp;
  int		index;
  int		atomlen;
	
      
  /* convert string to 'q', and get length */
  q = AtomHashString(lpString,&atomlen);
  
  /* find the q value, note: this could be INTATOM */
  /* if q matches, then do case insensitive compare*/
  for(index = 0;(lp = GetAtomPointer(at,index));index++) {
    if(lp->q == q) {	
      if(HIWORD(lpString) == 0)
	return ATOMBASE + index;
      if(lstrcmpiW(&at->AtomData[lp->idx],lpString) == 0)
	return ATOMBASE + index;
    }
  }
  return 0;
}


UINT
AWGLGetAtomName(ATOMTABLE *at, ATOM atom, WCHAR *lpString,int len)
{
	
	ATOMENTRY	*lp;
	WCHAR 	*atomstr;
	int		atomlen;
	
	
	
	/* return the atom name, or create the INTATOM */
	if((lp = GetAtomPointer(at,atom - ATOMBASE))) {
		if(lp->idsize) {
			atomlen = lstrlenW(atomstr = &at->AtomData[lp->idx]);
			if (atomlen < len)
			    lstrcpyW(lpString,atomstr);
			else {
			    lstrcpynW(lpString,atomstr,len-1);
			    lpString[len-1] = '\0';
			}
			return (UINT)lstrlenW(lpString);
		} else {
			//wsprintf((wchar *)lpString,"#%d",lp->q);
			return (UINT)lstrlenW(lpString);
		}
	}
	return 0;
}


/********************************************************/
/* convert alphanumeric string to a 'q' value. 		*/
/* 'q' values do not need to be unique, they just limit */
/* the search we need to make to find a string		*/
/********************************************************/

static ATOMID
AtomHashString(const WCHAR * lp,int *lplen)
{
	ATOMID 	q;
	WCHAR   *p,ch;
	int	len;

	/* if we have an intatom... */
	if(HIWORD(lp) == 0) {
		if(lplen) *lplen = 0;
		return (ATOMID)lp;
	}

	/* convert the string to an internal representation */
	for(p=(WCHAR *)lp,q=0,len=0;(p++,ch=*p++);len++)
		q = (q<<1) + iswlower(ch)?towupper(ch):ch;

	/* 0 is reserved for empty slots */
	if(q == 0)
		q++;

	/* avoid strlen later */
	/* check out with unicode */
	if(lplen) {
		*lplen = ++len;
	}
	return q;
}

/********************************************************/
/*	convert an atom index into a pointer into an 	*/
/* 	atom table.  This validates the pointer is in   */
/*	range, and that the data is accessible		*/
/********************************************************/

static ATOMENTRY *
GetAtomPointer(ATOMTABLE *at,int index)
{
	ATOMENTRY *lp;
	
	/* if no table, then no pointers */
	if(at->AtomTable == 0)
		return 0;

	/* bad index */
	if((index < 0) || (index >= at->TableSize))
		return 0;

	/* we have a pointer */
	lp = &at->AtomTable[index];


	/* is the index past stored data, validity check		*/
	/* LATER: is the size of the entry within the available space 	*/
	if(lp->idx > at->DataSize)
		return 0;

	return lp;
}

int ansi2unicode( WCHAR *uni,const char *ansi, int s)
{
	register int i;
	
	for(i=0;i<=s;i++) 
		uni[i] = (WCHAR)ansi[i];
	return i;
}

int
unicode2ansi( char *ansi,const WCHAR *uni, int s)
{
	register int i;
	
	for(i=0;i<=s;i++) 
		ansi[i] = (char)uni[i];
	return i;
}


#endif

/* EOF */
