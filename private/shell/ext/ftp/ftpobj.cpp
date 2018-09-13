/*****************************************************************************
 *
 *    ftpobj.cpp - IDataObject interface
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpobj.h"
#include "ftpurl.h"
#include <shlwapi.h>


// CLSIDs
// {299D0193-6DAA-11d2-B679-006097DF5BD4}
const GUID CLSID_FtpDataObject = { 0x299d0193, 0x6daa, 0x11d2, 0xb6, 0x79, 0x0, 0x60, 0x97, 0xdf, 0x5b, 0xd4 };

/*****************************************************************************
 *
 *    g_dropTypes conveniently mirrors our FORMATETCs.
 *
 *    Hardly coincidence, of course.  Enum_Fe did the real work.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *    Preinitialized global data.
 *
 *****************************************************************************/
FORMATETC g_formatEtcOffsets;
FORMATETC g_formatPasteSucceeded;
CLIPFORMAT g_cfTargetCLSID;

FORMATETC g_dropTypes[] =
{
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_ISTREAM },  // DROP_FCont
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_FGDW
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_FGDA
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_IDList
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_URL
//    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_Offsets
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_PrefDe
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_PerfDe
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_FTP_PRIVATE
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_OLEPERSIST - see _RenderOlePersist() for desc.
    { CF_HDROP, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },  // DROP_Hdrop
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }, // DROP_FNMA
    { 0, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL }  // DROP_FNMW
};



/*****************************************************************************\
    GLOBAL: c_stgInit

    DESCRIPTION:
        Mostly straightforward.  The only major weirdness is that cfURL
    is delay-rendered iff the m_pflHfpl contains only one object.  Otherwise,
    cfURL is not supported.  (URLs can refer to only one object at a time.)
\*****************************************************************************/
STGMEDIUM c_stgInit[] =
{
    { 0, 0, 0 },  // DROP_FCont
    { TYMED_HGLOBAL, 0, 0 },    // DROP_FGDW - delay-rendered
    { TYMED_HGLOBAL, 0, 0 },    // DROP_FGDA - delay-rendered
    { TYMED_HGLOBAL, 0, 0 },    // DROP_IDList - delay-rendered
    { 0, 0, 0 },                // DROP_URL - opt delay-rendered
//    { 0, 0, 0 },                // DROP_Offsets
    { TYMED_HGLOBAL, 0, 0 },    // DROP_PrefDe - delay-rendered
    { 0, 0, 0 },                // DROP_PerfDe
    { TYMED_HGLOBAL, 0, 0 },    // DROP_FTP_PRIVATE
    { TYMED_HGLOBAL, 0, 0 },    // DROP_OLEPERSIST - see _RenderOlePersist() for desc.
    { 0, 0, 0 },                // DROP_Hdrop
    { 0, 0, 0 },                // DROP_FNMA
    { 0, 0, 0 }                 // DROP_FNMW
};



/*****************************************************************************\
    FUNCTION: TraceMsgWithFormatEtc

    DESCRIPTION:
\*****************************************************************************/
void TraceMsgWithFormat(DWORD dwFlags, LPCSTR pszBefore, LPFORMATETC pFormatEtc, LPCSTR pszAfter, HRESULT hr)
{
#ifdef DEBUG
    TCHAR szFormatName[MAX_PATH];
    TCHAR szMedium[MAX_PATH];

    szFormatName[0] = 0;
    szMedium[0] = 0;
    if (pFormatEtc)
    {
        // This may fail if it's a basic format.
        if (!GetClipboardFormatName(pFormatEtc->cfFormat, szFormatName, ARRAYSIZE(szFormatName)))
            wnsprintf(szFormatName, ARRAYSIZE(szFormatName), TEXT("Pre-defined=%d"), pFormatEtc->cfFormat);

        switch (pFormatEtc->tymed)
        {
        case TYMED_HGLOBAL: StrCpyN(szMedium, TEXT("HGLOBAL"), ARRAYSIZE(szMedium)); break;
        case TYMED_FILE: StrCpyN(szMedium, TEXT("File"), ARRAYSIZE(szMedium)); break;
        case TYMED_GDI: StrCpyN(szMedium, TEXT("GDI"), ARRAYSIZE(szMedium)); break;
        case TYMED_MFPICT: StrCpyN(szMedium, TEXT("MFPICT"), ARRAYSIZE(szMedium)); break;
        case TYMED_ENHMF: StrCpyN(szMedium, TEXT("ENHMF"), ARRAYSIZE(szMedium)); break;
        case TYMED_ISTORAGE: StrCpyN(szMedium, TEXT("ISTORAGE"), ARRAYSIZE(szMedium)); break;
        case TYMED_ISTREAM: StrCpyN(szMedium, TEXT("ISTREAM"), ARRAYSIZE(szMedium)); break;
        }
    }
    else
    {
        szMedium[0] = 0;
    }

    TraceMsg(dwFlags, "%hs [FRMTETC: %ls, lndx: %d, %ls] hr=%#08lx, %hs", pszBefore, szFormatName, pFormatEtc->lindex, szMedium, hr, pszAfter);
#endif // DEBUG
}


/*****************************************************************************\
    FUNCTION: _IsLindexOkay
 
   DESCRIPTION:
    If ife != DROP_FCont, then pfeWant->lindex must be -1.
 
    If ife == DROP_FCont, then pfeWant->lindex must be in the range
    0 ... m_pflHfpl->GetCount() - 1
\*****************************************************************************/
BOOL CFtpObj::_IsLindexOkay(int ife, FORMATETC *pfeWant)
{
    BOOL fResult;

    if (ife != DROP_FCont)
        fResult = pfeWant->lindex == -1;
    else
        fResult = (LONG)pfeWant->lindex < m_pflHfpl->GetCount();

    return fResult;
}


/*****************************************************************************\
    FUNCTION: _FindData

    DESCRIPTION:
        Locate our FORMATETC/STGMEDIUM given a FORMATETC from somebody else.
    On success, stores the index found into *piOut.
 
    We do not allow clients to change the TYMED of a FORMATETC, so
    in fact checking the TYMED is what we want, even on a SetData.
\*****************************************************************************/
HRESULT CFtpObj::_FindData(FORMATETC *pfe, PINT piOut)
{
    int nIndex;
    HRESULT hres = DV_E_FORMATETC;

    *piOut = 0;
    for (nIndex = DROP_FCont; nIndex < DROP_OFFERMAX; nIndex++)
    {
        ASSERT(0 == (g_dropTypes[nIndex]).ptd);
        ASSERT(g_dropTypes[nIndex].dwAspect == DVASPECT_CONTENT);

        if ((pfe->cfFormat == g_dropTypes[nIndex].cfFormat) && !ShouldSkipDropFormat(nIndex))
        {
            if (EVAL(g_dropTypes[nIndex].ptd == NULL))
            {
                if (EVAL(pfe->dwAspect == DVASPECT_CONTENT))
                {
                    if (EVAL(g_dropTypes[nIndex].tymed & pfe->tymed))
                    {
                        if (EVAL(_IsLindexOkay(nIndex, pfe)))
                        {
                            *piOut = nIndex;
                            hres = S_OK;
                        }
                        else
                            hres = DV_E_LINDEX;
                    }
                    else
                        hres = DV_E_TYMED;
                }
                else
                    hres = DV_E_DVASPECT;
            }
            else
                hres = DV_E_DVTARGETDEVICE;
            break;
        }
    }

    return hres;
}


/*****************************************************************************\
    FUNCTION: _FindDataForGet
 
    DESCRIPTION:
        Locate our FORMATETC/STGMEDIUM given a FORMATETC from somebody else.
    On success, stores the index found into *piOut.  Unlike _FindData, we will
    fail the call if the data object doesn't currently have the clipboard format.
    (Delayed render counts as "currently having it".  What we are filtering out
    are formats for which GetData will necessarily fail.)
\*****************************************************************************/
HRESULT CFtpObj::_FindDataForGet(FORMATETC *pfe, PINT piOut)
{
    HRESULT hr = _FindData(pfe, piOut);

    // TODO: g_cfHIDA should return an array of pidls for each folder.
    //       If we do this, the caller will support creating Shortcuts
    //       (LNK files) that point to these pidls.  We may want to do 
    //       that later.

    if (SUCCEEDED(hr))
    {
        if (*piOut != DROP_FCont)
        {
            if (m_stgCache[*piOut].tymed)
            {
                // Do we have data at all?
                // (possibly delay-rendered)
            }
            else
                hr = DV_E_FORMATETC;        // I guess not
        }
        else
        {
            // File contents always okay
        }
    }

#ifdef DEBUG
    if (FAILED(hr))
    {
        //TraceMsg(TF_FTPDRAGDROP, "CFtpObj::_FindDataForGet(FORMATETC.cfFormat=%d) Failed.", pfe->cfFormat);
        *piOut = 0xBAADF00D;
    }
#endif

    return hr;
}


