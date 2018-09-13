//+----------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1999
//
//  File:       peerUrlMap.hxx
//
//  Contents:   peer factory url map
//
//-----------------------------------------------------------------------------

#ifndef I_PEERURLMAP_HXX_
#define I_PEERURLMAP_HXX_
#pragma INCMSG("--- Beg 'peerurlmap.hxx'")

///////////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////////

MtExtern(CPeerFactoryUrlMap)
MtExtern(CPeerFactoryUrlMap_aryFactories)

class CPeerFactoryUrl;

///////////////////////////////////////////////////////////////////////////////
//
// Class:   CPeerMgr
//
///////////////////////////////////////////////////////////////////////////////

class CPeerFactoryUrlMap : public CVoid
{
public:

    DECLARE_CLASS_TYPES(CPeerFactoryUrlMap, CVoid)
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CPeerFactoryUrlMap))

    //
    // methods
    //

    CPeerFactoryUrlMap(CDoc * pDoc);
    ~CPeerFactoryUrlMap();

    HRESULT EnsurePeerFactoryUrl(LPTSTR pchUrl, CMarkup * pMarkup, CPeerFactoryUrl ** ppFactory);

    HRESULT StopDownloads();

    //
    // data
    //

    CDoc *              _pDoc;

    DECLARE_CPtrAry(CAryFactories, CPeerFactoryUrl*, Mt(Mem), Mt(CPeerFactoryUrlMap_aryFactories))
    CAryFactories       _aryFactories;

    CStringTable        _UrlMap;
};

///////////////////////////////////////////////////////////////////////////////
// eof

#pragma INCMSG("--- End 'peerurlmap.hxx'")
#else
#pragma INCMSG("*** Dup 'peerurlmap.hxx'")
#endif
 