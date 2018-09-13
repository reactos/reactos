/*
 *  @doc    INTERNAL
 *
 *  @module FONTINFO.H -- Font information
 *  
 *  Purpose:
 *      Font info, used with caching and fontlinking
 *  
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      6/25/98     cthrash     Created
 *
 *  Copyright (c) 1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__FONTINFO_HXX_
#define I__FONTINFO_HXX_
#pragma INCMSG("--- Beg 'fontinfo.hxx'")

#ifndef X_UNISID_H
#define X_UNISID_H
#include <unisid.h>
#endif

MtExtern(CFontInfoCache);
MtExtern(CFontInfoCache_pv);

class CFontInfo
{
public:
    CStr        _cstrFaceName;
    SCRIPT_IDS  _sids;
};

class CFontInfoCache : public CDataAry<CFontInfo>
{
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CFontInfoCache))
    CFontInfoCache() : CDataAry<CFontInfo>(Mt(CFontInfoCache_pv)) {}
    void    Free();

    HRESULT AddInfoToAtomTable(LPCTSTR pch, LONG *plIndex);
    HRESULT GetAtomFromName(LPCTSTR pch, LONG *plIndex);
    HRESULT GetInfoFromAtom(long lIndex, CFontInfo ** ppfi);
    HRESULT SetScriptIDsOnAtom(LONG lIndex, SCRIPT_IDS sids)
    {
        CFontInfo * pfi;
        HRESULT hr = THR(GetInfoFromAtom(lIndex, &pfi));
        if (!hr)
        {
            pfi->_sids = sids;
        }
        RRETURN(hr);
    }
    HRESULT AddScriptIDOnAtom(LONG lIndex, SCRIPT_ID sid)
    {
        CFontInfo * pfi;
        HRESULT hr = THR(GetInfoFromAtom(lIndex, &pfi));
        if (!hr)
        {
            pfi->_sids |= ScriptBit(sid);
        }
        RRETURN(hr);
    }

    WHEN_DBG( void Dump() );
};

#pragma INCMSG("--- End 'fontinfo.hxx'")
#else
#pragma INCMSG("*** Dup 'fontinfo.hxx'")
#endif