// The following are used to enumerate sub directories when creating a list of pidls for
// a directory download (Ftp->FileSys).
typedef struct tagGENPIDLLIST
{
    CFtpPidlList *      ppidlList;
    IMalloc *           pm;
    IProgressDialog *   ppd;
    CWireEncoding *     pwe;
} GENPIDLLIST;


/*****************************************************************************\
     FUNCTION: ProcessItemCB
 
    DESCRIPTION:
        This function will add the specified pidl to the list.  It will then
    detect if it's a folder and if so, will call EnumFolder() to recursively
    enum it's contents and call ProcessItemCB() for each one.
 
    PARAMETERS:
\*****************************************************************************/
HRESULT ProcessItemCB(LPVOID pvFuncCB, HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, LPVOID pvData)
{
    GENPIDLLIST * pGenPidlList = (GENPIDLLIST *) pvData;
    HRESULT hr = S_OK;

    // Does the user want to cancel?
    if (pGenPidlList->ppd && pGenPidlList->ppd->HasUserCancelled())
    {
        EVAL(SUCCEEDED(pGenPidlList->ppd->StopProgressDialog()));
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    if (SUCCEEDED(hr))
    {
        // No, don't cancel so continue...

        // Add everything except SoftLinks.
        // This is because dir SoftLinks may cause infinite recurion.
        // Someday, we may want to upload a shortcut but
        // that's too much work for now.
        if (0 != FtpPidl_GetAttributes(pidlFull))
        {
            // We exist to do this:
            pGenPidlList->ppidlList->InsertSorted(pidlFull);
        }

        // Is this a dir/folder that we need to recurse into?
        if (SUCCEEDED(hr) && (FILE_ATTRIBUTE_DIRECTORY & FtpPidl_GetAttributes(pidlFull)))
        {
            hr = EnumFolder((LPFNPROCESSITEMCB) pvFuncCB, hint, pidlFull, pGenPidlList->pwe, pfValidhinst, pvData);
        }
    }

    return hr;
}


/*****************************************************************************\
     FUNCTION: _ExpandPidlListRecursively
 
    DESCRIPTION:
        This function will take the pidl list (ppidlListSrc) and call into it
    to enumerate.  It will provide ProcessItemCB as the callback function.
    This function will help it create a new CFtpPidlList which will not only
    contain the pidls in a base folder, but also all the pidls in any subfolders
    that are in the original list.

    Delay-render a file group descriptor.
\*****************************************************************************/
CFtpPidlList * CFtpObj::_ExpandPidlListRecursively(CFtpPidlList * ppidlListSrc)
{
    GENPIDLLIST pep = {0};

    pep.ppidlList = NULL;
    pep.ppd = m_ppd;
    pep.pwe = m_pff->GetCWireEncoding();
    if (SUCCEEDED(CFtpPidlList_Create(0, NULL, &pep.ppidlList)))
    {
        m_pff->GetItemAllocator(&pep.pm);

        if (EVAL(m_pfd) && EVAL(pep.pm))
        {
            HINTERNET hint;

            if (SUCCEEDED(m_pfd->GetHint(NULL, NULL, &hint, NULL, m_pff)))
            {
                LPITEMIDLIST pidlRoot = ILClone(m_pfd->GetPidlReference());

                if (EVAL(pidlRoot))
                {
                    HRESULT hr = ppidlListSrc->RecursiveEnum(pidlRoot, ProcessItemCB, hint, (LPVOID) &pep);
                   
                    if (m_ppd)
                        EVAL(SUCCEEDED(m_ppd->StopProgressDialog()));

                    if (FAILED(hr) && (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr) && !m_fErrAlreadyDisplayed)
                    {
                        pep.ppidlList->Release();
                        pep.ppidlList = NULL;

                        // Oh, I want a real hwnd, but where or where can I get one?
                        DisplayWininetErrorEx(NULL, TRUE, HRESULT_CODE(hr), IDS_FTPERR_TITLE_ERROR, IDS_FTPERR_DROPFAIL, IDS_FTPERR_WININET, MB_OK, NULL, NULL);
                        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);  // Wrong permissions

                        // We need to suppress subsequent error dlgs from this location
                        // because callers like to ask for FILEGROUPDESCRIPTORA and
                        // if that fails, ask for FILEGROUPDESCRIPTORW and we don't
                        // want an error dialog for each.
                        m_fErrAlreadyDisplayed = TRUE;
                    }

                    ILFree(pidlRoot);
                }

                m_pfd->ReleaseHint(hint);
                pep.pm->Release();
            }
        }
    }

    return pep.ppidlList;
}


/*****************************************************************************\
    FUNCTION: _DelayRender_FGD

    DESCRIPTION:
        Delay-render a file group descriptor
\*****************************************************************************/
HGLOBAL CFtpObj::_DelayRender_FGD(BOOL fUnicode)
{
    HGLOBAL hGlobal = NULL;
    
    if (m_fCheckSecurity &&
        ZoneCheckPidlAction(SAFECAST(this, IInternetSecurityMgrSite *), URLACTION_SHELL_FILE_DOWNLOAD, m_pff->GetPrivatePidlReference(), (PUAF_DEFAULT | PUAF_WARN_IF_DENIED)))
    {
        m_pflHfpl->TraceDump(m_pff->GetPrivatePidlReference(), TEXT("_DelayRender_FGD() TraceDump before"));
        CFtpPidlList * pPidlList;
    
        if (!m_fFGDRendered)
        {
            pPidlList = _ExpandPidlListRecursively(m_pflHfpl);
            if (pPidlList)
            {
                // We succeeded so now it's expanded.
                m_fFGDRendered = TRUE;
            }
        }
        else
        {
            m_pflHfpl->AddRef();
            pPidlList = m_pflHfpl;
        }

        if (pPidlList)
        {
            hGlobal = Misc_HFGD_Create(pPidlList, m_pff->GetPrivatePidlReference(), fUnicode);
            IUnknown_Set(&m_pflHfpl, pPidlList);
            m_pflHfpl->TraceDump(m_pff->GetPrivatePidlReference(), TEXT("_DelayRender_FGD() TraceDump after"));
            pPidlList->Release();
        }
    }
    else
    {
        // Suppress future UI.  We don't need to check any more
        // because our pidl won't change.  We could not pass PUAF_WARN_IF_DENIED
        // but that won't suppress the UI in the prompt case. (Only admins can
        // turn on the prompt case).
        m_fCheckSecurity = FALSE;
    }

    return hGlobal;
}


