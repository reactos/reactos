

#include <Atom.h>
#include <process.h>
#include <thread.h>
#include <wstring.h>


/*
	title:	atom.c
	author: Boudewijn Dekker
	hsitory:copied from twin wine source
	modified:	-- add wide char support
			-- removed ex functions
			-- use a per process local atom table
	todo :	
		check initatomtable
		check if not calling down to ntdll conflicts with anything
		check anis2unicode procedure 
*/


/* system global and local atom tables */

static ATOMTABLE GlobalAtomTable;

/* internal functions */
ATOM GLDeleteAtom(ATOMTABLE *at, ATOM nAtom);
ATOM AWGLAddAtom( ATOMTABLE *at, const wchar_t *lpString);
ATOM AWGLFindAtom(ATOMTABLE *at, const wchar_t *lpString);
UINT AWGLGetAtomName(ATOMTABLE *at,ATOM atom, wchar_t *lpString, int nSize);

static ATOMENTRY *GetAtomPointer(ATOMTABLE *,int);
static ATOMID    AtomHashString(const wchar_t *,int *);

#define ATOMBASE	 0xcc00



ATOM
STDCALL
GlobalDeleteAtom(
    ATOM nAtom
    )
{
	return GLDeleteAtom(&GlobalAtomTable, nAtom);
}


BOOL
STDCALL
InitAtomTable(
    DWORD nSize
    )
{
// nSize should be a prime number
	
	if ( nSize < 4 || nSize >= 512 ) {
		nSize = 37;
	}
	/*
	if ( GetTeb()->pPeb->LocalAtomTable == NULL ) {
		GetTeb()->pPeb->LocalAtomTable = (ATOMTABLE *)malloc(nSize*sizeof(ATOMTABLE));
	}
	
	GetTeb()->pPeb->LocalAtomTable->TableSize = nSize;
	*/
	return TRUE;
}


ATOM
STDCALL
DeleteAtom(
    ATOM nAtom
    )
{
	return GLDeleteAtom(&GetTeb()->pPeb->LocalAtomTable, nAtom);
	
}




ATOM
STDCALL
GlobalAddAtomA(
    const char *lpString
    )
{

	UINT	 BufLen = strlen(lpString);
	wchar_t *lpBuffer = (wchar_t *)malloc(BufLen*sizeof(wchar_t));
	ATOM atom;
	ansi2unicode(lpBuffer, lpString,BufLen);
	atom = AWGLAddAtom(&GlobalAtomTable,lpBuffer );
	free(lpBuffer);
	return atom;
}





ATOM
STDCALL
GlobalAddAtomW(
    const wchar_t *lpString
    )
{
	return AWGLAddAtom(&GlobalAtomTable, lpString);	
}


ATOM
STDCALL
GlobalFindAtomA(
    const char *lpString
    )
{
	ATOM 	a;
	UINT	BufLen = strlen(lpString);
	wchar_t *lpBuffer = (wchar_t *)malloc(BufLen*sizeof(wchar_t));
	ansi2unicode(lpBuffer, lpString,BufLen);
	a = AWGLFindAtom(&GlobalAtomTable, lpBuffer);
	free(lpBuffer);
	return a;
}


ATOM
STDCALL
GlobalFindAtomW(
    const wchar_t *lpString
    )
{
	return AWGLFindAtom(&GlobalAtomTable, lpString);	
}



UINT
STDCALL
GlobalGetAtomNameA(
    ATOM nAtom,
    char *lpBuffer,
    int nSize
    )
{
	
	wchar_t *lpUnicode = (wchar_t *)malloc(nSize *sizeof(wchar_t));
	UINT x = AWGLGetAtomName(&GlobalAtomTable,nAtom, lpUnicode,nSize);	
	unicode2ansi(lpBuffer,lpUnicode,nSize);
	free(lpUnicode);
	return x;
}


UINT
STDCALL
GlobalGetAtomNameW(
    ATOM nAtom,
    wchar_t * lpBuffer,
    int nSize
    )
{
	return AWGLGetAtomName(&GlobalAtomTable, nAtom, lpBuffer, nSize);	
}


ATOM
STDCALL
AddAtomA(
    const char *lpString
    )
{
	UINT	BufLen = strlen(lpString);
	wchar_t *lpBuffer = (wchar_t*)malloc(BufLen*2);
	ATOM a;
	ansi2unicode(lpBuffer, lpString,BufLen);
	a = AWGLAddAtom(&GetTeb()->pPeb->LocalAtomTable, lpBuffer);
	free(lpBuffer);
	return a;
	
}


ATOM
STDCALL
AddAtomW(
    const wchar_t * lpString
    )
{
	return AWGLAddAtom(&GetTeb()->pPeb->LocalAtomTable, lpString);
}




ATOM
STDCALL
FindAtomA(
    const char *lpString
    )
{
	UINT	BufLen = strlen(lpString);
	wchar_t *lpBuffer = (wchar_t *)malloc(BufLen*2);
	ATOM a;
	ansi2unicode(lpBuffer, lpString,BufLen);
	a = AWGLFindAtom(&GetTeb()->pPeb->LocalAtomTable, lpBuffer);
	free(lpBuffer);
	return a;
}


ATOM
STDCALL
FindAtomW(
    const wchar_t * lpString
    )
{
	return AWGLFindAtom(&GetTeb()->pPeb->LocalAtomTable, lpString);
}



