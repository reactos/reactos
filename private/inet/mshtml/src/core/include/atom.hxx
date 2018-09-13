//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       atom.hxx
//
//  Contents:   Hashed atom-table support
//
//----------------------------------------------------------------------------

#ifndef I_ATOM_HXX_
#define I_ATOM_HXX_
#pragma INCMSG("--- Beg 'atom.hxx'")

class CImplAtom;
class CAtom;

MtExtern(CImplAtom)
MtExtern(CAtomPool)

//+---------------------------------------------------------------------------
//
//  Class:      CAtom
//
//  Synopsis:   CAtom is a "unique string" class which assigns a sequential
//              number to each distinct string.
//
//              A CAtom can be derefenced as a TCHAR * (just like CStr)
//
//              Atoms obtained from the same atom table can be compared
//              quickly because an atom table guarantees that
//
//              (atom1 == atom2) if and only if (!_tcscmp(atom1, atom2))
//
//----------------------------------------------------------------------------
class CAtom
{
public:
    operator TCHAR *() const { return _pch; }
    UINT            Number();
    TCHAR          *_pch;
    CImplAtom      *Holder();
};

extern const CAtom NULLATOM;

//+---------------------------------------------------------------------------
//
//  Class:      CImplAtom
//
//  Synopsis:   Helper for CAtom.
//              Each CAtom actually points to the _achAtom of a CImplAtom.
//
//----------------------------------------------------------------------------
class CImplAtom
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CImplAtom))
        
    void Init(UINT number, TCHAR *pch, UINT len)
    {
        _number = number;
        _tcsncpy(_achAtom, pch, len);
        _achAtom[len] = _T('\0');
    }

    CAtom           Atom();
    UINT            _number;
    TCHAR           _achAtom[];
};

//+---------------------------------------------------------------------------
//
//  Class:      CConstantAtomPool
//
//  Synopsis:   A fixed-size collection of constant atoms.
//
//              Designed to be initialized from static data.
//
//              The only instance should be the global s_constantAtomPool
//
//----------------------------------------------------------------------------
class CConstantAtomPool
{
public:
    CAtom Atom(TCHAR *pch, UINT len);
    CAtom Atom(TCHAR *pch) { return Atom(pch, _tcslen(pch)); }
    CAtom QuickAtom(TCHAR *pch, UINT len, DWORD hash);
    CAtom QuickAtom(TCHAR *pch, DWORD hash) { return QuickAtom(pch, hash, _tcslen(pch)); }
    CAtom AtomFromNumber(UINT number);
    int TrialHash();
    
    class CEntryRange
    {
    public:
        UINT    _numberFirst;
        UINT    _cAtoms;
        TCHAR **_ppchAtoms; // dense array of atoms indexed by number-_numberFirst
    };

    DWORD _cRanges;
    DWORD _sizeAtomHash;
    DWORD _strideMask;
    
    CEntryRange *_pRanges;
    TCHAR *_apchAtomHash[]; // sparse array of atoms indexed by hash value
};

extern CConstantAtomPool s_constantAtomPool;

//+---------------------------------------------------------------------------
//
//  Class:      CAtomPool
//
//  Synopsis:   A growable collection of atoms.
//
//              Begins with the contents of s_constantAtomPool and grows.
//
//              Any two atoms which have equal strings are guaranteed to
//              be identical (atom1 == atom2) if they come from the same
//              table.
//
//----------------------------------------------------------------------------
class CAtomPool
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CAtomPool))
    CAtomPool(UINT numberFirst);
    
    HRESULT GetAtom(CAtom *pAtom, TCHAR *pch, UINT len);
    HRESULT GetAtom(CAtom *pAtom, TCHAR *pch) { return GetAtom(pAtom, pch, _tcslen(pch)); }
    CAtom AtomFromNumber(UINT number);
    
private:

    class CEntry
    {
    public:
        CAtom _atom;
        DWORD _hash;
    };

    CAtom  *_pAtomHash;   // sparse array of atoms indexed by hash value
    DWORD   _sizeAtomHash;// size of pAtomHash array
    DWORD   _strideMask;  // for generating stride
    int     _iSize;       // index to next size
    CAtom   _atomOne[1];  // dummy first atom to avoid alloc-on-construction
    
    CEntry *_pEntries;    // dense array of entries indexed by number-numberFirst
    UINT    _numberFirst; // first number
    DWORD   _cEntries;    // number of atoms in table
    DWORD   _maxEntries;  // max number of atoms in table
};


//+----------------------------------------------------------------------------
//
// Inline function and method definitions
//
//-----------------------------------------------------------------------------

inline DWORD HashAtomString(TCHAR *pch, UINT len)
{
    DWORD z;

    z = 0;

    while (len--)
    {
        z += *(pch++);
        // BUGBUG: will this compile to a single instruction?
        z = (z << 19 ^ z >> (32-19));
    }

    return z;
}

inline CImplAtom *CAtom::Holder()
{
    return ((CImplAtom *)((BYTE*)_pch - offsetof(CImplAtom, _achAtom)));
}

inline UINT CAtom::Number()
{
    return Holder()->_number;
}

#define ATOM(name) ((s_ia##name)._achAtom)

inline CAtom CImplAtom::Atom()
{
    CAtom atom;
    atom._pch = _achAtom;
    return atom;
}

// Examples of initialization
//
//
// CImplAtom s_iaFRAMESET   = {UINT_FRAMESET, _T("frameset")};
// CImplAtom s_iaFORM       = {UINT_FORM,     _T("form")};
// CImplAtom s_iaSRC        = {UINT_SRC,      _T("src")};
// TCHAR *[] s_constantAtoms = {
//      ATOM(FRAMESET),
//      ATOM(FORM),
//      ATOM(SRC)
// };
// TCHAR *[] s_constantAtomHash = {
//      NULL,
//      NULL,
//      ATOM(FORM),
//      NULL,
//      ATOM(SRC),
//      ATOM(FRAMESET),
//      NULL
// };
//
// CConstantAtomPool s_constantAtomPool = { s_constantAtoms, s_constantAtomHash };

#pragma INCMSG("--- End 'atom.hxx'")
#else
#pragma INCMSG("*** Dup 'atom.hxx'")
#endif
