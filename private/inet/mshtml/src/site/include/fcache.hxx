/*
 *  FCACHE.HXX
 *  
 *  Purpose:
 *      CCharFormatArray and CParaFormatArray
 *  
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#ifndef I_FCACHE_HXX_
#define I_FCACHE_HXX_
#pragma INCMSG("--- Beg 'fcache.hxx'")

#ifndef X_DLL_HXX_
#define X_DLL_HXX_
#include "dll.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_DATCACHE_HXX_
#define X_DATCACHE_HXX_
#include "datcache.hxx"
#endif

    //
    // Handi-dandi inlines
    //

inline const CCharFormat * GetCharFormatEx ( long iCF )
{
    const CCharFormat *pCF = &(*TLS(_pCharFormatCache))[iCF];
    Assert( pCF);
    return pCF;
}

inline const CParaFormat * GetParaFormatEx ( long iPF )
{
    const CParaFormat *pPF = &(*TLS(_pParaFormatCache))[iPF];
    Assert( pPF);
    return pPF;
}

inline const CFancyFormat * GetFancyFormatEx ( long iFF )
{
    const CFancyFormat *pFF = &(*TLS(_pFancyFormatCache))[iFF];
    Assert( pFF);
    return pFF;
}

inline CAttrArray * GetExpandosAttrArrayFromCacheEx( long iStyleAttrArray)
{
    const CAttrArray *pExpandosAttrArray = &(*TLS(_pStyleExpandoCache))[iStyleAttrArray];
    Assert( pExpandosAttrArray);
    return (CAttrArray *)pExpandosAttrArray;
}

#pragma INCMSG("--- End 'fcache.hxx'")
#else
#pragma INCMSG("*** Dup 'fcache.hxx'")
#endif
