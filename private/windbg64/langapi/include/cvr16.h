// CVR: CodeView Record utilities

#if CC_CVTYPE32
#include "./cvr32.h"
#else

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

#ifndef __CVR_INCLUDED__
#define __CVR_INCLUDED__

#ifndef __PDB_INCLUDED__
#include <pdb.h>
#endif
#ifndef _CV_INFO_INCLUDED
#include <cvinfo.h>
#endif
#ifndef _INC_STDDEF
#include <stddef.h>
#endif
#ifndef _WINDOWS_
// get rid of baggage we don't need from windows.h
#define WIN32_LEAN_AND_MEAN
//#define NOGDI
#define NOUSER
#define NONLS
#include "windows.h"
#endif

typedef BYTE* PB;
typedef long CB;
typedef char* SZ;		// zero terminated string
typedef char* ST;		// length prefixed string
typedef SYMTYPE* PSYM;
typedef SYMTYPE UNALIGNED * PSYMUNALIGNED;
typedef TYPTYPE* PTYPE;

//////////////////////////////////////////////////////////////////////////////
// TII (type index iterator) implementation declarations

typedef ptrdiff_t IB;

struct TYTI { // position of type indices within a type record with the given leaf
	USHORT leaf;
	SZ sz;						// leaf type name
	short cib;
	const IB* rgibTI;
	TI* (*pfn)(PTYPE ptype, int iib, PB* ppb, PB pbEnd); // fn to call if cib == cibFunction
	PB  (*pfnPbAfter)(void* pv);	  // end of record fn to call for elements of a field list
};

// all pointers to a TYTI are pointing to const TYTIs
typedef const TYTI *	PTYTI;

struct SYTI { // position of symbol indices within a symbol recoord with the given rectyp
	USHORT rectyp;
	SZ sz;						// symbol rectyp name
	IB	ibName;					// position of symbol name
	ST (*pfnstName)(PSYM psym);	// function to call if name offset variable
	BOOL  isGlobal;				// symbol is global
	short cib;
	const IB* rgibTI;
};

// all pointers to a SYTI are pointing to const SYTIs
typedef const SYTI *	PSYTI;


#if	defined(PDB_LIBRARY)
#define CVR_EXPORT
#else
#if defined(CVR_IMP)
#define CVR_EXPORT	__declspec(dllexport)
#else
#define CVR_EXPORT	__declspec(dllimport)
#endif
#endif

#ifndef CVRAPI
#define CVRAPI	 __cdecl
#endif

class SymTiIter { // type indices within symbol record iterator
public:
	CVR_EXPORT	SymTiIter(PSYM psym_);
	inline TI&  rti();
	inline BOOL next();
private:
	PSYM psym;			// current symbol
	int  iib;			// index of curren TI in this symbol record
	PSYTI psyti;		// address of symbol->ti-info for current symbol record
};

inline TI& SymTiIter::rti()
{
	return *(TI*)((PB)psym + psyti->rgibTI[iib]);
}

inline BOOL SymTiIter::next()
{
	return ++iib < psyti->cib;
}

class TypeTiIter { // type indices within type record iterator
public:
	TypeTiIter(TYPTYPE* ptype);
	inline TI& rti();
	BOOL next();
	PB pbFindField(unsigned leaf);
private:
	void init();
	BOOL nextField();

	PTYPE ptype;		// current type
	USHORT* pleaf;		// leaf part of current type
	unsigned	leaf;	// cached, aligned, no op-size override version of leaf
	PB   pbFnState;		// private state of current iterating fn (iff ptyti->cib == cibFunction)
	PB   pbEnd;			// pointer just past end of type record
	int  iib;  			// index of current TI in this type record
	BOOL isFieldList;	// TRUE if this type record is a LF_FIELDLIST
	TI*  pti;			// address of current TI
	PTYTI ptyti;		// address of type->ti-info for current type record
};

inline TI& TypeTiIter::rti()
{
	return *pti;
}

// utility function protos
CVR_EXPORT BOOL CVRAPI fGetSymName(PSYM psym, OUT ST* pst);
		   BOOL fSymIsGlobal(PSYM psym);
		   BOOL fGetTypeLeafName(PTYPE ptype, OUT SZ* psz);
CVR_EXPORT BOOL CVRAPI fGetSymRecTypName(PSYM psym, OUT SZ* psz);

////////////////////////////////////////////////////////////////////////////////
// Inline utility functions.

// Return the number of bytes in an ST
inline CB cbForSt(ST st)
{
	return *(PB)st + 1;
}

// Return the number of bytes the type record occupies.
//
inline CB cbForType(PTYPE ptype)
{
	return ptype->len + sizeof(ptype->len);
}

// Return a pointer to the byte just past the end of the type record.
//
inline PB pbEndType(PTYPE ptype)
{
	return (PB)ptype + cbForType(ptype);
}

// Return the number of bytes the symbol record occupies.
//
#define MDALIGNTYPE_	DWORD

inline CB cbAlign_(CB cb)
{
	return ((cb + sizeof(MDALIGNTYPE_) - 1)) & ~(sizeof(MDALIGNTYPE_) - 1);
}

inline CB cbForSym(PSYMUNALIGNED psym)
{
	CB cb = psym->reclen + sizeof(psym->reclen);
	// procrefs also have a hidden length preceeded name following the record
	if ((psym->rectyp == S_PROCREF) || (psym->rectyp == S_LPROCREF))
		cb += cbAlign_(cbForSt((ST)((PB)psym + cb)));
	return cb;
}

// Return a pointer to the byte just past the end of the symbol record.
//
inline PB pbEndSym(PSYM psym)
{
	return (PB)psym + cbForSym(psym);
}

#endif // CC_CVTYPE32
#endif // __CVR_INCLUDED__