/*****************************************************************************\
    FUNCTION: _DelayRender_IDList

    DESCRIPTION:
        Delay-render an ID List Array (HIDA)
\*****************************************************************************/
HRESULT CFtpObj::_DelayRender_IDList(STGMEDIUM * pStgMedium)
{
    pStgMedium->hGlobal = Misc_HIDA_Create(m_pff->GetPublicRootPidlReference(), m_pflHfpl);

    ASSERT(pStgMedium->hGlobal);
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: _DelayRender_URL

    DESCRIPTION:
        The caller wants an URL in an Ansi String
\*****************************************************************************/
HRESULT CFtpObj::_DelayRender_URL(STGMEDIUM * pStgMedium)
{
    LPSTR pszUrl = NULL;
    LPITEMIDLIST pidlFull = NULL;
    LPITEMIDLIST pidl = m_pflHfpl->GetPidl(0);

    ASSERT(pidl);   // We need this
    // Sometimes m_pflHfpl->GetPidl(0) is fully qualified and
    // sometimes it's not.
    if (!FtpID_IsServerItemID(pidl))
    {
        pidlFull = ILCombine(m_pfd->GetPidlReference(), pidl);
        pidl = pidlFull;
    }

    ASSERT(m_pflHfpl->GetCount() == 1); // How do we give them more than 1 URL?
    if (pidl)
    {
        TCHAR szUrl[MAX_URL_STRING];

        if (EVAL(SUCCEEDED(UrlCreateFromPidl(pidl, SHGDN_FORADDRESSBAR, szUrl, ARRAYSIZE(szUrl), (ICU_ESCAPE | ICU_USERNAME), TRUE))))
        {
            DWORD cchSize = (lstrlen(szUrl) + 1);

            pszUrl = (LPSTR) LocalAlloc(LPTR, (cchSize * sizeof(CHAR)));
            if (EVAL(pszUrl))
                SHTCharToAnsi(szUrl, pszUrl, cchSize);
        }

        ILFree(pidlFull);
    }

    pStgMedium->hGlobal = (HGLOBAL) pszUrl;
    return S_OK;
}



#pragma BEGIN_CONST_DATA

DROPEFFECT c_deCopyLink = DROPEFFECT_COPY | DROPEFFECT_LINK;
DROPEFFECT c_deLink     =          DROPEFFECT_LINK;

#pragma END_CONST_DATA
/*****************************************************************************\
    FUNCTION: _DelayRender_PrefDe

    DESCRIPTION:
        Delay-render a preferred drop effect.
 
    The preferred drop effect is DROPEFFECT_COPY (with DROPEFFECT_LINK as fallback),
    unless you are dragging an FTP site, in which case it's just DROPEFFECT_LINK.
 
    DROPEFFECT_MOVE is never preferred.  We can do it; it just isn't preferred.
 
    BUGBUG/NOTES: About DROPEFFECT_MOVE
    We cannot support Move on platforms before NT5 because of a Recycle Bin bug
    were it would clain to have succeeded with the copy but it actually didn't
    copy anything.  On NT5, the Recycle Bin drop target will call pDataObject->SetData()
    with a data type of "Dropped On" and the data being the CLSID of the drop
    target in addition to really copying the files to the recycle bin.  This will 
    let us delete the files knowing they are in the recycle bin.
\*****************************************************************************/
HRESULT CFtpObj::_DelayRender_PrefDe(STGMEDIUM * pStgMedium)
{
    DROPEFFECT * pde;

    if (!m_pfd->IsRoot())
        pde = &c_deCopyLink;
    else
        pde = &c_deLink;

    return Misc_CreateHglob(sizeof(*pde), pde, &pStgMedium->hGlobal);
}


/*****************************************************************************\
    FUNCTION: _RenderOlePersist

    DESCRIPTION:
        When the copy source goes away (the process shuts down), it calls
    OleFlushClipboard.  OLE will then copy our data, release us, and then
    give out our data later.  This works for most things except for:
    1. When lindex needs to very.  This doesn't work because ole doesn't know
       how to ask us how may lindexs they need to copy.
    2. If this object has a private interface OLE doesn't know about.  For us,
       it's IAsyncOperation.

   To get around this problem, we want OLE to recreate us when some possible
   paste target calls OleGetClipboard.  We want OLE to call OleLoadFromStream()
   to have us CoCreated and reload our persisted data via IPersistStream.
   OLE doesn't want to do this by default or they may have backward compat
   problems so they want a sign from the heavens, or at least from us, that
   we will work.  They ping our "OleClipboardPersistOnFlush" clipboard format
   to ask this.
\*****************************************************************************/
HRESULT CFtpObj::_RenderOlePersist(STGMEDIUM * pStgMedium)
{
    // The actual cookie value is opaque to the outside world.  Since
    // we don't use it either, we just leave it at zero in case we use
    // it in the future.  It's mere existence will cause OLE to do the
    // use our IPersistStream, which is what we want.
    DWORD dwCookie = 0;
    return Misc_CreateHglob(sizeof(dwCookie), &dwCookie, &pStgMedium->hGlobal);
}


/*****************************************************************************\
    FUNCTION: _RenderFGD

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::_RenderFGD(int nIndex, STGMEDIUM * pStgMedium)
{
    HRESULT hr = _DoProgressForLegacySystemsPre();

    if (SUCCEEDED(hr))
        pStgMedium->hGlobal = _DelayRender_FGD((DROP_FGDW == nIndex) ? TRUE : FALSE);

    if (!pStgMedium->hGlobal)
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);    // Probably failed because of Zones check.

    return hr;
}


/*****************************************************************************\
    FUNCTION: _ForceRender

    DESCRIPTION:
        We previously delayed rendering the data for perf reasons.  This function
    was called, so we now need to render the data.
\*****************************************************************************/
HRESULT CFtpObj::_ForceRender(int nIndex)
{
    HRESULT hr = S_OK;

    // We only support HGLOBALs here, but the caller may be valid
    // to ask for something we don't support or an extended data.
    //    ASSERT((m_stgCache[nIndex].tymed) == TYMED_HGLOBAL);

    if (!m_stgCache[nIndex].hGlobal)
    {
        STGMEDIUM medium = {TYMED_HGLOBAL, 0, NULL};

        switch (nIndex)
        {
        case DROP_FCont:
            ASSERT(0);
            break;
        case DROP_FGDW:
        case DROP_FGDA:
            hr = _RenderFGD(nIndex, &medium);
            break;
        case DROP_IDList:
            hr = _DelayRender_IDList(&medium);
            break;
/* Nuke
        case DROP_Offsets:
            ASSERT(0);
//            hglob = _DelayRender_Offsets();
            break;
*/
        case DROP_PrefDe:
            hr = _DelayRender_PrefDe(&medium);
            break;
        case DROP_PerfDe:
            ASSERT(0);
//            hglob = _DelayRender_PerfDe();
            break;
        case DROP_FTP_PRIVATE:
            hr = DV_E_FORMATETC;
            break;
        case DROP_OLEPERSIST:
            hr = _RenderOlePersist(&medium);
            break;
        case DROP_Hdrop:
            ASSERT(0);
//            hglob = _DelayRender_Hdrop();
            break;
        case DROP_FNMA:
            ASSERT(0);
//            hglob = _DelayRender_FNM();
            break;
        case DROP_FNMW:
            ASSERT(0);
//            hglob = _DelayRender_FNM();
            break;
        case DROP_URL:
            hr = _DelayRender_URL(&medium);
            break;
        default:
            ASSERT(0);      // Should never hit.
            break;
        }

        if (medium.hGlobal)  // Will fail if the Zones Security Check Fails.
        {
            m_stgCache[nIndex].pUnkForRelease = NULL;
            m_stgCache[nIndex].hGlobal = medium.hGlobal;
        }
        else
        {
            if (S_OK == hr)
                hr = E_OUTOFMEMORY;
        }
    }

    if (FAILED(hr))
        TraceMsg(TF_FTPDRAGDROP, "CFtpObj::_ForceRender() FAILED. hres=%#08lx", hr);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _DoProgressForLegacySystemsPre

    DESCRIPTION:
        Shell's pre-NT5 didn't do progress on the File Contents drop, so we
    will do it here.  This function will display a progress dialog while we
    walk the server and expand the pidls that are needed to be copied.
    Later, 
\*****************************************************************************/
HRESULT CFtpObj::_DoProgressForLegacySystemsPre(void)
{
    HRESULT hr = S_OK;

    if (DEBUG_LEGACY_PROGRESS || (SHELL_VERSION_NT5 > GetShellVersion()))
    {
        TraceMsg(TF_ALWAYS, "CFtpObj::_DoProgressForLegacySystemsPre() going to do the Legacy dialogs.");

        // Do we need to initialize the list?
        if (!m_ppd && (-1 == m_nStartIndex))
        {
            // Yes, so create the create the dialog and find the sizes of the list.
            if (m_ppd)
                _CloseProgressDialog();

            m_uliCompleted.QuadPart = 0;
            m_uliTotal.QuadPart = 0;
            m_ppd = CProgressDialog_CreateInstance(IDS_COPY_TITLE, IDA_FTPDOWNLOAD);
            if (EVAL(m_ppd))
            {
                WCHAR wzProgressDialogStr[MAX_PATH];

                // Tell the user we are calculating how long it will take.
                if (EVAL(LoadStringW(HINST_THISDLL, IDS_PROGRESS_DOWNLOADTIMECALC, wzProgressDialogStr, ARRAYSIZE(wzProgressDialogStr))))
                    EVAL(SUCCEEDED(m_ppd->SetLine(2, wzProgressDialogStr, FALSE, NULL)));

                // We give a NULL punkEnableModless because we don't want to go modal.
                EVAL(SUCCEEDED(m_ppd->StartProgressDialog(NULL, NULL, PROGDLG_AUTOTIME, NULL)));
           }
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _DoProgressForLegacySystemsStart

    DESCRIPTION:
        Shell's pre-NT5 didn't do progress on the File Contents drop, so we
    will do it here.  Only return FAILED(hr) if IProgressDialog::HasUserCancelled().
\*****************************************************************************/
HRESULT CFtpObj::_DoProgressForLegacySystemsStart(LPCITEMIDLIST pidl, int nIndex)
{
    HRESULT hr = S_OK;

    if (DEBUG_LEGACY_PROGRESS || (SHELL_VERSION_NT5 > GetShellVersion()))
    {
        TraceMsg(TF_ALWAYS, "CFtpObj::_DoProgressForLegacySystemsStart() going to do the Legacy dialogs.");

        // Do we need to initialize the list?
        if (-1 == m_nStartIndex)
            hr = _SetProgressDialogValues(nIndex);   // Yes, so do so.

        if (EVAL(m_ppd))
        {
            WCHAR wzTemplate[MAX_PATH];
            WCHAR wzPath[MAX_PATH];
            WCHAR wzStatusText[MAX_PATH];
            LPITEMIDLIST pidlBase = (LPITEMIDLIST) pidl;

            EVAL(SUCCEEDED(m_ppd->StartProgressDialog(NULL, NULL, PROGDLG_AUTOTIME, NULL)));

            // Generate the string "Downloading <FileName>..." status string
            EVAL(LoadStringW(HINST_THISDLL, IDS_DOWNLOADING, wzTemplate, ARRAYSIZE(wzTemplate)));
            wnsprintfW(wzStatusText, ARRAYSIZE(wzStatusText), wzTemplate, FtpPidl_GetLastItemDisplayName(pidl));
            EVAL(SUCCEEDED(m_ppd->SetLine(1, wzStatusText, FALSE, NULL)));

            if (FtpPidl_IsDirectory(pidl, FALSE))
            {
                pidlBase = ILClone(pidl);
                ILRemoveLastID(pidlBase);
            }

            // Generate the string "From <SrcFileDir>" status string
            GetDisplayPathFromPidl(pidlBase, wzPath, ARRAYSIZE(wzPath), TRUE);
            EVAL(LoadStringW(HINST_THISDLL, IDS_DL_SRC_DIR, wzTemplate, ARRAYSIZE(wzTemplate)));
            wnsprintfW(wzStatusText, ARRAYSIZE(wzStatusText), wzTemplate, wzPath);
            EVAL(SUCCEEDED(m_ppd->SetLine(2, wzStatusText, FALSE, NULL)));

            EVAL(SUCCEEDED(m_ppd->SetProgress64(m_uliCompleted.QuadPart, m_uliTotal.QuadPart)));
            TraceMsg(TF_ALWAYS, "CFtpObj::_DoProgressForLegacySystemsStart() SetProgress64(%#08lx, %#08lx)", m_uliCompleted.LowPart, m_uliTotal.LowPart);
            if (m_ppd->HasUserCancelled())
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);

            if (pidlBase != pidl)   // Did we allocated it?
                ILFree(pidlBase);
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: _DoProgressForLegacySystemsPost

    DESCRIPTION:
        Shell's pre-NT5 didn't do progress on the File Contents drop, so we
    will do it here.  Only return FAILED(hr) if IProgressDialog::HasUserCancelled().
\*****************************************************************************/
HRESULT CFtpObj::_DoProgressForLegacySystemsPost(LPCITEMIDLIST pidl, BOOL fLast)
{
    HRESULT hr = S_OK;

    if ((DEBUG_LEGACY_PROGRESS || (SHELL_VERSION_NT5 > GetShellVersion())) && EVAL(m_ppd))
    {
        if (pidl)
        {
            // Add the file size to the Completed.
            m_uliCompleted.QuadPart += FtpPidl_GetFileSize(pidl);
        }

        TraceMsg(TF_ALWAYS, "CFtpObj::_DoProgressForLegacySystemsPost() Closing DLG");

        if (fLast)
            IUnknown_Set((IUnknown **)&m_ppd, NULL);    // The stream will close the dialog and release it.
    }

    return hr;
}


HRESULT CFtpObj::_SetProgressDialogValues(int nIndex)
{
    HRESULT hr = S_OK;

    m_nStartIndex = nIndex;
    if (EVAL(m_ppd))
    {
        // Calculate m_nEndIndex
        while (nIndex < m_pflHfpl->GetCount())
        {
            if (!FtpPidl_IsDirectory(m_pflHfpl->GetPidl(nIndex), FALSE))
                m_nEndIndex = nIndex;
            nIndex++;
        }

        for (nIndex = 0; nIndex < m_pflHfpl->GetCount(); nIndex++)
        {
            LPCITEMIDLIST pidl = m_pflHfpl->GetPidl(nIndex);
            m_uliTotal.QuadPart += FtpPidl_GetFileSize(pidl);
        }

        // Reset because the above for loop can take a long time and the estimated time
        // is based on the time between ::StartProgressDialog() and the first
        // ::SetProgress() call.
        EVAL(SUCCEEDED(m_ppd->Timer(PDTIMER_RESET, NULL)));
   }

    return hr;
}


HRESULT CFtpObj::_CloseProgressDialog(void)
{
    m_nStartIndex = -1; // Indicate we haven't inited yet.
    if (m_ppd)
    {
        EVAL(SUCCEEDED(m_ppd->StopProgressDialog()));
        IUnknown_Set((IUnknown **)&m_ppd, NULL);
    }
    return S_OK;
}


HRESULT CFtpObj::_RefThread(void)
{
    if (NULL == m_punkThreadRef)
    {
        // This is valid to fail from some hosts who won't go away,
        // so they don't need to support ref counting threads.
        SHGetThreadRef(&m_punkThreadRef);
    }

    return S_OK;
}


HRESULT CFtpObj::_RenderFileContents(LPFORMATETC pfe, LPSTGMEDIUM pstg)
{
    HRESULT hr = E_INVALIDARG;

    // callers have a bad habit of asking for lindex == -1 because
    // that means 'all' data.  But how can you hand out one IStream* for
    // all files?
    if (-1 != pfe->lindex)
    {
        LPITEMIDLIST pidl = m_pflHfpl->GetPidl(pfe->lindex);
        //    FileContents are always regenerated afresh.
        pstg->pUnkForRelease = 0;
        pstg->tymed = TYMED_ISTREAM;

        if (EVAL(pidl))
        {
            hr = _DoProgressForLegacySystemsStart(pidl, pfe->lindex);
            if (SUCCEEDED(hr))
            {
                // Is it a directory?
                if (FtpPidl_IsDirectory(pidl, FALSE))
                {
                    // Yes, so pack the name and attributes
                    hr = DV_E_LINDEX;
                    AssertMsg(0, TEXT("Someone is asking for a FILECONTENTs for a directory item."));
                }
                else
                {
                    // No, so give them the stream.
                    
                    // shell32 v5 will display progress dialogs, but we need to
                    // display progress dialogs for shell32 v3 or v4.  We do this
                    // by creating the progress dialog when the caller asks for the
                    // first stream.  We then need to find out when they call for
                    // the last stream and then hand off the IProgressDialog to the
                    // CFtpStm.  The CFtpStm will then close down the dialog when the
                    // caller closes it.
                    hr = CFtpStm_Create(m_pfd, pidl, GENERIC_READ, &pstg->pstm, m_uliCompleted, m_uliTotal, m_ppd, (pfe->lindex == m_nEndIndex));
                    EVAL(SUCCEEDED(_DoProgressForLegacySystemsPost(pidl, (pfe->lindex == m_nEndIndex))));
                }
            }
            else
            {
                // The user may have cancelled
                ASSERT(HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr);
            }
        }

        if (FAILED(hr))
            _CloseProgressDialog();
    }

    //TraceMsg(TF_FTPDRAGDROP, "CFtpObj::GetData() CFtpStm_Create() returned hr=%#08lx", hr);
    return hr;
}


/*****************************************************************************\
    FUNCTION: IsEqualFORMATETC
  
    DESCRIPTION:
        The two fields of a FORMATETC that need to match to be equivalent are:
    cfFormat and lindex.
\*****************************************************************************/
BOOL IsEqualFORMATETC(FORMATETC * pfe1, FORMATETC * pfe2)
{
    BOOL fIsEqual = FALSE;

    if ((pfe1->cfFormat == pfe2->cfFormat) && (pfe1->lindex == pfe2->lindex))
    {
        fIsEqual = TRUE;
    }

    return fIsEqual;
}


/*****************************************************************************\
      FUNCTION: _FreeExtraData
  
      DESCRIPTION:
\*****************************************************************************/
int CFtpObj::_DSA_FreeCB(LPVOID pvItem, LPVOID pvlparam)
{
    FORMATETC_STGMEDIUM * pfs = (FORMATETC_STGMEDIUM *) pvItem;

    if (EVAL(pfs))
        ReleaseStgMedium(&(pfs->medium));

    return 1;
}


/*****************************************************************************\
      FUNCTION: _FindSetDataIndex
  
      DESCRIPTION:
\*****************************************************************************/
int CFtpObj::_FindExtraDataIndex(FORMATETC *pfe)
{
    int nIndex;

    for (nIndex = (DSA_GetItemCount(m_hdsaSetData) - 1); nIndex >= 0; nIndex--)
    {
        FORMATETC_STGMEDIUM * pfs = (FORMATETC_STGMEDIUM *) DSA_GetItemPtr(m_hdsaSetData, nIndex);

        if (IsEqualFORMATETC(pfe, &pfs->formatEtc))
        {
            return nIndex;
        }
    }

    return -1;
}


/*****************************************************************************\
      FUNCTION: _SetExtraData
  
      DESCRIPTION:
        We don't render the data, but we will carry it because someone may need
      or want it.  This is the case with the drag source's defview pushing in
      the icon points via CFSTR_SHELLIDLISTOFFSET for the drop target.
\*****************************************************************************/
HRESULT CFtpObj::_SetExtraData(FORMATETC *pfe, STGMEDIUM *pstg, BOOL fRelease)
{
    HRESULT hr;
    int nIndex = _FindExtraDataIndex(pfe);

    // Do we already have someone's copy?
    if (-1 == nIndex)
    {
        FORMATETC_STGMEDIUM fs;

        fs.formatEtc = *pfe;

        // If there is a pointer, copy the data because we can't maintain the lifetime
        // of the pointer.
        if (fs.formatEtc.ptd)
        {
            fs.dvTargetDevice = *(pfe->ptd);
            fs.formatEtc.ptd = &fs.dvTargetDevice;
        }

        hr = CopyStgMediumWrap(pstg, &fs.medium);
        if (EVAL(SUCCEEDED(hr)))
        {
            // No, so just append it to the end.
            DSA_AppendItem(m_hdsaSetData, &fs);
        }
    }
    else
    {
        FORMATETC_STGMEDIUM fs;

        DSA_GetItem(m_hdsaSetData, nIndex, &fs);
        // Free the previous guy.
        ReleaseStgMedium(&fs.medium);

        // Yes, so Replace it.
        hr = CopyStgMediumWrap(pstg, &fs.medium);
        if (EVAL(SUCCEEDED(hr)))
        {
            // Replace the data.
            DSA_SetItem(m_hdsaSetData, nIndex, &fs);
        }
    }

    return hr;
}


typedef struct
{
    DWORD dwVersion;
    DWORD dwExtraSize;   // After pidl list
    BOOL fFGDRendered;
    DWORD dwReserved1;
    DWORD dwReserved2;
} FTPDATAOBJ_PERSISTSTRUCT;


/*****************************************************************************\
    FUNCTION: FormatEtcSaveToStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT FormatEtcSaveToStream(IStream *pStm, FORMATETC * pFormatEtc)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        // We don't support ptd because where would the allocation be
        // on the load?
        if (EVAL(NULL == pFormatEtc->ptd))
        {
            WCHAR szFormatName[MAX_PATH];

            if (EVAL(GetClipboardFormatNameW(pFormatEtc->cfFormat, szFormatName, ARRAYSIZE(szFormatName))))
            {
                DWORD cbFormatNameSize = ((lstrlenW(szFormatName) + 1) * sizeof(szFormatName[0]));

                hr = pStm->Write(pFormatEtc, SIZEOF(*pFormatEtc), NULL);
                if (EVAL(SUCCEEDED(hr)))
                {
                    hr = pStm->Write(&cbFormatNameSize, SIZEOF(cbFormatNameSize), NULL);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        hr = pStm->Write(szFormatName, cbFormatNameSize, NULL);
                    }
                }
            }
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FormatEtcLoadFromStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT FormatEtcLoadFromStream(IStream *pStm, FORMATETC * pFormatEtc)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        hr = pStm->Read(pFormatEtc, SIZEOF(*pFormatEtc), NULL);
        ASSERT(NULL == pFormatEtc->ptd);    // We don't support this.

        if (EVAL(SUCCEEDED(hr)))
        {
            DWORD cbFormatNameSize;

            hr = pStm->Read(&cbFormatNameSize, SIZEOF(cbFormatNameSize), NULL);
            if (EVAL(SUCCEEDED(hr)))
            {
                WCHAR szFormatName[MAX_PATH];

                hr = pStm->Read(szFormatName, cbFormatNameSize, NULL);
                if (EVAL(SUCCEEDED(hr)))
                {
                    pFormatEtc->cfFormat = (CLIPFORMAT)RegisterClipboardFormatW(szFormatName);
                }
            }
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

    }

    return hr;
}


typedef struct
{
    DWORD dwVersion;
    DWORD dwExtraSize;               // After this struct
    DWORD dwTymed;              // What type of data is stored?
    BOOL fUnkForRelease;        // Did we save the object after this?
    DWORD dwReserved1;          //
    DWORD dwReserved2;          //
} STGMEDIUM_PERSISTSTRUCT;

/*****************************************************************************\
    FUNCTION: StgMediumSaveToStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT StgMediumSaveToStream(IStream *pStm, STGMEDIUM * pMedium)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        STGMEDIUM_PERSISTSTRUCT smps = {0};

        smps.dwVersion = 1;
        smps.dwTymed = pMedium->tymed;

        switch (pMedium->tymed)
        {
        case TYMED_HGLOBAL:
        {
            IStream * pstmHGlobal;

            hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &pstmHGlobal);
            if (EVAL(SUCCEEDED(hr)))
            {
                STATSTG statStg;

                hr = pstmHGlobal->Stat(&statStg, STATFLAG_NONAME);
                if (EVAL(SUCCEEDED(hr)))
                {
                    ASSERT(!statStg.cbSize.HighPart);
                    smps.dwExtraSize = statStg.cbSize.LowPart;
                    hr = pStm->Write(&smps, SIZEOF(smps), NULL);
                    if (EVAL(SUCCEEDED(hr)))
                        hr = pstmHGlobal->CopyTo(pStm, statStg.cbSize, NULL, NULL);
                }

                pstmHGlobal->Release();
            }
        }
        break;

        case TYMED_FILE:
            smps.dwExtraSize = ((lstrlenW(pMedium->lpszFileName) + 1) * sizeof(WCHAR));

            hr = pStm->Write(&smps, SIZEOF(smps), NULL);
            if (EVAL(SUCCEEDED(hr)))
            {
                hr = pStm->Write(pMedium->lpszFileName, smps.dwExtraSize, NULL);
                ASSERT(SUCCEEDED(hr));
            }
            break;

        case TYMED_GDI:
        case TYMED_MFPICT:
        case TYMED_ENHMF:
        case TYMED_ISTORAGE:
        case TYMED_ISTREAM:
        default:
            ASSERT(0);  // What are you doing?  Impl this if you need it.
            hr = E_NOTIMPL;
            break;
        }
    }

    return hr;
}


LPWSTR OLESTRAlloc(DWORD cchSize)
{
    return (LPWSTR) new WCHAR [cchSize + 1];
}


/*****************************************************************************\
    FUNCTION: StgMediumLoadFromStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT StgMediumLoadFromStream(IStream *pStm, STGMEDIUM * pMedium)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm && pMedium)
    {
        STGMEDIUM_PERSISTSTRUCT smps;

        pMedium->pUnkForRelease = NULL;
        hr = pStm->Read(&smps, SIZEOF(smps), NULL);
        if (EVAL(SUCCEEDED(hr)))
        {
            pMedium->tymed = smps.dwTymed;
            ASSERT(!pMedium->pUnkForRelease);

            switch (pMedium->tymed)
            {
            case TYMED_HGLOBAL:
            {
                IStream * pstmTemp;
                hr = CreateStreamOnHGlobal(NULL, FALSE, &pstmTemp);
                if (EVAL(SUCCEEDED(hr)))
                {
                    ULARGE_INTEGER uli = {0};

                    uli.LowPart = smps.dwExtraSize;
                    hr = pStm->CopyTo(pstmTemp, uli, NULL, NULL);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        hr = GetHGlobalFromStream(pstmTemp, &pMedium->hGlobal);
                    }

                    pstmTemp->Release();
                }
            }
            break;

            case TYMED_FILE:
                pMedium->lpszFileName = OLESTRAlloc(smps.dwExtraSize / sizeof(WCHAR));
                if (pMedium->lpszFileName)
                    hr = pStm->Read(pMedium->lpszFileName, smps.dwExtraSize, NULL);
                else
                    hr = E_OUTOFMEMORY;
                break;

            case TYMED_GDI:
            case TYMED_MFPICT:
            case TYMED_ENHMF:
            case TYMED_ISTORAGE:
            case TYMED_ISTREAM:
            default:
                ASSERT(0);  // What are you doing?  Impl this if you need it.
                // Some future version must have done the save, so skip the
                // data so we don't leave unread data.
                if (0 != smps.dwExtraSize)
                {
                    LARGE_INTEGER li = {0};

                    li.LowPart = smps.dwExtraSize;
                    EVAL(SUCCEEDED(pStm->Seek(li, STREAM_SEEK_CUR, NULL)));
                }
                hr = E_NOTIMPL;
                break;
            }
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FORMATETC_STGMEDIUMSaveToStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT FORMATETC_STGMEDIUMSaveToStream(IStream *pStm, FORMATETC_STGMEDIUM * pfdops)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        hr = FormatEtcSaveToStream(pStm, &pfdops->formatEtc);
        if (EVAL(SUCCEEDED(hr)))
            hr = StgMediumSaveToStream(pStm, &pfdops->medium);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FORMATETC_STGMEDIUMLoadFromStream

    DESCRIPTION:
\*****************************************************************************/
HRESULT FORMATETC_STGMEDIUMLoadFromStream(IStream *pStm, FORMATETC_STGMEDIUM * pfdops)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        hr = FormatEtcLoadFromStream(pStm, &pfdops->formatEtc);
        if (EVAL(SUCCEEDED(hr)))
            hr = StgMediumLoadFromStream(pStm, &pfdops->medium);
    }

    return hr;
}



/////////////////////////////////
////// IAsynchDataObject Impl
/////////////////////////////////


/*****************************************************************************\
    FUNCTION: IAsyncOperation::GetAsyncMode

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::GetAsyncMode(BOOL * pfIsOpAsync)
{
    *pfIsOpAsync = TRUE;
    return S_OK;
}
  

/*****************************************************************************\
    FUNCTION: IAsyncOperation::StartOperation

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::StartOperation(IBindCtx * pbcReserved)
{
    ASSERT(!pbcReserved);
    m_fDidAsynchStart = TRUE;

    return S_OK;
}
  

/*****************************************************************************\
    FUNCTION: IAsyncOperation::InOperation

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::InOperation(BOOL * pfInAsyncOp)
{
    if (m_fDidAsynchStart)
        *pfInAsyncOp = TRUE;
    else
        *pfInAsyncOp = FALSE;

    return S_OK;
}
  

/*****************************************************************************\
    FUNCTION: IAsyncOperation::EndOperation

    DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::EndOperation(HRESULT hResult, IBindCtx * pbcReserved, DWORD dwEffects)
{
    if (SUCCEEDED(hResult) &&
        (DROPEFFECT_MOVE == dwEffects))
    {
        CFtpPidlList * pPidlListNew = CreateRelativePidlList(m_pff, m_pflHfpl);

        if (pPidlListNew)
        {
            Misc_DeleteHfpl(m_pff, GetDesktopWindow(), pPidlListNew);
            pPidlListNew->Release();
        }
    }
 
    m_fDidAsynchStart = FALSE;
    return S_OK;
}
  


/////////////////////////////////
////// IPersistStream Impl
/////////////////////////////////


/*****************************************************************************\
    FUNCTION: IPersistStream::Load

    DESCRIPTION:
        See IPersistStream::Save() for the layout of the stream.
\*****************************************************************************/
HRESULT CFtpObj::Load(IStream *pStm)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        FTPDATAOBJ_PERSISTSTRUCT fdoss;
        DWORD dwNumPidls;
        DWORD dwNumStgMedium;

        hr = pStm->Read(&fdoss, SIZEOF(fdoss), NULL);   // #1
        // If we rev the version, read it now (fdoss.dwVersion)

        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidl = NULL;       // ILLoadFromStream frees the param

            ASSERT(!m_pff);
            m_fFGDRendered = fdoss.fFGDRendered;

            hr = ILLoadFromStream(pStm, &pidl); // #2
            if (EVAL(SUCCEEDED(hr)))
            {
                hr = SHBindToIDList(pidl, NULL, IID_CFtpFolder, (void **)&m_pff);
                if (EVAL(SUCCEEDED(hr)))
                    m_pfd = m_pff->GetFtpDir();

                ASSERT(m_pfd);
                ILFree(pidl);
            }
        }

        if (EVAL(SUCCEEDED(hr)))
        {
            hr = pStm->Read(&dwNumPidls, SIZEOF(dwNumPidls), NULL);  // #3
            if (EVAL(SUCCEEDED(hr)))
                hr = CFtpPidlList_Create(0, NULL, &m_pflHfpl);
        }

        if (EVAL(SUCCEEDED(hr)))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumPidls) && SUCCEEDED(hr); nIndex++)
            {
                LPITEMIDLIST pidl = NULL;       // ILLoadFromStream frees the param

                hr = ILLoadFromStream(pStm, &pidl); // #4
                if (EVAL(SUCCEEDED(hr)))
                {
                    hr = m_pflHfpl->InsertSorted(pidl);
                    ILFree(pidl);
                }
            }
        }

        if (EVAL(SUCCEEDED(hr)))
            hr = pStm->Read(&dwNumStgMedium, SIZEOF(dwNumStgMedium), NULL);  // #5

        if (EVAL(SUCCEEDED(hr)))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumStgMedium) && SUCCEEDED(hr); nIndex++)
            {
                FORMATETC_STGMEDIUM fs;

                hr = FORMATETC_STGMEDIUMLoadFromStream(pStm, &fs);   // #6
                if (EVAL(SUCCEEDED(hr)))
                    DSA_AppendItem(m_hdsaSetData, &fs);
            }
        }

        if (EVAL(SUCCEEDED(hr)))
        {
            // We may be reading a version newer than us, so skip their data.
            if (0 != fdoss.dwExtraSize)
            {
                LARGE_INTEGER li = {0};
                
                li.LowPart = fdoss.dwExtraSize;
                hr = pStm->Seek(li, STREAM_SEEK_CUR, NULL);
            }
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IPersistStream::Save

    DESCRIPTION:
        The stream will be layed out in the following way:

    Version 1:
        1. FTPDATAOBJ_PERSISTSTRUCT - Constant sized data.
        <PidlList BEGIN>
            2. PIDL pidl - Pidl for m_pff.  It will be a public pidl (fully qualified
                        from the shell root)
            3. DWORD dwNumPidls - Number of pidls coming.
            4. PIDL pidl(n) - Pidl in slot (n) of m_pflHfpl
        <PidlList END>
        5. DWORD dwNumStgMedium - Number of FORMATETC_STGMEDIUMs coming
        6. FORMATETC_STGMEDIUM fmtstg(n) - dwNumStgMedium FORMATETC_STGMEDIUMs.
\*****************************************************************************/
HRESULT CFtpObj::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pStm)
    {
        FTPDATAOBJ_PERSISTSTRUCT fdoss = {0};
        DWORD dwNumPidls = m_pflHfpl->GetCount();
        DWORD dwNumStgMedium = DSA_GetItemCount(m_hdsaSetData);

        fdoss.dwVersion = 1;
        fdoss.fFGDRendered = m_fFGDRendered;
        hr = pStm->Write(&fdoss, SIZEOF(fdoss), NULL);  // #1
        if (EVAL(SUCCEEDED(hr)))
        {
            ASSERT(m_pff);
            hr = ILSaveToStream(pStm, m_pff->GetPublicRootPidlReference()); // #2
        }

        if (EVAL(SUCCEEDED(hr)))
            hr = pStm->Write(&dwNumPidls, SIZEOF(dwNumPidls), NULL);  // #3

        if (EVAL(SUCCEEDED(hr)))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumPidls) && SUCCEEDED(hr); nIndex++)
            {
                LPITEMIDLIST pidlCur = m_pflHfpl->GetPidl(nIndex);

                ASSERT(pidlCur);
                hr = ILSaveToStream(pStm, pidlCur); // #4
            }
        }

        if (EVAL(SUCCEEDED(hr)))
            hr = pStm->Write(&dwNumStgMedium, SIZEOF(dwNumStgMedium), NULL);  // #5

        if (EVAL(SUCCEEDED(hr)))
        {
            for (int nIndex = 0; (nIndex < (int)dwNumStgMedium) && SUCCEEDED(hr); nIndex++)
            {
                FORMATETC_STGMEDIUM fs;

                DSA_GetItem(m_hdsaSetData, nIndex, &fs);

                hr = FORMATETC_STGMEDIUMSaveToStream(pStm, &fs);   // #6
            }
        }

    }

    return hr;
}


