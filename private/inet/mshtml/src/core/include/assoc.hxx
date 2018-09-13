//+------------------------------------------------------------------------
//
//  File:       assoc.hxx
//
//  Contents:   Generic dynamic associative array class
//
//  Classes:    CAssocArray, CPtrBag
//
//-------------------------------------------------------------------------

#ifndef I_ASSOC_HXX_
#define I_ASSOC_HXX_
#pragma INCMSG("--- Beg 'assoc.hxx'")

MtExtern(CAssoc);

// Hashing function decls

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash);
DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash);


//+---------------------------------------------------------------------------
//
//  Class:      CAssoc
//
//  Purpose:    A single association in an associative array mapping
//              strings -> DWORD_PTRs.
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssoc
{
public:
    
    // DECLARE_MEMCLEAR_NEW_DELETE not used due to ascparse including this header
    // file without first including cdutil.hxx

    void * operator new(size_t cb) { return(MemAllocClear(Mt(CAssoc), cb)); }
    void * operator new(size_t cb, size_t cbExtra) { return(MemAllocClear(Mt(CAssoc), cb + cbExtra)); }
    void   operator delete(void * pv) { MemFree(pv); }
    
    void Init(DWORD_PTR number, const TCHAR *pch, int length, DWORD hash)
    {
        _number = number;
        _hash = hash;
        memcpy(_ach, pch, (length+1) * sizeof(TCHAR));
    }

    TCHAR *   String()   { return _ach; }
    DWORD_PTR Number()   { return _number; }
    DWORD     Hash()     { return _hash; }
    
    DWORD_PTR _number;
    DWORD     _hash;
    TCHAR     _ach[UNSIZED_ARRAY];
};

//+---------------------------------------------------------------------------
//
//  Class:      CAssocArray
//
//  Purpose:    A hash table implementation mapping strings -> DWORDs
//
//              The class is designed to be an aggregate so that it is
//              statically initializable. Therefore, it has no base
//              class, no constructor/destructors, and no private members
//              or methods.
//
//----------------------------------------------------------------------------
class CAssocArray
{
public:

    void Init();
    void Deinit();
    
    CAssoc *AssocFromString(const TCHAR *pch, DWORD len, DWORD hash);
    CAssoc *AssocFromStringCi(const TCHAR *pch, DWORD len, DWORD hash);
    CAssoc *AddAssoc(DWORD_PTR number, const TCHAR *pch, DWORD len, DWORD hash);

    CAssoc **_pHashTable;            // Assocs hashed by string
    DWORD    _cHash;                 // entries in the hash table
    DWORD    _mHash;                 // hash table modulus
    DWORD    _sHash;                 // hash table stride mask
    DWORD    _maxHash;               // maximum entries allowed
    DWORD    _iSize;                 // prime array index
    
    union {
        BOOL    _fStatic;            // TRUE for a static Assoc table
        CAssoc *_pAssocOne;          // NULL for a dynamic Assoc table
    };

    DWORD EmptyHashIndex(DWORD hash);
    HRESULT ExpandHash();
};

//+---------------------------------------------------------------------------
//
//  Class:      CImplPtrBag
//
//  Purpose:    Implements an associative array of strings->pointers.
//
//              Implemented as a CAssocArray. Can be intialized to point to
//              a static associative array, or can be dynamic.
//
//----------------------------------------------------------------------------
class CImplPtrBag : protected CAssocArray
{
public:
    CImplPtrBag()
        { Init(); }
    CImplPtrBag(const CAssocArray *pTable) // copy static table
        { Assert(pTable->_fStatic); *(CAssocArray*)this = *pTable; }
        
    ~CImplPtrBag()
        { if (!_fStatic) { Deinit(); } }
    
    HRESULT SetImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e);
    void *GetImpl(const TCHAR *pch, DWORD cch, DWORD hash);

    HRESULT SetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e);
    void *GetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash);
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBag
//
//  Purpose:    A case-sensitive ptrbag.
//
//              This template class declares a concrete derived class
//              of CImplPtrBag.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CPtrBag : protected CImplPtrBag
{
public:

    CPtrBag() : CImplPtrBag()
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
    CPtrBag(const CAssocArray *pTable) : CImplPtrBag(pTable)
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
        
    ~CPtrBag() {}

    CPtrBag& operator=(const CPtrBag &); // no copying

    HRESULT     Set(const TCHAR *pch, ELEM e)
                    { return Set(pch, _tcslen(pch), e); }
    HRESULT     Set(const TCHAR *pch, DWORD cch, ELEM e)
                    { return Set(pch, cch, HashString(pch, cch, 0), e); }
    HRESULT     Set(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        Get(const TCHAR *pch)
                    { return Get(pch, _tcslen(pch)); }
    ELEM        Get(const TCHAR *pch, DWORD cch)
                    { return Get(pch, cch, HashString(pch, cch, 0)); }
    ELEM        Get(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetImpl(pch, cch, hash); }
};

//+---------------------------------------------------------------------------
//
//  Class:      CPtrBagCi
//
//  Purpose:    A case-insensitive ptrbag.
//
//              Supplies case-insensitive GetCi/SetCi methods in addition to
//              the  case-sensitive GetCs/SetCs methods.
//
//----------------------------------------------------------------------------

template <class ELEM>
class CPtrBagCi : protected CImplPtrBag
{
public:

    CPtrBagCi() : CImplPtrBag()
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
    CPtrBagCi(const CAssocArray *pTable) : CImplPtrBag(pTable)
        { Assert(sizeof(ELEM) <= sizeof(void*)); }
        
    ~CPtrBagCi() {}

    CPtrBagCi& operator=(const CPtrBagCi &); // no copying

    HRESULT     SetCs(const TCHAR *pch, ELEM e)
                    { return SetCs(pch, _tcslen(pch), e); }
    HRESULT     SetCs(const TCHAR *pch, DWORD cch, ELEM e)
                    { return SetCs(pch, cch, HashStringCi(pch, cch, 0), e); }
    HRESULT     SetCs(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        GetCs(const TCHAR *pch)
                    { return GetCs(pch, _tcslen(pch)); }
    ELEM        GetCs(const TCHAR *pch, DWORD cch)
                    { return GetCs(pch, cch, HashStringCi(pch, cch, 0)); }
    ELEM        GetCs(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetImpl(pch, cch, hash); }

    HRESULT     SetCi(const TCHAR *pch, ELEM e)
                    { return SetCi(pch, _tcslen(pch), e); }
    HRESULT     SetCi(const TCHAR *pch, DWORD cch, ELEM e)
                    { return SetCi(pch, cch, HashStringCi(pch, cch, 0), e); }
    HRESULT     SetCi(const TCHAR *pch, DWORD cch, DWORD hash, ELEM e)
                    { return CImplPtrBag::SetCiImpl(pch, cch, hash, (void*)(DWORD_PTR)e); }
    ELEM        GetCi(const TCHAR *pch)
                    { return GetCi(pch, _tcslen(pch)); }
    ELEM        GetCi(const TCHAR *pch, DWORD cch)
                    { return GetCi(pch, cch, HashStringCi(pch, cch, 0)); }
    ELEM        GetCi(const TCHAR *pch, DWORD cch, DWORD hash)
                    { return (ELEM)(DWORD_PTR)CImplPtrBag::GetCiImpl(pch, cch, hash); }

};

#pragma INCMSG("--- End 'assoc.hxx'")
#else
#pragma INCMSG("*** Dup 'assoc.hxx'")
#endif
