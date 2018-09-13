//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       copyhook.cxx
//
//  Contents:   CShareCopyHook implementation
//
//  History:    21-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "copyhook.hxx"
#include "cache.hxx"
#include "shrinfo.hxx"
#include "util.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CShareCopyHook::CShareCopyHook
//
//  Synopsis:   Constructor
//
//  History:    21-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

CShareCopyHook::CShareCopyHook(
    VOID
    )
    :
    _uRefs(0)
{
    INIT_SIG(CShareCopyHook);

    AddRef(); // give it the correct initial reference count. add to the DLL reference count
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCopyHook::~CShareCopyHook
//
//  Synopsis:   Destructor
//
//  History:    21-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

CShareCopyHook::~CShareCopyHook()
{
    CHECK_SIG(CShareCopyHook);
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCopyHook::CopyCallback
//
//  Derivation: ICopyHook
//
//  Synopsis:   Called when the shell is copying an object
//
//  History:    21-Apr-95 BruceFo  Created
//
// BUGBUG: instead of deleting a share on a directory move, how about
// moving the share?
//
//--------------------------------------------------------------------------

STDMETHODIMP_(UINT)
CShareCopyHook::CopyCallback(
    HWND hwnd,
    UINT wFunc,
    UINT fFlags,
    LPCWSTR pszSrcFile,
    DWORD dwSrcAttribs,
    LPCWSTR pszDestFile,
    DWORD dwDestAttribs
    )
{
    appDebugOut((DEB_TRACE,
        "CShareCopyHook::CopyCallback. %ws -> %ws\n",
        pszSrcFile, pszDestFile));

    UINT idMsg;

    if (!(dwSrcAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
       return IDYES;  //We're only worried about directories
    }

    if (!g_fSharingEnabled)
    {
        return IDYES;
    }

    switch (wFunc)
    {
        case FO_DELETE:
            idMsg = MSG_RMDIRCONFIRM;
            break;

        case FO_RENAME:
        case FO_MOVE:
            idMsg = MSG_MVDIRCONFIRM;
            break;

        default:
            return IDYES;
    }

    BOOL fChange = FALSE;
    UINT wnErr = IDYES;    /* by default, shell should go ahead and do it */
    CShareInfo* pWarnList = NULL;
    HRESULT hr = g_ShareCache.ConstructParentWarnList(pszSrcFile, &pWarnList);
    if (SUCCEEDED(hr))
    {
        if (NULL != pWarnList)
        {
            for (CShareInfo* p = (CShareInfo*) pWarnList->Next();
                 p != pWarnList;
                 p = (CShareInfo*) p->Next())
            {
                wnErr = WarnDelShare(hwnd, idMsg, p->GetNetname(), p->GetPath());
                if (wnErr != IDYES)
                {
                    // IDYES: obviously, continue
                    break;
                }

                fChange = TRUE;
            }

            // get rid of the temporary list
            DeleteShareInfoList(pWarnList, TRUE);

            if (fChange)
            {
                g_ShareCache.Refresh();
            }
        }
    }
    return wnErr;
}