#define MAX_STREAM_SIZE    (500 * 1024) // 500k
/*****************************************************************************\
    FUNCTION: IPersistStream::GetSizeMax

    DESCRIPTION:
        Now this is tough.  I can't calculate the real value because I don't know
    how big the hglobals are going to be for the user provided data.  I will
    assume everything fits in
\*****************************************************************************/
HRESULT CFtpObj::GetSizeMax(ULARGE_INTEGER * pcbSize)
{
    if (pcbSize)
    {
        pcbSize->HighPart = 0;
        pcbSize->LowPart = MAX_STREAM_SIZE;
    }
    
    return E_NOTIMPL;
}


/////////////////////////////////
////// IDataObject Impl
/////////////////////////////////

/*****************************************************************************\
    FUNCTION: IDataObject::GetData

    DESCRIPTION:
        Render the data in the requested format and put it into the
    STGMEDIUM structure.
\*****************************************************************************/
HRESULT CFtpObj::GetData(LPFORMATETC pfe, LPSTGMEDIUM pstg)
{
    int ife;
    HRESULT hr;

    hr = _FindDataForGet(pfe, &ife);
    if (SUCCEEDED(hr))
    {
        if (ife == DROP_FCont)
            hr = _RenderFileContents(pfe, pstg);
        else
        {
            hr = _ForceRender(ife);
            if (SUCCEEDED(hr))  // May not succeed for security reasons.
            {
                ASSERT(m_stgCache[ife].hGlobal);

                // It's possible to use the hacking STGMEDIUM.pUnkForRelease to give away
                // pointers to our data, but we then need massive amounts of code to babysite
                // the lifetime of those pointers.  This becomes more work when ::SetData() can
                // replace that data, so we just take the hit of the memcpy for less code.
                hr = CopyStgMediumWrap(&m_stgCache[ife], pstg);
                ASSERT(SUCCEEDED(hr));
                ASSERT(NULL == pstg->pUnkForRelease);
                //TraceMsg(TF_FTPDRAGDROP, "CFtpObj::GetData() pstg->hGlobal=%#08lx. pstg->pUnkForRelease=%#08lx.", pstg->hGlobal, pstg->pUnkForRelease);
            }
        }

        TraceMsgWithFormat(TF_FTPDRAGDROP, "CFtpObj::GetData()", pfe, "Format in static list", hr);
    }
    else
    {
        int nIndex = _FindExtraDataIndex(pfe);

        if (-1 == nIndex)
            hr = E_FAIL;
        else
        {
            FORMATETC_STGMEDIUM fs;

            DSA_GetItem(m_hdsaSetData, nIndex, &fs);
            hr = CopyStgMediumWrap(&fs.medium, pstg);
        }

        TraceMsgWithFormat(TF_FTPDRAGDROP, "CFtpObj::GetData()", pfe, "Looking in dyn list", hr);
    }

    return hr;
}


