// HTML lexer tables
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __HLTABLE_H__
#define __HLTABLE_H__

#include "lexhtml.h"

#ifndef PURE
#define PURE =0
#endif

// These values must match the number of built-in tables (CV_FIXED), 
// and the capacity of the inVariant bits of the lex state (CV_MAX).
const UINT CV_FIXED =  4; // Count Variants in Fixed tables
const UINT CV_MAX   = 16; // Count variants maximum total

// Macro for determining number of elements in an array
#define CELEM_ARRAY(a)  (sizeof(a) / sizeof(a[0]))

// length-limited string compare function pointer (e.g. strncmp/strnicmp)

typedef  int (_cdecl* PFNNCMP)(LPCTSTR, LPCTSTR, size_t);

#define CASE (TRUE)
#define NOCASE (FALSE)

#define NOT_FOUND (-1)

////////////////////////////////////////////////////////////////////////////

// return A-Z[a-z] index if alpha, else -1
inline int PeekIndex(TCHAR c, BOOL bCase /*= NOCASE*/)
{
	if ((c >= _T('A')) && (c <= _T('Z')))
		return c - _T('A');
	else if ((c >= _T('a')) && (c <= _T('z')))
		return c - _T('a') + (bCase ? 26 : 0);
	else
		return -1;
}

// static table lookups
int LookupLinearKeyword(ReservedWord *rwTable, int cel, RWATT_T att, LPCTSTR pchLine, int cbLen, BOOL bCase = NOCASE);
int LookupIndexedKeyword(ReservedWord *rwTable, int cel, int * indexTable, RWATT_T att, LPCTSTR pchLine, int cbLen, BOOL bCase = NOCASE);

// content model
// Map between element / lex state
//
struct ELLEX {
	LPCTSTR sz;
	int     cb;
	DWORD   lxs;
};

DWORD TextStateFromElement(LPCTSTR szEl, int cb);
ELLEX * pellexFromTextState(DWORD state);
inline LPCTSTR ElementFromTextState(DWORD state)
{
	ELLEX *pellex = pellexFromTextState(state);
	return pellex ? pellex->sz : 0;
}

#ifdef _DEBUG
int CheckWordTable(ReservedWord *arw, int cel, LPCTSTR szName = NULL);
int CheckWordTableIndex(ReservedWord *arw, int cel, int *ai, BOOL bCase = FALSE, LPCTSTR szName = NULL);
int MakeIndexHere(ReservedWord *arw, int cel, int *ab, BOOL bCase = FALSE, LPCTSTR szName = NULL);
#endif

int MakeIndex(ReservedWord *arw, int cel, int **pab, BOOL bCase = FALSE, LPCTSTR szName = NULL);

////////////////////////////////////////////////////////////////////////////

// test for SGML identifier character:
// alphanumeric or '-' or '.'
inline BOOL IsIdChar(TCHAR ch)
{
	return IsCharAlphaNumeric(ch) || ch == _T('-') || ch == _T('.') || ch == _T(':');
}

////////////////////////////////////////////////////////////////////////////
// Abstract Base Classes
//
class CTable
{
public:
	virtual ~CTable() {}
	virtual int Find(LPCTSTR pch, int cb) PURE;
};

class CTableSet
{
public:
	virtual ~CTableSet() {}
	virtual int FindElement(LPCTSTR pch, int cb) PURE;
	virtual int FindAttribute(LPCTSTR pch, int cb) PURE;
	virtual int FindEntity(LPCTSTR pch, int cb) PURE;
	const TCHAR* Name() const { return m_strName; }

protected:
	TCHAR m_strName[1024];

};

typedef CTable *PTABLE;
typedef CTableSet * PTABLESET;
typedef const CTableSet * PCTABLESET;

// static, built-in table
class CStaticTable : public CTable
{
public:
	CStaticTable(
		RWATT_T att,
		ReservedWord *prgrw, UINT cel, 
		int *prgi = NULL, 
		BOOL bCase = FALSE,
		LPCTSTR szName = NULL);
	virtual ~CStaticTable() {} // nothing to delete
	BOOL Find(LPCTSTR pch, int cb);
private:
	ReservedWord *m_prgrw; // reserved word table
	UINT m_cel;            // element count (size)
	int *m_prgi;           // index table
	BOOL m_bCase;          // case sensitive?
	RWATT_T m_att;         // attribute mask for table lookup
};

class CStaticTableSet : public CTableSet
{
public:
	CStaticTableSet(RWATT_T att, UINT nIdName);
	virtual ~CStaticTableSet() {}
	int FindElement(LPCTSTR pch, int cb);
	int FindAttribute(LPCTSTR pch, int cb);
	int FindEntity(LPCTSTR pch, int cb);
private:
	CStaticTable m_Elements;
	CStaticTable m_Attributes;
	CStaticTable m_Entities;
};

////////////////////////////////////////////////////////////////////////////
// CLStr
// A very simple length-and-buffer string representation
// with just enough functionality for our purpose.
class CLStr
{
public:
	CLStr() : m_cb(0), m_rgb(0) {}
	CLStr(const BYTE * rgb, DWORD cb) : m_rgb(rgb), m_cb(cb) {}
	BOOL Compare(const BYTE * rgb, DWORD cb, BOOL bCase)
	{
		int r;
		if (bCase)
			r = memcmp(rgb, m_rgb, __min(m_cb, cb));
		else
			r = _memicmp(rgb, m_rgb, __min(m_cb, cb));
		return (0 == r) ? (cb - m_cb) : r;
	}
	// data
	DWORD m_cb;
	const BYTE * m_rgb;
};
typedef CLStr * PLSTR;
typedef const CLStr * PCLSTR;
typedef const CLStr & RCLSTR;

extern CStaticTableSet * g_pTabDefault;
extern PTABLESET g_pTable;
extern HINT g_hintTable[];

#endif // __HLTABLE_H__
