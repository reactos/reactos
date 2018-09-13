//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       assoc.cxx
//
//  Contents:   String-indexed bag classses (associative-array)
//
//              CPtrBag   (case-sensitive)
//              CPtrBagCi (optionally case-insensitive)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

MtDefine(CAssoc, PerProcess, "CAssoc")

#define ASSOC_HASH_MARKED ((CAssoc*)1)

//+---------------------------------------------------------------------------
//
//  Variable:   s_asizeAssoc
//
//              A list of primes for use as hashing modulii
//
//----------------------------------------------------------------------------
DWORD s_asizeAssoc[] = {/* 3,5,7,11,17,23, */ 37,59,89,139,227,359,577,929,
    1499,2423,3919,6337,10253,16573,26821,43391,70207,113591,183797,297377,
    481171,778541,1259701,2038217,3297913,5336129,8633983,13970093,22604069,
    36574151,59178199,95752333,154930511,250682837,405613333,656296153,
    1061909479,1718205583,2780115059};
    
// an alternate list (grows faster)
#if 0
DWORD s_asizeAssoc[] = {/*3,5,7,13,*/23,43,83,163,317,631,1259,2503,5003,9973,
    19937,39869,79699,159389,318751,637499,1274989,2549951,5099893,10199767,
    20399531,40799041,81598067,163196129,326392249,652784471,1305568919,
    2611137817};
#endif


//+---------------------------------------------------------------------------
//
//  Function:   HashString
//
//              Computes a 32-bit hash value for a unicode string
//
//              asm version supplied so that we take advantage of ROR
//
//----------------------------------------------------------------------------
#if defined(_M_IX86) && !defined(WIN16)

#pragma warning(disable:4035) // implicit return value left in eax

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash)
{
    _asm {
        mov         ecx, len            ;   // ecx = len
        mov         ebx, pch            ;   // ebx = pch
        mov         eax, hash           ;   // eax = hash
        xor         edx, edx            ;   // edx = 0
        cmp         ecx, 0              ;   // while (!len)
        je          loop_exit           ;   // {
    loop_top:
        mov         dx, word ptr [ebx]  ;   //     *pch
        ror         eax, 7              ;   //     hash = (hash >> 7) | (hash << (32-7))
        add         ebx, 2              ;   //     pch++
        add         eax, edx            ;   //     hash += *pch
        dec         ecx                 ;   //     len--
        jnz         loop_top            ;   // }
    loop_exit:
    }                                       // result in eax
}

#pragma warning(default:4035)

#else

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += *pch; // Case-sensitive hash
        pch++;
        len--;
    }

    return hash;
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   HashStringCi
//
//              Computes a 32-bit hash value for a unicode string which
//              is case-insensitive for ASCII characters.
//
//              asm version supplied so that we take advantage of ROR
//
//----------------------------------------------------------------------------
#if defined(_M_IX86) && !defined(WIN16)

#pragma warning(disable:4035) // implicit return value left in eax

DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash)
{
    _asm {
        mov         ecx, len            ;   // ecx = len
        mov         ebx, pch            ;   // ebx = pch
        mov         eax, hash           ;   // eax = hash
        xor         edx, edx            ;   // edx = 0
        cmp         ecx, 0              ;   // while (!hash)
        je          loop_exit           ;   // {
    loop_top:
        mov         dx, word ptr [ebx]  ;   //     *pch
        ror         eax, 7              ;   //     hash = (hash >> 7) | (hash << (32-7))
        add         ebx, 2              ;   //     pch++
        and         dx, ~(_T('a')-_T('A'))
        add         eax, edx            ;   //     hash += *pch
        dec         ecx                 ;   //     len--
        jnz         loop_top            ;   // }
    loop_exit:
    }                                       // result in eax
}

#pragma warning(default:4035)