/*****************************************************************************\
    IDataObject::GetDataHere

    Render the data in the requested format and put it into the
    object provided by the caller.
\*****************************************************************************/
HRESULT CFtpObj::GetDataHere(FORMATETC *pfe, STGMEDIUM *pstg)
{
    TraceMsg(TF_FTPDRAGDROP, "CFtpObj::GetDataHere() pfe->cfFormat=%d.", pfe->cfFormat);
    return E_NOTIMPL;
}



/*****************************************************************************\
    FUNCTION: IDataObject::QueryGetData

    DESCRIPTION:
       Indicate whether we could provide data in the requested format.
\*****************************************************************************/
HRESULT CFtpObj::QueryGetData(FORMATETC *pfe)
{
    int ife;
    HRESULT hr = _FindDataForGet(pfe, &ife);
    
    if (FAILED(hr))
    {
        // If it wasn't one of the types we offer, see if it was given to us via
        // IDataObject::SetData().
        int nIndex = _FindExtraDataIndex(pfe);

        if (-1 != nIndex)
            hr = S_OK;
    }

    TraceMsgWithFormat(TF_FTPDRAGDROP, "CFtpObj::QueryGetData()", pfe, "", hr);
    return hr;
}


/*****************************************************************************\
      FUNCTION: IDataObject::GetCanonicalFormatEtc
  
      DESCRIPTION:
      Our data are not sensitive to device-specific renderings,
      so we do what the book tells us to do.
 
      Or we *try* to do what the book tells us to do.
 
      OLE random documentation of the day:
      IDataObject::GetCanonicalFormatEtc.
 
      Turns out that the man page contradicts itself within sentences:
 
         DATA_S_SAMEFORMATETC - The FORMATETC structures are the same
                    and NULL is returned in pfeOut.
 
         If the data object never provides device-specific renderings,
         the implementation of IDataObject::GetCanonicalFormatEtc
         simply copies the input FORMATETC to the output FORMATETC,
         stores a null in the ptd field, and returns DATA_S_SAMEFORMATETC.
 
      And it turns out that the shell doesn't do *either* of these things.
      It just returns DATA_S_SAMEFORMATETC and doesn't touch pfeOut.
 
      The book is even more confused.  Under pfeOut, it says
 
         The value is NULL if the method returns DATA_S_SAMEFORMATETC.
 
      This makes no sense.  The caller provides the value of pfeOut.
      How can the caller possibly know that the method is going to return
      DATA_S_SAMEFORMATETC before it calls it?  If you expect the
      method to write "pfeOut = 0" before returning, you're nuts.  That
      communicates nothing to the caller.
 
      I'll just do what the shell does.
\*****************************************************************************/
HRESULT CFtpObj::GetCanonicalFormatEtc(FORMATETC *pfeIn, FORMATETC *pfeOut)
{
    return DATA_S_SAMEFORMATETC;
}


