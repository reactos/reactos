//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       atom.cxx
//
//  Contents:   Atom-table support
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ATOM_HXX_
#define X_ATOM_HXX_
#include "atom.hxx"
#endif

const CAtom NULLATOM = {NULL};

// List of primes for use as hashing modulos and table sizes.
// When the number of entries grows to s(n), the hashtable is grown to s(n+1)
DWORD s_asizeAtom[] = {/* 3,5,7,11,17,23, */ 37,59,89,139,227,359,577,929,
    1499,2423,3919,6337,10253,16573,26821,43391,70207,113591,183797,297377,
    481171,778541,1259701,2038217,3297913,5336129,8633983,13970093,22604069,
    36574151,59178199,95752333,154930511,250682837,405613333,656296153,
    1061909479,1718205583,2780115059};
    
// an alternate list (grows faster)
#if 0
DWORD s_asizeAtom[] = {/*3,5,7,13,*/23,43,83,163,317,631,1259,2503,5003,9973,
    19937,39869,79699,159389,318751,637499,1274989,2549951,5099893,10199767,
    20399531,40799041,81598067,163196129,326392249,652784471,1305568919,
    2611137817};
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CConstantAtomPool::Atom
//
//  Synopsis:   Returns an atom from the constant atom pool,
//              or the NULL CAtom if none match.
//
//  Arguments:  [pch] -- String to match the atom
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
CAtom CConstantAtomPool::Atom(TCHAR *pch, UINT len)
{
    return QuickAtom(pch, HashAtomString(pch, len));
}

