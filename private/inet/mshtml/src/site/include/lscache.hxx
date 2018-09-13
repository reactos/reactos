/*
 *  @doc    INTERNAL
 *
 *  @module LSCACHE.HXX -- CLSCache and related classes implementation
 *
 *  Owner: <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      1/26/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_LSCACHE_HXX_
#define I_LSCACHE_HXX_
#pragma INCMSG("--- Beg 'lscache.hxx'")

class CDoc;
class CLSCache;
class CLineServices;

MtExtern(CAryLSCache_aryLSCacheEntry_pv)

// We will keep atleast these many contexts around.
#define N_CACHE_MINSIZE 1

class CLSCacheEntry
{
    friend class CLSCache;
    
private:
    CLineServices * _pLS;
    BOOL            _fUsed;
};

class CLSCache
{
private:
    DECLARE_CDataAry (CAryLSCache, CLSCacheEntry, Mt(Mem), Mt(CAryLSCache_aryLSCacheEntry_pv))
    CAryLSCache _aLSEntries;
    LONG        _cUsed;
    
public:
    CLSCache()     { _cUsed = 0; }
    CLineServices *GetFreeEntry(CMarkup *pMarkup, BOOL fStartUpLSDLL);
    void           ReleaseEntry(CLineServices *pLS);
    void           Dispose(BOOL fDisposeAll);
    WHEN_DBG(void  VerifyNoneUsed() { Assert(_cUsed == 0); })
};

#pragma INCMSG("--- End 'lscache.hxx'")
#else
#pragma INCMSG("*** Dup 'lscache.hxx'")
#endif