/*****************************************************************************\
      FUNCTION: IDataObject::SetData
  
      DESCRIPTION:
      We let people change TYMED_HGLOBAL gizmos, but nothing else.
  
      We need to do a careful two-step when replacing the HGLOBAL.
      If the user gave us a plain HGLOBAL without a pUnkForRelease,
      we need to invent our own pUnkForRelease to track it.  But we
      don't want to release the old STGMEDIUM until we're sure we
      can accept the new one.
  
      fRelease == 0 makes life doubly interesting, because we also
      have to clone the HGLOBAL (and remember to free the clone on the
      error path).
  
      _SOMEDAY_/TODO -- Need to support PerformedDropEffect so we can
      clean up stuff on a cut/paste.
\*****************************************************************************/
HRESULT CFtpObj::SetData(FORMATETC *pfe, STGMEDIUM *pstg, BOOL fRelease)
{
    int ife;
    HRESULT hr;

    hr = _FindData(pfe, &ife);
    if (SUCCEEDED(hr))
    {
        if (ife == DROP_FCont)
        {
            TraceMsg(TF_FTPDRAGDROP, "CFtpObj::SetData(FORMATETC.cfFormat=%d) ife == DROP_FCont", pfe->cfFormat);
            hr = DV_E_FORMATETC;
        }
        else
        {
            ASSERT(g_dropTypes[ife].tymed == TYMED_HGLOBAL);
            ASSERT(pstg->tymed == TYMED_HGLOBAL);
            if (EVAL(pstg->hGlobal))
            {
                STGMEDIUM stg = {0};

                hr = CopyStgMediumWrap(pstg, &stg);
                if (EVAL(SUCCEEDED(hr)))
                {
                    ReleaseStgMedium(&m_stgCache[ife]);
                    m_stgCache[ife] = stg;
                }
            }
            else
            {            // Tried to SetData a _DelayRender
                hr = DV_E_STGMEDIUM;    // You idiot you
            }
        }

        TraceMsgWithFormat(TF_FTPDRAGDROP, "CFtpObj::SetData()", pfe, "in static list", hr);
    }
    else
    {
        hr = _SetExtraData(pfe, pstg, fRelease);
        TraceMsgWithFormat(TF_FTPDRAGDROP, "CFtpObj::SetData()", pfe, "in dyn list", hr);
    }

    return hr;
}