#else
DWORD HashStringCi(const TCHAR *pch, DWORD len, DWORD hash)
{
    while (len)
    {
        hash = (hash >> 7) | (hash << (32-7));
        hash += (*pch & ~(_T('a')-_T('A'))); // Case-insensitive hash
        pch++;
        len--;
    }

    return hash;
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   _tcsnzequal
//
//              Tests equality of two strings, where the first is
//              specified by pch/cch, and the second is \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _tcsnzequal(const TCHAR *string1, DWORD cch, const TCHAR *string2)
{
    while (cch)
    {
        if (*string1 != *string2)
            return FALSE;
            
        string1 += 1;
        string2 += 1;
        cch --;
    }
    
    return (*string2) ? FALSE : TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   _7csnziequal
//
//              Tests 7-bit-case-insensitive equality of two strings, where
//              the first is specified by pch/cch, and the second is
//              \00-terminated.
//
//----------------------------------------------------------------------------
BOOL _7csnziequal(const TCHAR *string1, DWORD cch, const TCHAR *string2)
{
    while (cch)
    {
        if (*string1 != *string2)
        {
            if ((*string1 ^ *string2) != _T('a') - _T('A'))
                return FALSE;
                
            if (((unsigned)(*string2 - _T('A')) > _T('Z') - _T('A')) &&
                ((unsigned)(*string2 - _T('a')) > _T('z') - _T('a')))
                return FALSE;
        }
        string1 += 1;
        string2 += 1;
        cch --;
    }
    
    return (*string2) ? FALSE : TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::Init
//
//  Synopsis:   Initializes CAssocArray to _cHash = 1.
//              An initial size >0 is required to allow "hash % _mHash".
//  
//              Note: to avoid memory allocation here,
//              _pAssocHash and _pAssocs are init'ed to point to a dummy
//              member _assocOne.
//
//  Arguments:  [pch] -- String to match the assoc
//              [hash]-- the result of HashAssocString(pch)
//
//  Returns:    CAssoc (possibly NULL)
//
//----------------------------------------------------------------------------
void CAssocArray::Init()
{
    // init hashtable
    _pHashTable   = &_pAssocOne;
    _pAssocOne    = NULL;
    _cHash        = 0;
    _mHash        = 1;
    _sHash        = 0;
    _maxHash      = 0;
    _iSize        = 0;

    Assert(!_fStatic);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::~CAssocArray
//
//  Synopsis:   Deletes all the assocs stored in the table and frees
//              the hash table memory.
//  
//              Note: _pAssocHash is only freed if it does not point to _assocOne.
//
//  Arguments:  [pch] -- String to match the assoc
//              [hash]-- the result of HashAssocString(pch)
//
//  Returns:    CAssoc (possibly NULL)
//
//----------------------------------------------------------------------------
void CAssocArray::Deinit()
{
    CAssoc **ppn;
    int     c;

    Assert(!_fStatic);
    
    for (ppn = _pHashTable, c = _mHash; c; ppn++, c--)
    {
        delete *ppn;
    }

    if (_pHashTable != &_pAssocOne)
    {
        MemFree(_pHashTable);
        _pHashTable = &_pAssocOne;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::AddAssoc
//
//  Synopsis:   Adds a assoc (a string-number association) to the table.
//              Neither the string nor the number should previously appear.
//
//  Returns:    HRESULT (possibly E_OUTOFMEMORY)
//
//----------------------------------------------------------------------------
CAssoc *CAssocArray::AddAssoc(DWORD_PTR number, const TCHAR *pch, DWORD len, DWORD hash)
{
    CAssoc *passoc;
    HRESULT hr;

    Assert(!_fStatic);
    Assert(!AssocFromString(pch, len, hash));
    
    // Step 1: create the new assoc
    passoc = new ((len+1)*sizeof(TCHAR)) CAssoc;
    if (!passoc)
        return NULL;
        
    passoc->Init(number, pch, len, hash);

    // Step 2: verify that the tables have enough room
    if (_cHash >= _maxHash)
    {
        hr = THR(ExpandHash());
        if (hr)
            goto Error;
    }

    // Step 3: insert the new assoc in the hash array
    _pHashTable[EmptyHashIndex(hash)] = passoc;
    _cHash++;
    
    return passoc;

Error:
    delete passoc;

    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::EmptyHashIndex
//
//  Synopsis:   Finds first empty hash index for the given hash value.
//
//----------------------------------------------------------------------------
DWORD CAssocArray::EmptyHashIndex(DWORD hash)
{
    DWORD i = hash % _mHash;
    DWORD s;
    
    if (!_pHashTable[i])
        return i;

    s = (hash & _sHash) + 1;
    
    do
    {
        if (i < s)
            i += _mHash;
        i -=s ;
    } while (_pHashTable[i]);

    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::HashIndexFromString
//
//  Synopsis:   Finds the place in the hash table in which the assoc with
//              the specified string lives. If no assoc with the string is
//              present, returns the place in the hash table where the
//              assoc would be.
//
//----------------------------------------------------------------------------
CAssoc *CAssocArray::AssocFromString(const TCHAR *pch, DWORD cch, DWORD hash)
{
    DWORD    i;
    DWORD    s;
    CAssoc  *passoc;
    
    i = hash % _mHash;
    passoc = _pHashTable[i];
    
    if (!passoc)
        return NULL;
        
    if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
        return passoc;
        
    s = (hash & _sHash) + 1;
    
    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;
    
        if (passoc->Hash() == hash && _tcsnzequal(pch, cch, passoc->String()))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::HashIndexFromStringCi
//
//  Synopsis:   Just like HashIndexFromString, but case-insensitive.
//
//----------------------------------------------------------------------------
CAssoc *CAssocArray::AssocFromStringCi(const TCHAR *pch, DWORD cch, DWORD hash)
{
    DWORD    i;
    DWORD    s;
    CAssoc  *passoc;
    
    i = hash % _mHash;
    passoc = _pHashTable[i];
    
    if (!passoc)
        return NULL;
        
    if (passoc->Hash() == hash && _7csnziequal(pch, cch, passoc->String()))
        return passoc;
        
    s = (hash & _sHash) + 1;
    
    for (;;)
    {
        if (i < s)
            i += _mHash;

        i -= s;

        passoc = _pHashTable[i];

        if (!passoc)
            return NULL;
    
        if (passoc->Hash() == hash && _7csnziequal(pch, cch, passoc->String()))
            return passoc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAssocArray::ExpandHash
//
//  Synopsis:   Expands the hash table, preserving order information
//              (so that colliding hash entries can be taken out in the
//              same order they were put in).
//
//----------------------------------------------------------------------------
HRESULT CAssocArray::ExpandHash()
{
    CAssoc **ppn;
    CAssoc **pHashTableOld;
    CAssoc **ppnMax;
    DWORD mHashOld;
    DWORD sHashOld;
    DWORD hash;
    DWORD i;
    DWORD s;
    
    Assert(_iSize < ARRAY_SIZE(s_asizeAssoc));
    Assert(!_fStatic);

    // allocate memory for expanded hash table
    ppn = (CAssoc**)MemAllocClear(Mt(CAssoc), sizeof(CAssoc*) * s_asizeAssoc[_iSize]);
    if (!ppn)
    {
        return E_OUTOFMEMORY;
    }

    // set new sizes
    pHashTableOld = _pHashTable;
    mHashOld = _mHash;
    sHashOld = _sHash;
    _mHash = s_asizeAssoc[_iSize];
    _maxHash = _mHash/2;
    for (_sHash = 1; _sHash <= _maxHash; _sHash = _sHash << 1 | 1);
    Assert(_sHash < _mHash);
    _iSize++;
    _pHashTable = ppn;
    
    // rehash - do per hash value to preserve the "order"
    if (_cHash)
    {
        Assert(_cHash < mHashOld);
        
        for (ppn = pHashTableOld, ppnMax = ppn+mHashOld; ppn < ppnMax; ppn++)
        {
            if (*ppn && *ppn != ASSOC_HASH_MARKED)
            {
                hash = (*ppn)->Hash();
                i = hash % mHashOld;
                s = (hash & sHashOld) + 1;
                
                // inner loop needed to preserve "hash order" for ci aliases
                while (pHashTableOld[i])
                {
                    if (pHashTableOld[i] != ASSOC_HASH_MARKED &&
                        pHashTableOld[i]->Hash() == hash)
                    {
                        _pHashTable[EmptyHashIndex(hash)] = pHashTableOld[i];
                        pHashTableOld[i] = ASSOC_HASH_MARKED;
                    }
                    
                    if (i < s)
                        i += mHashOld;

                    i -= s;
                }
            }
        }
    }

    // free old memory
    if (pHashTableOld != &_pAssocOne)
        MemFree(pHashTableOld);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::SetImpl
//
//  Synopsis:   Sets an association of the string to the void*, creating
//              a new association if needed.
//
//              TODO: remove associations if e==NULL.
//
//----------------------------------------------------------------------------
HRESULT CImplPtrBag::SetImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e)
{
    CAssoc *passoc;
    
    Assert(!_fStatic);

    passoc = AssocFromString(pch, cch, hash);
    
    if (passoc)
        passoc->_number = (DWORD_PTR)e;
    else
    {
        passoc = AddAssoc((DWORD_PTR)e, pch, cch, hash);
        if (!passoc)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::GetImpl
//
//  Synopsis:   Returns the void* associated with the given string, or
//              NULL if none.
//
//----------------------------------------------------------------------------
void *CImplPtrBag::GetImpl(const TCHAR *pch, DWORD cch, DWORD hash)
{
    CAssoc *passoc;
    
    passoc = AssocFromString(pch, cch, hash);
    
    if (passoc)
        return (void*)passoc->_number;

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::SetCiImpl
//
//  Synopsis:   Sets an association of the string to the specified void*.
//              If there is no association which satisfies a case-insensitive
//              match, a new association is created.
//
//----------------------------------------------------------------------------
HRESULT CImplPtrBag::SetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash, void *e)
{
    CAssoc *passoc;
    
    Assert(!_fStatic);
    
    passoc = AssocFromStringCi(pch, cch, hash);
    
    if (passoc)
        passoc->_number = (DWORD_PTR)e;
    else
    {
        passoc = AddAssoc((DWORD_PTR)e, pch, cch, hash);
        if (!passoc)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImplPtrBag::GetCiImpl
//
//  Synopsis:   Returns the void* associated with the given string or
//              a case-insensitive match, or NULL if none.
//
//----------------------------------------------------------------------------
void *CImplPtrBag::GetCiImpl(const TCHAR *pch, DWORD cch, DWORD hash)
{
    CAssoc *passoc;
    
    passoc = AssocFromStringCi(pch, cch, hash);
    
    if (passoc)
        return (void*)passoc->_number;

    return NULL;
}



