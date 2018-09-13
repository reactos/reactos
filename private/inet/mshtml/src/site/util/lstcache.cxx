//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       numconv.cxx
//
//  Contents:   Numeral String Conversions
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TEXTEDIT_H_
#define X_TEXTEDIT_H_
#include "textedit.h"
#endif

#ifndef X_ARRAY_HXX_
#define X_ARRAY_HXX_
#include "array.hxx"
#endif

#ifndef X_ELEMENT_H_
#define X_ELEMENT_H_
#include "element.h"
#endif

#ifndef X_LSTCACHE_HXX_
#define X_LSTCACHE_HXX_
#include "lstcache.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Member:     Set( wLevel, pLI, e )
//
//  Purpose:    Sets an entry in one of the two index caches.  The index
//              caches are CArray<LONG>'s.  They grow as necessary, but
//              do not ever shrink.
//
//              One important side effect is that the most current CListIndex
//              is stashed in liCurrentIndexValue.  This facilitates the
//              speedy query at render time.
//
//  Arguements: wLevel is a zero-based index into the cache. Note that
//              a level of zero indicates that there are no list containers.
//              lValue is the new value
//              e is either INDEXCACHE_TOP or INDEXCACHE_CURRENT
//
//  Returns:    Nothing.
//
//----------------------------------------------------------------------------

HRESULT
CListCache::Set(
    WORD wLevel,
    struct CListIndex * pLI,
    enum INDEXCACHE e)
{
    HRESULT hr = S_OK;

    // Make space if necessary.

    if (wLevel >= Depth(e))
    {
        if (!_pCache->_aryIndex[e].Add( 1 + wLevel - Depth(e), NULL ))
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // Set the value.  Cache the value in liCurrentIndexValue for
    // speedy exectution of GetListIndex().

    *(_pCache->_aryIndex[e].Elem( wLevel )) = liCurrentIndexValue = *pLI;

    // Set the bit field, indicate that the array entry is valid.

    CListCache::ValidateLevel( wLevel, e );

Cleanup:

    RRETURN(hr);
}

HRESULT
CListCache::Instantiate()
{
    Assert( !Instantiated() );

    _pCache = new CListCacheInst;

    if (_pCache)
    {
        PrepareListIndexForRender();
        _pCache->_yScroll = 0;
    }

    return _pCache ? S_OK : E_OUTOFMEMORY;
}

void
CListCache::PrepareListIndexForRender( void )
{
    if (Instantiated())
    {
        ZeroMemory( _pCache->_abIndex[INDEXCACHE_CURRENT], (CListing::MAXLEVELS + 7) >> 3);

        _pCache->_aryIndex[INDEXCACHE_CURRENT].CopyFrom( _pCache->_aryIndex[INDEXCACHE_TOP] );
    }
}

#if DBG==1
void
CListCache::Dump()
{
    if (Instantiated())
    {
        int i;

        for (i=0;i<2;i++)
        {
            enum INDEXCACHE e = i ? INDEXCACHE_CURRENT : INDEXCACHE_TOP;
            char ach[256];
            char * pch = ach;
            WORD wLevel;

            pch += wsprintfA(ach, "%s: ", e ? "cur" : "top");

            for (wLevel = 0; wLevel <= Depth(e); wLevel++)
            {
                if (Valid(wLevel,e))
                {
                    CListIndex LI = GetAt(wLevel, e);

                    pch += wsprintfA(pch, "%3d ", LI.lValue);
                }
                else
                {
                    StrCpyA( pch, "--- " );
                    pch += 4;
                }
            }

            StrCpyA(pch, "\r\n");
            
            OutputDebugStringA(ach);
        }
    }
}
#endif