/*****************************************************************************\
      FUNCTION: IDataObject::EnumFormatEtc
  
      DESCRIPTION:
        _UNDOCUMENTED_:  If you drag something from a DefView, it will
      check the data object to see if it has a hida.  If so, then it
      will cook up a CFSTR_SHELLIDLISTOFFSET *for you* and SetData
      the information into the data object.  So in order to get
      position-aware drag/drop working, you must allow DefView to change
      your CFSTR_SHELLIDLISTOFFSET.
 
     We allow all FORMATETCs to be modified except for FileContents.
\*****************************************************************************/
HRESULT CFtpObj::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenum)
{
    HRESULT hres;

    switch (dwDirection)
    {
    case DATADIR_GET:
        hres = CFtpEfe_Create(DROP_OFFERMAX - DROP_FCont, &g_dropTypes[DROP_FCont],
                   &m_stgCache[DROP_FCont], this, ppenum);
        TraceMsg(TF_FTPDRAGDROP, "CFtpObj::EnumFormatEtc(DATADIR_GET) CFtpEfe_Create() returned hres=%#08lx", hres);
        break;

    case DATADIR_SET:
        hres = CFtpEfe_Create(DROP_OFFERMAX - DROP_OFFERMIN, &g_dropTypes[DROP_OFFERMIN],
                   &m_stgCache[DROP_OFFERMIN], NULL, ppenum);
        TraceMsg(TF_FTPDRAGDROP, "CFtpObj::EnumFormatEtc(DATADIR_SET) CFtpEfe_Create() returned hres=%#08lx", hres);
        break;

    default:
        ASSERT(0);
        hres = E_NOTIMPL;
        break;
    }

    return hres;
}