//+---------------------------------------------------------------------------
//
//  Member:     CConstantAtomPool::QuickAtom
//
//  Synopsis:   A version of Atom which can be used if the hash value is
//              already known.
//
//  Arguments:  [pch] -- String to match the atom
//              [hash]-- the result of HashAtomString(pch)
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
inline CAtom CConstantAtomPool::QuickAtom(TCHAR *pch, UINT len, DWORD hash)
{
    CAtom atom;
    TCHAR **ppch = _apchAtomHash + (hash % _sizeAtomHash);

    // Note: CONSTANT_ATOM_POOL_SIZE is chosen so that there are no collisions
    
    for (;;)
    {
        atom._pch = *ppch;
        if (!atom || !_tcsncmp(atom, -1, pch, len))
            return atom;
        ppch -= (hash & _strideMask) + 1;
        if (ppch <= _apchAtomHash)
            ppch += _sizeAtomHash;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CConstantAtomPool::AtomFromNumber
//
//  Synopsis:   Looks up an atom based on number.
//
//  Arguments:  [number] -- The number to lookup
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
CAtom CConstantAtomPool::AtomFromNumber(UINT number)
{
    CEntryRange *pRange;
    CAtom atom;
    UINT i;
    UINT j;

    for (i = _cRanges, pRange = _pRanges; i; i--, pRange++)
    {
        j = number - pRange->_numberFirst;
        
        if (j < pRange->_cAtoms) // uses UINT wraparound
        {
            atom._pch = pRange->_ppchAtoms[j];
            return atom;
        }
    }

    return NULLATOM;
}

#if 0

//+---------------------------------------------------------------------------
//
//  Member:     CAtomPool::CAtomPool
//
//  Synopsis:   Initializes CAtomPool to _sizeAtomHash = _maxAtoms = 1.
//              An initial size >0 is required to allow "hash % _sizeAtomHash".
//  
//              Note: to avoid memory allocation in the constructor,
//              _pAtomHash and _pAtoms are init'ed to point to a dummy
//              member _atomOne.
//
//  Arguments:  [pch] -- String to match the atom
//              [hash]-- the result of HashAtomString(pch)
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
CAtomPool::CAtomPool(UINT numberFirst)
{
    _pAtomHash    = _atomOne;
    _sizeAtomHash = 1;
    _strideMask   = 0;
    _iSize        = 0;
    *_atomOne     = NULLATOM;
    
    _pEntries     = NULL;
    _numberFirst  = numberFirst;
    _cEntries     = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAtomPool::~CAtomPool
//
//  Synopsis:   Deletes all the atoms stored in the table and frees
//              the hash table memory.
//  
//              Note: _pAtomHash is only freed if it does not point to _atomOne.
//
//  Arguments:  [pch] -- String to match the atom
//              [hash]-- the result of HashAtomString(pch)
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
CAtomPool::~CAtomPool()
{
    CEntry *pe;
    int     i;
    
    for (pe = _pEntries, i = _cEntries; i; pe++, i--)
    {
        delete pe->_atom.Holder();
    }

    if (_pEntries)
        MemFree(_pEntries);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAtomPool::GetAtom
//
//  Synopsis:   Finds an atom.
//
//  Arguments:  [patom] -- Return value (never NULL if S_OK)
//              [pch]   -- String to match the atom
//
//  Returns:    HRESULT (possibly E_OUTOFMEMORY)
//
//----------------------------------------------------------------------------
HRESULT CAtomPool::GetAtom(CAtom *patom, TCHAR *pch, UINT len)
{
    CAtom       *pa;
    CAtom        a;
    CImplAtom   *pia;
    CEntry      *pe;
    DWORD        hash;
    DWORD        stride;
    HRESULT      hr;
    
    // Step 1: Find the atom
    hash = HashAtomString(pch, len);
    
    a = s_constantAtomPool.QuickAtom(pch, len, hash);
    if (a)
    {
        *patom = a;
        return S_OK;
    }

    pa = _pAtomHash + (hash % _sizeAtomHash);
    stride = (hash & _strideMask) + 1;
    while ((TCHAR*)*pa)
    {
        if (!_tcsncmp(*pa, -1, pch, len))
        {
            *patom = *pa;
            return S_OK;
        }

        if ((pa -= stride) < _pAtomHash)
            pa += _sizeAtomHash;
    }

    // Step 2: expand hash if needed
    if (_cEntries == _maxEntries)
    {
        DWORD i;
        DWORD newHash;
        DWORD newMax;
        
        // allocate new memory for dense and sparse arrays
        newHash = s_asizeAtom[_iSize];
        newMax  = s_asizeAtom[_iSize+1];
        hr = THR(MemRealloc((void**)&_pEntries, sizeof(CEntry) * newMax + sizeof(CAtom) * newHash));
        if (hr)
            RRETURN(hr);

        // set new sizes
        _maxEntries = newMax;
        _pAtomHash = (CAtom *)(_pEntries + newMax);
        _sizeAtomHash = newHash;
        _iSize++;

        // set new stridemask
        _strideMask = 0;
        for (i = _sizeAtomHash >> 1, _strideMask = 0; i; i >>= 1)
            _strideMask = (_strideMask << 1) | 1;
        
        // rehash
        memset(_pAtomHash, 0, sizeof(CAtom) * _sizeAtomHash);
        for (pe = _pEntries, i = _cEntries; i; i--, pe++)
        {
            pa = _pAtomHash + pe->_hash % _sizeAtomHash;
            stride = (pe->_hash & _strideMask) + 1;
            while (*pa)
            {
                if ((pa -= stride) < _pAtomHash)
                    pa += _sizeAtomHash;
            }
            *pa = pe->_atom;
        }

        // locate spot for new atom
        pa = _pAtomHash + hash % _sizeAtomHash;
        stride = (hash & _strideMask) + 1;
        while (*pa)
        {
            if ((pa -= stride) < _pAtomHash)
                pa += _sizeAtomHash;
        }
    }

    // Step 3: create new atom and put it in the tables
    pia = new((len + 1) * sizeof(TCHAR)) CImplAtom();
    if (!pia)
        RRETURN(E_OUTOFMEMORY);
    pia->Init(_numberFirst+_cEntries, pch, len);
    pe = _pEntries + (_cEntries++);
    pe->_hash = hash;
    *patom = *pa = pe->_atom = pia->Atom();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAtomPool::AtomFromNumber
//
//  Synopsis:   Looks up an atom based on number.
//
//  Arguments:  [number] -- The number to lookup
//
//  Returns:    CAtom (possibly NULL)
//
//----------------------------------------------------------------------------
CAtom CAtomPool::AtomFromNumber(UINT number)
{
    if (number < _numberFirst)
    {
        return s_constantAtomPool.AtomFromNumber(number);
    }

    DWORD i = (DWORD)number - (DWORD)_numberFirst;
    
    if (i < _cEntries)
    {
        return _pEntries[i]._atom;
    }

    return NULLATOM;
}

#endif

int CConstantAtomPool::TrialHash()
{
    static fDone = FALSE;
    CEntryRange *pRange;
    TCHAR buf[20];
    TCHAR **ppchAtom;
    TCHAR **ppchAtomHash = NULL;
    TCHAR **ppchProbe;
    DWORD hashSizeTrial;
    DWORD hash;
    DWORD hashXor;
    DWORD hashXorReverse = 0;
    UINT i;
    UINT j;
    int rot = 0;
    HRESULT hr;

    if (fDone)
        return 1;

    fDone = TRUE;
    
    hashSizeTrial = 0;
    for (i = _cRanges, pRange = _pRanges; i; i--, pRange++)
        hashSizeTrial += pRange->_cAtoms-1;

    hashSizeTrial += hashSizeTrial/3;
    hashSizeTrial |= 1;

Retry:
    if (!rot)
    {
        if (!hashXorReverse)
        {
            hr = THR(MemRealloc((void**)&ppchAtomHash, sizeof(TCHAR*)*hashSizeTrial));
            if (hr)
                RRETURN(hr);
            _itot(hashSizeTrial, buf, 10);
            OutputDebugStringA("Trying ");
            OutputDebugStringW(buf);
            OutputDebugStringA("\n");
        }
        
    }

    memset(ppchAtomHash, 0, sizeof(TCHAR*)*hashSizeTrial);
    
    hashXor = 0;
    for (i=0; i<32; i++)
        hashXor |= ((hashXorReverse >> i) & 1) << (31-i);
    
    _strideMask = 0;
    for (i = hashSizeTrial >> 1, _strideMask = 0; i; i >>= 1)
        _strideMask = (_strideMask << 1) | 1;
            
    for (i = _cRanges, pRange = _pRanges; i; i--, pRange++)
    {
        for (ppchAtom = pRange->_ppchAtoms, j = pRange->_cAtoms; j; j--, ppchAtom++)
        {
            hash = HashAtomString(*ppchAtom, _tcslen(*ppchAtom));
            hash = (hash << rot ^ hash >> (32-rot)) ^ hashXor;
            ppchProbe = ppchAtomHash + (hash % hashSizeTrial);
            if (*ppchProbe)
            {
                ppchProbe -= (hash & _strideMask) + 1;
                if (ppchProbe < ppchAtomHash)
                    ppchProbe += hashSizeTrial;
                if (*ppchProbe)
                {   
                    rot++;
                    if (rot >= 32)
                    {
                        rot = 0;
                        hashXorReverse++;
                        if (hashXorReverse >= 65536)
                        {
                            hashXorReverse = 0;
                            hashSizeTrial += 2;
                        }
                    }        
                    goto Retry;
                }
            }
            *ppchProbe = *ppchAtom;
        }
    }
    
    _itot(hashSizeTrial, buf, 10);
    OutputDebugStringA("Succeeded on ");
    OutputDebugStringW(buf);
    OutputDebugStringA(" with hash xor ");
    _itot(hashXor, buf, 16);
    OutputDebugStringW(buf);
    OutputDebugStringA(" rot ");
    _itot(rot, buf, 16);
    OutputDebugStringW(buf);
    OutputDebugStringA(".\n");
    
    for (i = hashSizeTrial, ppchAtom = ppchAtomHash; i; i--, ppchAtom++)
    {
        if (!*ppchAtom)
        {
            OutputDebugStringA("NULL,\n");
        }
        else
        {
            OutputDebugStringA("HASH_ENTRY(");
            OutputDebugStringW(*ppchAtom);
            OutputDebugStringA("),\n");
        }
    }

    MemFree(ppchAtomHash);

    return 0;
}


#if 0
    ENTITIZE(  34, "quot"    ),      // "
    ENTITIZE(  38, "amp"     ),      // & - ampersand
    ENTITIZE(  60, "lt"      ),      // < less than
    ENTITIZE(  62, "gt"      ),      // > greater than
    ENTITIZE2(8482, "Trade", TRUE ),
#endif

// CImplAtom s_iaFRAMESET   = {UINT_FRAMESET, _T("frameset")};

CImplAtom s_test = { 4, _T("foo") };

#define ENTITIZE(number, string) CImplAtom s_ia_Ent##string = { number, _T(#string) }
#define ENTATOM(string) ATOM(_Ent##string)

ENTITIZE(160, nbsp   );     // Non breaking space
ENTITIZE(161, iexcl  );     //
ENTITIZE(162, cent   );     // cent
ENTITIZE(163, pound  );     // pound
ENTITIZE(164, curren );     // currency
ENTITIZE(165, yen    );     // yen
ENTITIZE(166, brvbar );     // vertical bar
ENTITIZE(167, sect   );     // section
ENTITIZE(168, uml    );     //
ENTITIZE(169, copy   );     // Copyright
ENTITIZE(170, ordf   );     //
ENTITIZE(171, laquo  );     //
ENTITIZE(172, not    );     //
ENTITIZE(173, shy    );     //
ENTITIZE(174, reg    );     // Registered TradeMark
ENTITIZE(175, macr   );     //
ENTITIZE(176, deg    );     //
ENTITIZE(177, plusmn );     //
ENTITIZE(178, sup2   );     //
ENTITIZE(179, sup3   );     //
ENTITIZE(180, acute  );     //
ENTITIZE(181, micro  );     //
ENTITIZE(182, para   );     //
ENTITIZE(183, middot );     //
ENTITIZE(184, cedil  );     //
ENTITIZE(185, sup1   );     //
ENTITIZE(186, ordm   );     //
ENTITIZE(187, raquo  );     //
ENTITIZE(188, frac14 );     // 1/4
ENTITIZE(189, frac12 );     // 1/2
ENTITIZE(190, frac34 );     // 3/4
ENTITIZE(191, iquest );     // Inverse question mark
ENTITIZE(192, Agrave );     // Capital A, grave accent
ENTITIZE(193, Aacute );     // Capital A, acute accent
ENTITIZE(194, Acirc  );     // Capital A, circumflex accent
ENTITIZE(195, Atilde );     // Capital A, tilde
ENTITIZE(196, Auml   );     // Capital A, dieresis or umlaut mark
ENTITIZE(197, Aring  );     // Capital A, ring
ENTITIZE(198, AElig  );     // Capital AE dipthong (ligature)
ENTITIZE(199, Ccedil );     // Capital C, cedilla
ENTITIZE(200, Egrave );     // Capital E, grave accent
ENTITIZE(201, Eacute );     // Capital E, acute accent
ENTITIZE(202, Ecirc  );     // Capital E, circumflex accent
ENTITIZE(203, Euml   );     // Capital E, dieresis or umlaut mark
ENTITIZE(204, Igrave );     // Capital I, grave accent
ENTITIZE(205, Iacute );     // Capital I, acute accent
ENTITIZE(206, Icirc  );     // Capital I, circumflex accent
ENTITIZE(207, Iuml   );     // Capital I, dieresis or umlaut mark
ENTITIZE(208, ETH    );     // Capital Eth, Icelandic
ENTITIZE(209, Ntilde );     // Capital N, tilde
ENTITIZE(210, Ograve );     // Capital O, grave accent
ENTITIZE(211, Oacute );     // Capital O, acute accent
ENTITIZE(212, Ocirc  );     // Capital O, circumflex accent
ENTITIZE(213, Otilde );     // Capital O, tilde
ENTITIZE(214, Ouml   );     // Capital O, dieresis or umlaut mark
ENTITIZE(215, times  );     // multiply or times
ENTITIZE(216, Oslash );     // Capital O, slash
ENTITIZE(217, Ugrave );     // Capital U, grave accent
ENTITIZE(218, Uacute );     // Capital U, acute accent
ENTITIZE(219, Ucirc  );     // Capital U, circumflex accent
ENTITIZE(220, Uuml   );     // Capital U, dieresis or umlaut mark;
ENTITIZE(221, Yacute );     // Capital Y, acute accent
ENTITIZE(222, THORN  );     // Capital THORN, Icelandic
ENTITIZE(223, szlig  );     // Small sharp s, German (sz ligature)
ENTITIZE(224, agrave );     // Small a, grave accent
ENTITIZE(225, aacute );     // Small a, acute accent
ENTITIZE(226, acirc  );     // Small a, circumflex accent
ENTITIZE(227, atilde );     // Small a, tilde
ENTITIZE(228, auml   );     // Small a, dieresis or umlaut mark
ENTITIZE(229, aring  );     // Small a, ring
ENTITIZE(230, aelig  );     // Small ae dipthong (ligature)
ENTITIZE(231, ccedil );     // Small c, cedilla
ENTITIZE(232, egrave );     // Small e, grave accent
ENTITIZE(233, eacute );     // Small e, acute accent
ENTITIZE(234, ecirc  );     // Small e, circumflex accent
ENTITIZE(235, euml   );     // Small e, dieresis or umlaut mark
ENTITIZE(236, igrave );     // Small i, grave accent
ENTITIZE(237, iacute );     // Small i, acute accent
ENTITIZE(238, icirc  );     // Small i, circumflex accent
ENTITIZE(239, iuml   );     // Small i, dieresis or umlaut mark
ENTITIZE(240, eth    );     // Small eth, Icelandic
ENTITIZE(241, ntilde );     // Small n, tilde
ENTITIZE(242, ograve );     // Small o, grave accent
ENTITIZE(243, oacute );     // Small o, acute accent
ENTITIZE(244, ocirc  );     // Small o, circumflex accent
ENTITIZE(245, otilde );     // Small o, tilde
ENTITIZE(246, ouml   );     // Small o, dieresis or umlaut mark
ENTITIZE(247, divide );     // divide
ENTITIZE(248, oslash );     // Small o, slash
ENTITIZE(249, ugrave );     // Small u, grave accent
ENTITIZE(250, uacute );     // Small u, acute accent
ENTITIZE(251, ucirc  );     // Small u, circumflex accent
ENTITIZE(252, uuml   );     // Small u, dieresis or umlaut mark
ENTITIZE(253, yacute );     // Small y, acute accent
ENTITIZE(254, thorn  );     // Small thorn, Icelandic
ENTITIZE(255, yuml   );     // Small y, dieresis or umlaut mark

TCHAR *s_capEntities160To255[] = {
ENTATOM( nbsp   ),     // Non breaking space
ENTATOM( iexcl  ),     //
ENTATOM( cent   ),     // cent
ENTATOM( pound  ),     // pound
ENTATOM( curren ),     // currency
ENTATOM( yen    ),     // yen
ENTATOM( brvbar ),     // vertical bar
ENTATOM( sect   ),     // section
ENTATOM( uml    ),     //
ENTATOM( copy   ),     // Copyright
ENTATOM( ordf   ),     //
ENTATOM( laquo  ),     //
ENTATOM( not    ),     //
ENTATOM( shy    ),     //
ENTATOM( reg    ),     // Registered TradeMark
ENTATOM( macr   ),     //
ENTATOM( deg    ),     //
ENTATOM( plusmn ),     //
ENTATOM( sup2   ),     //
ENTATOM( sup3   ),     //
ENTATOM( acute  ),     //
ENTATOM( micro  ),     //
ENTATOM( para   ),     //
ENTATOM( middot ),     //
ENTATOM( cedil  ),     //
ENTATOM( sup1   ),     //
ENTATOM( ordm   ),     //
ENTATOM( raquo  ),     //
ENTATOM( frac14 ),     // 1/4
ENTATOM( frac12 ),     // 1/2
ENTATOM( frac34 ),     // 3/4
ENTATOM( iquest ),     // Inverse question mark
ENTATOM( Agrave ),     // Capital A, grave accent
ENTATOM( Aacute ),     // Capital A, acute accent
ENTATOM( Acirc  ),     // Capital A, circumflex accent
ENTATOM( Atilde ),     // Capital A, tilde
ENTATOM( Auml   ),     // Capital A, dieresis or umlaut mark
ENTATOM( Aring  ),     // Capital A, ring
ENTATOM( AElig  ),     // Capital AE dipthong (ligature)
ENTATOM( Ccedil ),     // Capital C, cedilla
ENTATOM( Egrave ),     // Capital E, grave accent
ENTATOM( Eacute ),     // Capital E, acute accent
ENTATOM( Ecirc  ),     // Capital E, circumflex accent
ENTATOM( Euml   ),     // Capital E, dieresis or umlaut mark
ENTATOM( Igrave ),     // Capital I, grave accent
ENTATOM( Iacute ),     // Capital I, acute accent
ENTATOM( Icirc  ),     // Capital I, circumflex accent
ENTATOM( Iuml   ),     // Capital I, dieresis or umlaut mark
ENTATOM( ETH    ),     // Capital Eth, Icelandic
ENTATOM( Ntilde ),     // Capital N, tilde
ENTATOM( Ograve ),     // Capital O, grave accent
ENTATOM( Oacute ),     // Capital O, acute accent
ENTATOM( Ocirc  ),     // Capital O, circumflex accent
ENTATOM( Otilde ),     // Capital O, tilde
ENTATOM( Ouml   ),     // Capital O, dieresis or umlaut mark
ENTATOM( times  ),    // multiply or times
ENTATOM( Oslash ),     // Capital O, slash
ENTATOM( Ugrave ),     // Capital U, grave accent
ENTATOM( Uacute ),     // Capital U, acute accent
ENTATOM( Ucirc  ),     // Capital U, circumflex accent
ENTATOM( Uuml   ),     // Capital U, dieresis or umlaut mark,
ENTATOM( Yacute ),     // Capital Y, acute accent
ENTATOM( THORN  ),     // Capital THORN, Icelandic
ENTATOM( szlig  ),     // Small sharp s, German (sz ligature)
ENTATOM( agrave ),     // Small a, grave accent
ENTATOM( aacute ),     // Small a, acute accent
ENTATOM( acirc  ),     // Small a, circumflex accent
ENTATOM( atilde ),     // Small a, tilde
ENTATOM( auml   ),     // Small a, dieresis or umlaut mark
ENTATOM( aring  ),     // Small a, ring
ENTATOM( aelig  ),     // Small ae dipthong (ligature)
ENTATOM( ccedil ),     // Small c, cedilla
ENTATOM( egrave ),     // Small e, grave accent
ENTATOM( eacute ),     // Small e, acute accent
ENTATOM( ecirc  ),     // Small e, circumflex accent
ENTATOM( euml   ),     // Small e, dieresis or umlaut mark
ENTATOM( igrave ),     // Small i, grave accent
ENTATOM( iacute ),     // Small i, acute accent
ENTATOM( icirc  ),     // Small i, circumflex accent
ENTATOM( iuml   ),     // Small i, dieresis or umlaut mark
ENTATOM( eth    ),     // Small eth, Icelandic
ENTATOM( ntilde ),     // Small n, tilde
ENTATOM( ograve ),     // Small o, grave accent
ENTATOM( oacute ),     // Small o, acute accent
ENTATOM( ocirc  ),     // Small o, circumflex accent
ENTATOM( otilde ),     // Small o, tilde
ENTATOM( ouml   ),     // Small o, dieresis or umlaut mark
ENTATOM( divide ),     // divide
ENTATOM( oslash ),     // Small o, slash
ENTATOM( ugrave ),     // Small u, grave accent
ENTATOM( uacute ),     // Small u, acute accent
ENTATOM( ucirc  ),     // Small u, circumflex accent
ENTATOM( uuml   ),     // Small u, dieresis or umlaut mark
ENTATOM( yacute ),     // Small y, acute accent
ENTATOM( thorn  ),     // Small thorn, Icelandic
ENTATOM( yuml   ),     // Small y, dieresis or umlaut mark
};

CConstantAtomPool::CEntryRange s_erEntities160to255 = 
{
    160,                    // _numberFirst
    255+1-160,              // _cAtoms
    s_capEntities160To255,  // _ppchAtoms
};

CConstantAtomPool s_capEntities = 
{
    1,
    1,
    1,
    &s_erEntities160to255,
    {
ENTATOM( nbsp   ),     // Non breaking space
ENTATOM( iexcl  ),     //
ENTATOM( cent   ),     // cent
ENTATOM( pound  ),     // pound
    }
};

void TestHash() { s_capEntities.TrialHash(); }

