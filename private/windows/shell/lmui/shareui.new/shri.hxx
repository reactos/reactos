//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       shri.hxx
//
//  Contents:   Class object encapsulating a generic "share", that may be
//              realized via one or more file servers.
//
//  History:    8-Mar-96   BruceFo     Created
//
//----------------------------------------------------------------------------

#ifndef __SHRI_HXX__
#define __SHRI_HXX__

#include "shares.h"
#include "dlink.hxx"

//
// Note on memory handling: This class contains strings which point into the
// buffers returned by the various file server enumeration functions. Do not
// free those enumeration buffers until objects of this class are freed.
//

class CShare : public CDoubleLink
{
    DECLARE_SIG;

public:

    CShare();   // create new info
    ~CShare();

    VOID
    AddSmb(
        IN SHARE_INFO_2* pInfo  // may point to level 1 info???
        );

    VOID
    AddFpnw(
        IN FPNWVOLUMEINFO* pInfo
        );

    VOID
    AddSfm(
        IN AFP_VOLUME_INFO* pInfo
        );

    PWSTR
    GetName(
        VOID
        );

    PWSTR
    GetPath(
        VOID
        );

    VOID
    FillID(
        OUT LPIDSHARE pids
        );

#if DBG == 1
    VOID
    Dump(
        IN PWSTR pszCaption
        );
#endif // DBG == 1

private:

    //
    // Main object data
    //

    DWORD               m_dwService;    // mask of SHARE_SERVICE_*
    SHARE_INFO_2*       m_pSmbInfo;     // may point to level 1 info???
    AFP_VOLUME_INFO*    m_pSfmInfo;
    FPNWVOLUMEINFO*     m_pFpnwInfo;
};

#endif // __SHRI_HXX__