/*****************************************************************************\
      FUNCTION: IDataObject::DAdvise
  
      DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::DAdvise(FORMATETC *pfe, DWORD advfl, IAdviseSink *padv, DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}


/*****************************************************************************\
      FUNCTION: IDataObject::DUnadvise
  
      DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}


/*****************************************************************************\
      FUNCTION: IDataObject::EnumDAdvise
  
      DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj::EnumDAdvise(IEnumSTATDATA **ppeadv)
{
    return OLE_E_ADVISENOTSUPPORTED;
}


/*****************************************************************************\
      FUNCTION: CFtpObj_Create
  
      DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;
    CFtpObj * pfo;

    *ppvObj = NULL;

    hres = CFtpObj_Create(pff, pflHfpl, &pfo);
    if (EVAL(SUCCEEDED(hres)))
    {
        pfo->QueryInterface(riid, ppvObj);
        pfo->Release();
    }

     return hres;
}


/*****************************************************************************\
      FUNCTION: CFtpObj_Create
  
      DESCRIPTION:
\*****************************************************************************/
HRESULT CFtpObj_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, CFtpObj ** ppfo)
{
    HRESULT hres = S_OK;

    if (EVAL(pflHfpl->GetCount()))
    {
        *ppfo = new CFtpObj();

        if (EVAL(*ppfo))
        {
            CFtpObj * pfo = *ppfo;
            pfo->m_pfd = pff->GetFtpDir();

            if (EVAL(pfo->m_pfd))
            {
                pfo->m_pff = pff;
                if (pff)
                    pff->AddRef();

                IUnknown_Set(&pfo->m_pflHfpl, pflHfpl);
                
                if (pfo->m_pflHfpl->GetCount() == 1)
                {
                    pfo->m_stgCache[DROP_URL].tymed = TYMED_HGLOBAL;
                }
            }
            else
            {
                hres = E_FAIL;
                (*ppfo)->Release();
                *ppfo = NULL;
            }
        }
        else
            hres = E_OUTOFMEMORY;

    }
    else
    {
        *ppfo = NULL;
        hres = E_INVALIDARG;        /* Trying to get UI object of nil? */
    }

    return hres;
}


/*****************************************************************************\
    FUNCTION: CFtpObj_Create

    DESCRIPTION:
        This will be called by the Class Factory when the IDataObject gets
    persisted and then wants to be recreated in a new process. (Happens
    after the original thread/process calls OleFlushClipboard.
\*****************************************************************************/
HRESULT CFtpObj_Create(REFIID riid, void ** ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFtpObj * pfo = new CFtpObj();

    *ppvObj = NULL;
    if (pfo)
    {
        hr = pfo->QueryInterface(riid, ppvObj);
        pfo->Release();
    }

     return hr;
}

#define SETDATA_GROWSIZE        3

/****************************************************\
    Constructor
\****************************************************/
CFtpObj::CFtpObj() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pff);
    ASSERT(!m_pfd);
    ASSERT(!m_pflHfpl);
    ASSERT(!m_fDidAsynchStart);

    // NT #245306: If the user drags files from an FTP window (Thread 1)
    //    to a shell window (Thread 2), the shell window will do
    //    the drop on a background thread (thread 3).  Since the
    //    UI thread is no longer blocked, the user can now close
    //    the window.  The problem is that OLE is using Thread 2
    //    for marshalling.  In order to solve this problem, we
    //    ref count the thread for items that rely on it.
    //    This include FTP, normal Download, and other things
    //    in the future.
//    SHIncrementThreadModelessCount();

    m_nStartIndex = -1; // -1 means we don't know the start.
    m_fFGDRendered = FALSE;
    m_fCheckSecurity = TRUE;      // We need to keep checking.

    m_hdsaSetData = DSA_Create(sizeof(FORMATETC_STGMEDIUM), SETDATA_GROWSIZE);

    for (int nIndex = 0; nIndex < ARRAYSIZE(c_stgInit); nIndex++)
    {
        ASSERT(nIndex < ARRAYSIZE(m_stgCache));
        m_stgCache[nIndex] = c_stgInit[nIndex];
    }

    _RefThread();
    // The receiver may use us in the background, so make sure that our thread
    // doesn't go away.
    LEAK_ADDREF(LEAK_CFtpObj);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpObj::~CFtpObj()
{
    int ife;

    _CloseProgressDialog();
    for (ife = DROP_OFFERMIN; ife < DROP_OFFERMAX; ife++)
    {
        ReleaseStgMedium(&m_stgCache[ife]);
    }

    if (m_ppd)
        m_ppd->StopProgressDialog();

    IUnknown_Set((IUnknown **)&m_ppd, NULL);
    IUnknown_Set(&m_pff, NULL);
    IUnknown_Set(&m_pfd, NULL);
    IUnknown_Set(&m_pflHfpl, NULL);

    DSA_DestroyCallback(m_hdsaSetData, &_DSA_FreeCB, NULL);

    // NT #245306: If the user drags files from an FTP window (Thread 1)
    //    to a shell window (Thread 2), the shell window will do
    //    the drop on a background thread (thread 3).  Since the
    //    UI thread is no longer blocked, the user can now close
    //    the window.  The problem is that OLE is using Thread 2
    //    for marshalling.  In order to solve this problem, we
    //    ref count the thread for items that rely on it.
    //    This include FTP, normal Download, and other things
    //    in the future.
    ATOMICRELEASE(m_punkThreadRef);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpObj);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpObj::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpObj::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CFtpObj::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFtpObj, IDataObject),
        QITABENT(CFtpObj, IInternetSecurityMgrSite),
        QITABENT(CFtpObj, IPersist),
        QITABENT(CFtpObj, IPersistStream),
        QITABENT(CFtpObj, IAsyncOperation),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}
