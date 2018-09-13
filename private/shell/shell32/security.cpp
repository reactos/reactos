#include "shellprv.h"

extern "C" {
#include <shellp.h>
#include "ole2dup.h"
};

#include "util.h"
#include "_security.h"

/**********************************************************************\
    FUNCTION: ZoneCheckPidl

    DESCRIPTION:
        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
STDAPI ZoneCheckPidl(LPCITEMIDLIST pidl, DWORD dwActionType, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    HRESULT hr = E_FAIL;
    TCHAR szUrl[MAX_URL_STRING];

    SetFlag(dwFlags, PUAF_ISFILE);

    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, szUrl, SIZECHARS(szUrl), NULL)))
        hr = ZoneCheckUrl(szUrl, dwActionType, dwFlags, pisms);

    return hr;
}

/**********************************************************************\
    FUNCTION: ZoneCheckHDrop

    DESCRIPTION:
        Return S_OK if access is allowed.  This function will return
    S_FALSE if access was not allowed.
\**********************************************************************/
STDAPI ZoneCheckHDrop(IDataObject * pido, DWORD dwEffect, DWORD dwAction, DWORD dwFlags, IInternetSecurityMgrSite * pisms)
{
    HRESULT hr = E_FAIL;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    ASSERT(pido);

    // asking for CF_HDROP
    hr = pido->GetData(&fmte, &medium);
    // Bryanst: Let me know the drag source that caused this assert.
    if (SUCCEEDED(hr))
    {
        HDROP hDrop = (HDROP)medium.hGlobal;
        TCHAR szPath[MAX_PATH];
        DWORD dwCycleFlags;
        UINT nIndex;

        // Bryanst: Let me know the drag source that caused this assert.
        if (EVAL(hDrop))
        {
            UINT nNumOfFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, NULL);
            IInternetSecurityManager * pismCache = NULL;

            dwFlags |= PUAF_ISFILE;
            dwCycleFlags = dwFlags | PUAF_NOUI;

            // We will cycle thru all of the files in this HDROP until we find
            // the first questionable item.
            for (nIndex = 0; ((nIndex < nNumOfFiles) && DragQueryFile(hDrop, nIndex, szPath, ARRAYSIZE(szPath))); nIndex++)
            {
                if (S_OK != ZoneCheckUrlExCache(szPath, NULL, 0, NULL, 0, dwAction, dwCycleFlags, pisms, &pismCache))
                {
                    // We found a questionable file, so zone check that file.  This will be the only
                    // file we zone check because it will either cancel the operation or the user
                    // will explicitely agree to it.
                    hr = ZoneCheckUrlExCache(szPath, NULL, 0, NULL, 0, dwAction, dwFlags, pisms, &pismCache);
                    if (FAILED(hr))
                        TraceMsg(DM_TRACE, "ZoneCheckHDrop(%s) Cancelling Drag & Drop or Copy/Paste because Zones Security check failed.", szPath);

                    break;
                }
            }
            if (pismCache)
                pismCache->Release();
        }
        ReleaseStgMedium(&medium);
    }
    else
    {
#ifdef DEBUG
        IEnumFORMATETC * penumFormatetc;

        // BUGBUG: We can not obtain enough information to Zone Check the source of this information.
        TraceMsg(TF_WARNING, "ZoneCheckHDrop() ***********************************************************************");
        TraceMsg(TF_WARNING, "ZoneCheckHDrop() SECURITY - SECURITY - SECURITY - SECURITY - SECURITY - SECURITY");
        TraceMsg(TF_WARNING, "ZoneCheckHDrop() WARNING: No information included in the IDataObject to Zone Check.");
        TraceMsg(TF_WARNING, "ZoneCheckHDrop()          dwEffect=%#08lx; dwEffect=%#08lx; hr=%lx", dwEffect, dwAction, hr);
        TraceMsg(TF_WARNING, "ZoneCheckHDrop()");

        if (SUCCEEDED(pido->EnumFormatEtc(DATADIR_GET, &penumFormatetc)))
        {
            FORMATETC rgelt;

            while (S_OK == penumFormatetc->Next(1, &rgelt, NULL))
            {
                TraceMsg(TF_WARNING, "ZoneCheckHDrop() cfFormat=%#08lx; dwAspect=%#08lx; lindex=%ld; tymed=%lx;", rgelt.cfFormat, rgelt.dwAspect, rgelt.lindex, rgelt.tymed);
            }

            penumFormatetc->Release();
        }
        else
        {
            TraceMsg(TF_WARNING, "ZoneCheckHDrop()  Unable to list type supported because EnumFormatEtc() failed.");
        }

        TraceMsg(TF_WARNING, "ZoneCheckHDrop() ***********************************************************************");
#endif // DEBUG
        hr = S_OK;      // Allow the action to continue any way.
    }

    return hr;
}