UINT
STDCALL
GetAtomNameA(
    ATOM nAtom,
    char  *lpBuffer,
    int nSize
    )
{
	LPWSTR lpUnicode = (wchar_t *)malloc(nSize *2);
	UINT x = AWGLGetAtomName(&GlobalAtomTable, nAtom,lpUnicode,nSize);	
	unicode2ansi(lpBuffer,lpUnicode,nSize);
	free(lpUnicode);
	return x;
}


UINT
STDCALL
GetAtomNameW(
    ATOM nAtom,
    wchar_t * lpBuffer,
    int nSize
    )
{
	return AWGLGetAtomName(&GetTeb()->pPeb->LocalAtomTable,nAtom,lpBuffer,  nSize);
}

ATOM
GLDeleteAtom(
    ATOMTABLE *at, ATOM nAtom
    )
{

	ATOMENTRY *lp;

	/* a free slot has q == 0 && refcnt == 0 */
	if((lp = GetAtomPointer(at,nAtom - ATOMBASE))) {
		if(lp->idsize)
			lp->refcnt--;

		if(lp->refcnt == 0) {
			free(at->AtomTable);
			at->AtomTable = NULL;
			free(at->AtomData);
			at->AtomData = NULL;
			return lp->q = 0;
		}
	}
	return nAtom;



}


ATOM
AWGLAddAtom(
     ATOMTABLE *at, const wchar_t *lpString
  	)
{
	ATOM 		atom;
	ATOMID		q;
	LPATOMENTRY   	lp,lpfree;
	int		index,freeindex;
	int		atomlen;
	int		newlen;
	
	
	
	/* if we already have it, bump refcnt */
	if((atom = AWGLFindAtom(at, lpString ))) {
		lp = GetAtomPointer(at,atom - ATOMBASE);
		if(lp->idsize) lp->refcnt++;
		return atom;
	}

	/* add to a free slot */
	q = AtomHashString(lpString,&atomlen);

	lpfree 	  = 0;
	freeindex = 0;

	for(index = 0;(lp = GetAtomPointer(at,index));index++) {
		if(lp->q == 0 && lp->refcnt == 0) {	
			if(lp->idsize > atomlen) {
				if ((lpfree == 0) ||
					    (lpfree->idsize > lp->idsize)) {
					lpfree = lp;
					freeindex = index;
				}
			}
		}
	}
	/* intatoms do not take space in data, but do get new entries */
	/* an INTATOM will have length of 0 			      */
	if(lpfree && atomlen) {
		lpfree->q = q;
		lpfree->refcnt = 1;
		wcsncpy(&at->AtomData[lpfree->idx],lpString,atomlen);
		return freeindex + ATOMBASE;
	}

	/* no space was available, or we have an INTATOM		*/
	/* so expand or create the table 				*/
	if(at->AtomTable == 0) {
		at->AtomTable = (ATOMENTRY *) malloc(sizeof(ATOMENTRY));	
		at->TableSize = 1;
		lp = at->AtomTable;
		index = 0;
	} else {
		at->TableSize++;
		at->AtomTable = (ATOMENTRY *) realloc(
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
			at->AtomData = (wchar_t *) malloc(newlen*2);
			lp->idx = 0;
		} else {
			
			at->AtomData = (wchar_t *) realloc(at->AtomData,newlen*2);
			lp->idx = at->DataSize;
		}

		wcscpy(&at->AtomData[lp->idx],lpString);
		at->DataSize = newlen;
	}	

	return index + ATOMBASE;
}










ATOM
AWGLFindAtom(
     ATOMTABLE *at, const wchar_t *lpString
    )
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
			if(wcsicmp(&at->AtomData[lp->idx],lpString) == 0)
				return ATOMBASE + index;
		}
	}
	return 0;
}


UINT
AWGLGetAtomName(ATOMTABLE *at, ATOM atom, wchar_t *lpString,int len)
{
	
	ATOMENTRY	*lp;
	wchar_t 	*atomstr;
	int		atomlen;
	
	
	
	/* return the atom name, or create the INTATOM */
	if((lp = GetAtomPointer(at,atom - ATOMBASE))) {
		if(lp->idsize) {
			atomlen = wcslen(atomstr = &at->AtomData[lp->idx]);
			if (atomlen < len)
			    wcscpy(lpString,atomstr);
			else {
			    wcsncpy(lpString,atomstr,len-1);
			    lpString[len-1] = '\0';
			}
			return (UINT)wcslen(lpString);
		} else {
			//wsprintf((wchar *)lpString,"#%d",lp->q);
			return (UINT)wcslen(lpString);
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
AtomHashString(const wchar_t * lp,int *lplen)
{
	ATOMID 	q;
	wchar_t   *p,ch;
	int	len;

	/* if we have an intatom... */
	if(HIWORD(lp) == 0) {
		if(lplen) *lplen = 0;
		return (ATOMID)lp;
	}

	/* convert the string to an internal representation */
	for(p=(wchar_t *)lp,q=0,len=0;(p++,ch=*p++);len++)
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

int ansi2unicode( wchar_t *uni, char *ansi, int s)
{
	register int i;
	
	for(i=0;i<=s;i++) 
		uni[i] = (wchar_t)ansi[i];
	return;
}

int unicode2ansi( char *ansi, wchar_t *uni, int s)
{
	register int i;
	
	for(i=0;i<=s;i++) 
		ansi[i] = (char)uni[i];
	return;
}





