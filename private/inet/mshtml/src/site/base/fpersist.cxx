//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994
//
//  File:       persist.cxx
//
//  Contents:   Contains the saving and loading code for the form
//
//  Classes:    CDoc (partial)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif


#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"   // for databinding
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"  // for _pimganim
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_URLCOMP_HXX_
#define X_URLCOMP_HXX_
#include "urlcomp.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_WINCRYPT_H_
#define X_WINCRYPT_H_
#include "wincrypt.h"
#endif

#ifndef X_MIMEOLE_H_
#define X_MIMEOLE_H_
#define _MIMEOLE_  // To avoid having DECLSPEC_IMPORT
#include "mimeole.h"
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

#ifndef X_EVENTOBJ_H_
#define X_EVENT_OBJ_H_
#include "eventobj.hxx"
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include <shlobj.h>
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_SHFOLDER_HXX_
#define X_SHFOLDER_HXX_
#define _SHFOLDER_
#include "shfolder.h"
#endif

#define _hxx_
#include "hedelems.hdl"

#include <platform.h>

// N.B. taken from shlguidp.h of shell enlistment fame
// Consider: include this file instead (but note that this is a private guid)
#define WSZGUID_OPID_DocObjClientSite _T("{d4db6850-5385-11d0-89e9-00a0c90a90ac}")

extern "C" const CLSID CLSID_HTMLPluginDocument;

extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);

extern CCriticalSection    g_csFile;
extern TCHAR               g_achSavePath[];

//+---------------------------------------------------------------
//  Debugging support
//---------------------------------------------------------------

DeclareTag(tagFormP,            "FormPersist", "Form Persistence");
DeclareTag(tagDocRefresh,       "Doc",          "Trace ExecRefresh");
DeclareTag(tagTerminalServer,   "Doc",          "Use Terminal Server mode");
ExternTag(tagMsoCommandTarget);
PerfDbgExtern(tagPushData)
PerfDbgExtern(tagPerfWatch)

MtDefine(LoadInfo, Dwn, "LoadInfo")
MtDefine(CDocSaveToStream_aryElements_pv, Locals, "CDoc::SaveToStream aryElements::_pv")
MtDefine(CDocSaveSnapShotDocument_aryElements_pv, Locals, "CDoc::SaveSnapShotDocument aryElements::_pv")
MtDefine(NewDwnCtx, Dwn, "CDoc::NewDwnCtx")

#if DBG==1
static void
DebugSetTerminalServer()
{
    static int s_Real_fTerminalServer = -2;

    if (s_Real_fTerminalServer == -2)
    {
        s_Real_fTerminalServer = g_fTerminalServer;
    }

    g_fTerminalServer = IsTagEnabled(tagTerminalServer) ? TRUE
                                                        : s_Real_fTerminalServer;
}
#endif

//+---------------------------------------------------------------
//  Sturcture used in creating a desktop item.
//---------------------------------------------------------------

typedef struct {
    LPCTSTR pszUrl;
    HWND hwnd;
    DWORD dwItemType;
    int x;
    int y;
} CREATEDESKITEM;

static HRESULT CreateDesktopItem(LPCTSTR pszUrl, HWND hwnd, DWORD dwItemType, int x, int y);
MtDefine(SetAsDesktopItem, Utilities, "Set As Desktop Item...")

//+---------------------------------------------------------------
//
//   IsGlobalOffline
//
// Long term - This should call UrlQueryInfo so that wininet is
// not loaded unless needed
//
//---------------------------------------------------------------
extern BOOL IsGlobalOffline(void);

// IEUNIX: Needs to know filter type and save file with filter type.
#ifdef UNIX
//
// I'm taking this out temporarily during my merge.
// To coordinate with steveshi about putting it back - v-olegsl.
//
#if 0 // Move it back temporary. #if 1
#define MwFilterType(pIn, bSet) ""
#else
extern "C" char* MwFilterType(char*, BOOL);
#endif
#endif /** UNIX **/

BOOL
IsGlobalOffline(void)
{
    DWORD   dwState = 0, dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;
    HMODULE hModuleHandle = GetModuleHandleA("wininet.dll");

    if(!hModuleHandle)
        return FALSE;

    // Call InternetQueryOption when INTERNET_OPTION_LINE_STATE
    // implemented in WININET.
    if(InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState,
        &dwSize))
    {
        if(dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            fRet = TRUE;
    }

    return fRet;
}

static BOOL
IsNotConnected()
{
#ifdef WIN16
    //BUGWIN16: Ned to implement the new wininet APIs
    return FALSE;
#else
    DWORD dwConnectedStateFlags;

    return(     !InternetGetConnectedState(&dwConnectedStateFlags, 0)       // Not connected
            &&  !(dwConnectedStateFlags & INTERNET_CONNECTION_MODEM_BUSY)); // Not dialed out to another connection
#endif
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsFrameOffline
//
//---------------------------------------------------------------

BOOL
CDoc::IsFrameOffline(DWORD *pdwBindf)
{
    BOOL fIsFrameOffline = FALSE;
    DWORD dwBindf = 0;

    if (_dwLoadf & DLCTL_FORCEOFFLINE)
    {
        fIsFrameOffline = TRUE;
        dwBindf = BINDF_OFFLINEOPERATION;
    }
    else if (_dwLoadf & DLCTL_OFFLINEIFNOTCONNECTED)
    {
        if (IsNotConnected())
        {
            fIsFrameOffline = TRUE;
            dwBindf = BINDF_OFFLINEOPERATION;
        }
        else
            dwBindf = BINDF_GETFROMCACHE_IF_NET_FAIL;
    }

    if (pdwBindf)
        *pdwBindf = dwBindf;

    return fIsFrameOffline;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsOffline
//
//---------------------------------------------------------------

BOOL
CDoc::IsOffline()
{
    // Call InternetQueryOption when INTERNET_OPTION_LINE_STATE
    // implemented in WININET.
    return ((IsFrameOffline()) || (IsGlobalOffline()) );
}

HRESULT
CDoc::SetDirtyFlag(BOOL fDirty)
{
    if( fDirty )
        _lDirtyVersion = MAXLONG;
    else
        _lDirtyVersion = 0;
    
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::IsDirty
//
//  Synopsis:   Method of IPersist interface
//
//  Notes:      We must override this method because we are a container.
//              In addition to normal base processing we must pass this
//              call recursively to our embeddings.
//
//---------------------------------------------------------------

STDMETHODIMP
CDoc::IsDirty(void)
{
    // NOTE: (rodc) Never dirty in browse mode.
    if (!_fDesignMode)
        return S_FALSE;

    if (_lDirtyVersion != 0)
        return S_OK;

    if (PrimaryMarkup())
    {
        CNotification   nf;

        Assert( PrimaryRoot() );

        nf.UpdateDocDirty(PrimaryRoot());
        BroadcastNotify(&nf);
    }

    if (_lDirtyVersion == 0)
        return S_FALSE;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetCurFile, IPersistFile
//
//----------------------------------------------------------------------------

HRESULT
CDoc::GetCurFile(LPOLESTR *ppszFileName)
{
    TCHAR  achFile[MAX_PATH];
    ULONG  cchFile = ARRAY_SIZE(achFile);
    HRESULT hr;

    if (!_cstrUrl || GetUrlScheme(_cstrUrl) != URL_SCHEME_FILE)
        return E_UNEXPECTED;

    hr = THR(PathCreateFromUrl(_cstrUrl, achFile, &cchFile, 0));
    if (hr)
        RRETURN(hr);

    RRETURN(THR(TaskAllocString(achFile, ppszFileName)));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetClassID, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::GetClassID(CLSID *pclsid)
{
    // This method can be deleted if we can make IPersistFile a tearoff interface.
    if (pclsid == NULL)
    {
        RRETURN(E_INVALIDARG);
    }

    if (!_fFullWindowEmbed)
        *pclsid = *BaseDesc()->_pclsid;
    else
        *pclsid = CLSID_HTMLPluginDocument;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetCurMoniker, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::GetCurMoniker(IMoniker **ppmkName)
{
    if (!_pmkName)
        RRETURN(E_UNEXPECTED);

    *ppmkName = _pmkName;
    _pmkName->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Helper:     ReloadAndSaveDocInCodePage
//
//  Synopsis:   Helper to encapsulate reloading a document and saving it.
//              Save the URL pszUrl in the file pszPath, in character set
//              codepage.
//----------------------------------------------------------------------------
static HRESULT
ReloadAndSaveDocInCodePage(LPTSTR pszUrl, LPTSTR pszPath, CODEPAGE codepage,
    CODEPAGE codepageLoad, CODEPAGE codepageLoadURL)
{
    CDoc::LOADINFO LoadInfo = { 0 };
    MSG         msg;
    HRESULT     hr  = S_OK;

    // No container, not window enabled, not MHTML.
    CDoc *pTempDoc = new CDoc(NULL);

    if (!pTempDoc)
        goto Cleanup;

    pTempDoc->Init();

    // Don't execute scripts, we want the original.
    pTempDoc->_dwLoadf |= DLCTL_NO_SCRIPTS | DLCTL_NO_FRAMEDOWNLOAD |
                          DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_CLIENTPULL |
                          DLCTL_SILENT | DLCTL_NO_JAVA | DLCTL_DOWNLOADONLY;

    hr = CreateURLMoniker(NULL, LPTSTR(pszUrl), &LoadInfo.pmk);
    if (hr || !LoadInfo.pmk)
        goto Cleanup;

    LoadInfo.pchDisplayName = pszUrl;
    LoadInfo.codepageURL = codepageLoadURL;
    LoadInfo.codepage = codepageLoad;

    hr = pTempDoc->LoadFromInfo(&LoadInfo);
    if (hr)
        goto Cleanup;

    // BUGBUG Arye: I don't like having a message loop here,
    // Neither does Dinarte, nor Gary. But no one has a better
    // idea that can be implemented in a reasonable length of
    // time, so we'll do this for now so that Save As ... is a
    // synchronous operation.
    //
    // Process messages until we've finished loading.

    for (;;)
    {
        GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (pTempDoc->LoadStatus() >= LOADSTATUS_PARSE_DONE)
            break;
    }

    pTempDoc->_codepage = codepage;         // codepage of the doc
    pTempDoc->_codepageFamily = WindowsCodePageFromCodePage(codepage); // family codepage of the doc
    pTempDoc->_fDesignMode = TRUE;          // force to save from the tree.

    IGNORE_HR(pTempDoc->Save(pszPath, FALSE));

Cleanup:
    ReleaseInterface(LoadInfo.pmk);

    if (pTempDoc)
    {
        pTempDoc->Close(OLECLOSE_NOSAVE);
        pTempDoc->Release();
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PromptSave
//
//  Synopsis:   This function saves the document to a file name selected by
//              displaying a dialog. It changes current document name to given
//              name.
//
//              BUGBUG: for this version, Save As will not change the
//              current filename.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PromptSave(BOOL fSaveAs, BOOL fShowUI /* = TRUE */, TCHAR * pchPathName /* = NULL */)
{
    HRESULT                 hr = S_OK;
    TCHAR                   achPath[MAX_PATH];
    TCHAR *                 pchPath;
    CElement *              pElem;
    PARSEDURL               puw = {0};
    int                     cchUrl;
    TCHAR *                 pchQuery;
    CCollectionCache *      pCollectionCache;

    Assert(fShowUI || (!fShowUI && pchPathName));

    // Save image files as native file type
    if (_fImageFile)
    {
        Assert(fSaveAs); // image files are not editable
        Assert(_pPrimaryMarkup);

        // Locate the image element
        hr = THR(_pPrimaryMarkup->EnsureCollectionCache(CMarkup::IMAGES_COLLECTION));
        if (hr)
            goto Cleanup;

        pCollectionCache = _pPrimaryMarkup->CollectionCache();
        Assert(pCollectionCache);

        // We must have exactly one image in this document
        if (1 != pCollectionCache->SizeAry(CMarkup::IMAGES_COLLECTION))
        {
            Assert(FALSE);
            goto Cleanup;
        }

        hr = THR(pCollectionCache->GetIntoAry(CMarkup::IMAGES_COLLECTION, 0, &pElem));
        if (hr)
            goto Cleanup;
        Assert(pElem->Tag() != ETAG_INPUT);
        hr = THR(DYNCAST(CImgElement, pElem)->_pImage->PromptSaveAs(achPath, MAX_PATH));
        if (hr)
            goto Cleanup;
        pchPath = achPath;
    }
    else
    {
        CODEPAGE codepage = GetCodePage();

        if (fSaveAs)
        {
            if (!fShowUI)
            {
                pchPath = pchPathName;
            }
            else
            {
                achPath[0] = 0;

                if (pchPathName)
                {
                    _tcsncpy(achPath, pchPathName, MAX_PATH);
                    achPath[MAX_PATH - 1] = 0;
                }
                else
                {
                    // Get file name from _cstrUrl
                    // Note that location is already stripped out from the URL
                    // before it is copied into _cstrUrl. If _cstrUrl ends in '/',
                    // we assume that it does not have a file name at the end
                    if (_cstrUrl)
                    {
                        cchUrl = _cstrUrl.Length();
                        if (cchUrl && _cstrUrl[cchUrl - 1] != _T('/'))
                        {
                            puw.cbSize = sizeof(PARSEDURL);
                            if (SUCCEEDED(ParseURL(_cstrUrl, &puw)))
                            {
                                // Temporarily, null out the '?' in the url
                                pchQuery = _tcsrchr(puw.pszSuffix, _T('?'));
                                if (pchQuery)
                                    *pchQuery = 0;
                                _tcsncpy(achPath, PathFindFileName(puw.pszSuffix), MAX_PATH);
                                if (pchQuery)
                                    *pchQuery = _T('?');

                                achPath[MAX_PATH - 1] = 0;
                            }
                        }
                    }
                }
                if (!achPath[0])
                {
                    LoadString(GetResourceHInst(), IDS_UNTITLED_MSHTML,
                               achPath, ARRAY_SIZE(achPath));
                }

                {
                    CDoEnableModeless   dem(this);

                    if (dem._hwnd)
                    {
                        hr = RequestSaveFileName(achPath, ARRAY_SIZE(achPath), &codepage);
                    }

                    if ( hr || !dem._hwnd)
                        goto Cleanup;
                }
                
                pchPath = achPath;
            }
        }
        else
        {
            pchPath = NULL;
        }

        if (GetCodePage() != codepage && fSaveAs)
        {
            if (_fDesignMode)
            {
                // BUGBUG (johnv) This is a bit messy.  CDoc::Save should
                // take a codepage parameter.
                // Save from the tree, but in a different codepage.
                CODEPAGE codepageInitial = GetCodePage();
                CODEPAGE codepageFamilyInitial = GetFamilyCodePage();

                _codepage = codepage;
                _codepageFamily = WindowsCodePageFromCodePage(codepage);
                hr = THR(Save(pchPath, _fDesignMode));
                _codepage = codepageInitial;
                _codepageFamily = codepageFamilyInitial;
                IGNORE_HR(UpdateCodePageMetaTag(codepageInitial));

                if (hr)
                    goto Cleanup;
            }
            else
            {
                // Create a separate document, loading it in without
                // actually running any scripts.
                hr = THR(ReloadAndSaveDocInCodePage (LPTSTR(_cstrUrl),
                    pchPath, codepage, GetCodePage(), GetURLCodePage()));

                if (hr)
                    goto Cleanup;
            }

        }
        else
        {
            hr = THR(Save(pchPath, _fDesignMode));
            if (hr)
                goto Cleanup;
        }
    }
    hr = THR(SaveCompleted(pchPath));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PromptSaveImgCtx
//
//----------------------------------------------------------------------------

HRESULT
CDoc::PromptSaveImgCtx(const TCHAR * pchCachedFile, MIMEINFO * pmi,
                       TCHAR * pchFileName, int cchFileName)
{
    HRESULT hr;
    TCHAR * pchFile;
    TCHAR * pchFileExt;
    int     idFilterRes = IDS_SAVEPICTUREAS_ORIGINAL;

    Assert(pchCachedFile);

    // If there is no save directory then save
    // to the "My Pictures" Dir.
    {
        // NB: to keep from eating up too much stack, I'm going
        // to temporarily use the pchFileName buffer that is passed
        // in to construct the "My Pictures" dir string.

        // Also if the directory doesn't exist, then just leave g_achSavePath
        // NULL and we will default to the desktop

        LOCK_SECTION(g_csFile);

        if (!*g_achSavePath)
        {
            hr = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, 0, pchFileName);
            if (hr == S_OK && PathFileExists(pchFileName))
            {
                _tcscpy(g_achSavePath, pchFileName);
            }
        }
    }

    pchFile = _tcsrchr(pchCachedFile, _T(FILENAME_SEPARATOR));
    if (pchFile && *pchFile)
        _tcsncpy(pchFileName, ++pchFile, cchFileName - 1);
    else
        _tcsncpy(pchFileName, pchCachedFile, cchFileName - 1);
    pchFileName[cchFileName - 1] = _T('\0');

#ifndef UNIX // UNIX needs extension name if available.
    pchFileExt = _tcsrchr(pchFileName, _T('.'));
    if (pchFileExt)
        *pchFileExt = _T('\0');
#endif

    if (pmi && pmi->ids)
        idFilterRes = pmi->ids;

    {
        CDoEnableModeless   dem(this);
        
        if (dem._hwnd)
        {
            hr = FormsGetFileName(TRUE,  // indicates SaveFileName
                                  dem._hwnd,
                                  idFilterRes,
                                  pchFileName,
                                  cchFileName, (LPARAM)0);
        }
        else
            hr = E_FAIL;
    }
    
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveImgCtxAs
//
//----------------------------------------------------------------------------

void
CDoc::SaveImgCtxAs(CImgCtx *    pImgCtx,
                   CBitsCtx *   pBitsCtx,
                   int          iAction,
                   TCHAR *      pchFileName /*=NULL*/,
                   int          cchFileName /*=0*/)
{
#ifndef WINCE
    HRESULT hr = S_OK;
    TCHAR achPath1[MAX_PATH];
    TCHAR achPath2[MAX_PATH];
    TCHAR * pchPathSrc = NULL;
    TCHAR * pchPathDst = NULL;
    TCHAR * pchAlloc = NULL;
    int idsDefault;
    BOOL fSaveAsBmp;

    if (pBitsCtx)
    {
        idsDefault = IDS_ERR_SAVEPICTUREAS;

        hr = pBitsCtx->GetFile(&pchAlloc);
        if (hr)
            goto Cleanup;

        hr = PromptSaveImgCtx(pchAlloc, pBitsCtx->GetMimeInfo(), achPath1, ARRAY_SIZE(achPath1));
        if (hr)
            goto Cleanup;

        pchPathSrc = pchAlloc;
        pchPathDst = achPath1;
        fSaveAsBmp = FALSE;
    }
    else if (pImgCtx)
    {
        switch(iAction)
        {
            case IDM_SETWALLPAPER:
                idsDefault = IDS_ERR_SETWALLPAPER;

                hr = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, achPath2);
                if (hr)
                    goto Cleanup;

                _tcscat(achPath2, _T("\\Microsoft"));
                if (!PathFileExists(achPath2) && !CreateDirectory(achPath2, NULL))
                {
                    hr = GetLastWin32Error();
                    goto Cleanup;
                }
                _tcscat(achPath2, _T("\\Internet Explorer"));
                if (!PathFileExists(achPath2) && !CreateDirectory(achPath2, NULL))
                {
                    hr = GetLastWin32Error();
                    goto Cleanup;
                }

                hr = Format(0, achPath1, ARRAY_SIZE(achPath1), MAKEINTRESOURCE(IDS_WALLPAPER_BMP), achPath2);
                if (hr)
                    goto Cleanup;

                pchPathDst = achPath1;
                fSaveAsBmp = TRUE;
                break;

            case IDM_SETDESKTOPITEM:
            {
                LPCTSTR lpszURL = pImgCtx->GetUrl();

                idsDefault = IDS_ERR_SETDESKTOPITEM;
                fSaveAsBmp = FALSE;
                if(lpszURL)
                    hr = CreateDesktopItem( lpszURL,
                                            //(_pInPlace ? _pInPlace->_hwnd : GetDesktopWindow()), //Parent window for the UI dialog boxes
                                            GetDesktopWindow(), // If you set _pInPlace->hwnd as the parent, it gets disabled.
                                            COMP_TYPE_PICTURE,          // Desktop item type is IMG.
                                            COMPONENT_DEFAULT_LEFT,     // Default position of top left corner
                                            COMPONENT_DEFAULT_TOP );    // Default position of top
                else
                    hr = E_FAIL;

                if(hr)
                    goto Cleanup;
                break;
            }

            case IDM_SAVEPICTURE:
            default:
                MIMEINFO * pmi = pImgCtx->GetMimeInfo();

                idsDefault = IDS_ERR_SAVEPICTUREAS;

                hr = pImgCtx->GetFile(&pchAlloc);
                if (hr)
                {
                    if (!LoadString(GetResourceHInst(), IDS_UNTITLED_BITMAP, achPath2, ARRAY_SIZE(achPath2)))
                    {
                        hr = GetLastWin32Error();
                        goto Cleanup;
                    }

                    pmi = GetMimeInfoFromMimeType(CFSTR_MIME_BMP);
                    fSaveAsBmp = TRUE;
                    pchPathSrc = achPath2;
                }
                else
                {
                    fSaveAsBmp = FALSE;
                    pchPathSrc = pchAlloc;
                    _tcsncpy(achPath2, pchAlloc, ARRAY_SIZE(achPath2));
                    PathUndecorate(achPath2);
                }

                hr = PromptSaveImgCtx(achPath2, pmi, achPath1, ARRAY_SIZE(achPath1));
                if (hr)
                    goto Cleanup;

                pchPathDst = achPath1;

                if (!fSaveAsBmp)
                {
#ifdef UNIX // IEUNIX uses filter to know save-type
                    fSaveAsBmp = !_strnicmp(".bmp", MwFilterType(NULL, FALSE), 4);
#else
                    TCHAR * pchFileExt = _tcsrchr(pchPathDst, _T('.'));
                    fSaveAsBmp = pchFileExt && _tcsnipre(_T(".bmp"), 4, pchFileExt, -1);
#endif
                }
                break;
        }
    }
    else
    {
        Assert(FALSE);
        return;
    }

    if(iAction != IDM_SETDESKTOPITEM)
    {
        Assert(pchPathDst);
        Assert(fSaveAsBmp || pchPathSrc);

        if (fSaveAsBmp)
        {
            IStream *pStm = NULL;

            hr = THR(CreateStreamOnFile(pchPathDst,
                                    STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
                                    &pStm));
            if (hr)
                goto Cleanup;

            hr = pImgCtx->SaveAsBmp(pStm, TRUE);
            ReleaseInterface(pStm);

            if (hr)
            {
                DeleteFile(pchPathDst);
                goto Cleanup;
            }
        }
        else if (!CopyFile(pchPathSrc, pchPathDst, FALSE))
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        if (pchFileName && cchFileName > 0)
        {
            _tcsncpy(pchFileName, pchPathDst, cchFileName - 1);
            pchFileName[cchFileName - 1] = 0; // _tcsncpy doesn't seem to do this
        }

        if (iAction == IDM_SETWALLPAPER)
        {
            if (!SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, pchPathDst,
                                  SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
            {
                hr = GetLastWin32Error();
                goto Cleanup;
            }
        }
    }

Cleanup:
    MemFreeString(pchAlloc);
    if (FAILED(hr))
    {
        SetErrorInfo(hr);
        THR(ShowLastErrorInfo(hr, idsDefault));
    }
    return;
#endif // WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::RequestSaveFileName
//
//  Synopsis:   Provides the UI for the Save As.. command.
//
//  Arguments:  pszFileName    points to the buffer accepting the file name
//              cchFileName    the size of the buffer
//
//----------------------------------------------------------------------------

HRESULT
CDoc::RequestSaveFileName(LPTSTR pszFileName, int cchFileName, CODEPAGE *pCodePage)
{
    HRESULT  hr;

    Assert(pszFileName && cchFileName);

    hr = FormsGetFileName(TRUE,  // indicates SaveFileName
                          _pInPlace ? _pInPlace->_hwnd : 0,
                          IDS_HTMLFORM_SAVE,
                          pszFileName,
                          cchFileName, (LPARAM)pCodePage);
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     SetFilename
//
//  Synopsis:   Sets the current filename of the document.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetFilename(const TCHAR *pchFile)
{
    TCHAR achUrl[pdlUrlLen];
    ULONG cchUrl = ARRAY_SIZE(achUrl);
    HRESULT hr;

#ifdef WIN16
    Assert(0);
#else
    hr = THR(UrlCreateFromPath(pchFile, achUrl, &cchUrl, 0));
    if (hr)
#endif
        goto Cleanup;

    hr = THR(SetUrl(achUrl));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     SetUrl
//
//  Synopsis:   Sets the current url of the document and updates the moniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetUrl(const TCHAR *pchUrl, BOOL fKeepDwnPost)
{
    IMoniker *pmk = NULL;
    HRESULT hr;

    hr = THR(CreateURLMoniker(NULL, pchUrl, &pmk));
    if (hr)
        goto Cleanup;

    hr = THR(_cstrUrl.Set(pchUrl));
    if (hr)
        goto Cleanup;

    IGNORE_HR(_cstrCOMPAT_OMUrl.Set(_T("")));

    UpdateSecurityID();

    DeferUpdateTitle();

#if DBG == 1
    if (this == GetRootDoc())
    {
        DbgExSetTopUrl(_cstrUrl);
    }
#endif

    ReplaceInterface(&_pmkName, pmk);

    if (!fKeepDwnPost)
        ClearDwnPost();

    MemSetName((this, "CDoc SSN=%d URL=%ls", _ulSSN, (LPTSTR)_cstrUrl));

Cleanup:
    ClearInterface(&pmk);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//   Static : CreateSnapShotDocument
//
//   Synopsis : as the name says...
//
//+-------------------------------------------------------------------------

static HRESULT
CreateSnapShotDocument(CDoc * pSrcDoc, CDoc ** ppSnapDoc)
{
    HRESULT         hr = S_OK;
    IStream       * pstmFile = NULL;
    MSG             msg;
    TCHAR         * pstrFile = NULL;
    CDoc          * pSnapDoc = NULL;

    if (!ppSnapDoc)
        return E_POINTER;
    if (!pSrcDoc)
        return E_INVALIDARG;

    *ppSnapDoc = NULL;

    // No container, not window enabled, not MHTML.
    pSnapDoc = new CDoc(NULL);
    if (!pSnapDoc)
        goto Cleanup;

    pSnapDoc->Init();

    // Don't execute scripts, we want the original.
    pSnapDoc->_dwLoadf |= DLCTL_NO_SCRIPTS | DLCTL_NO_FRAMEDOWNLOAD |
                          DLCTL_NO_CLIENTPULL |
                          DLCTL_SILENT | DLCTL_NO_JAVA | DLCTL_DOWNLOADONLY;

    pSnapDoc->_fDesignMode = TRUE;          // force to save from the tree.

    pSrcDoc->GetFile(&pstrFile);

    hr = THR(CreateStreamOnFile(pstrFile,
                STGM_READ | STGM_SHARE_DENY_NONE, &pstmFile));
    if (hr)
        goto Cleanup;

    hr = THR(pSnapDoc->Load(pstmFile));
    if (hr)
        goto Cleanup;
    //
    // Process messages until we've finished loading.
    do
    {
        if (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } while ( pSnapDoc->LoadStatus() < LOADSTATUS_PARSE_DONE );

    // transfer doc to out parameter
    *ppSnapDoc = pSnapDoc;
    pSnapDoc = NULL;

Cleanup:
    ReleaseInterface(pstmFile);
    if (pSnapDoc)
    {
        // we are here due to an error
        pSnapDoc->Close(OLECLOSE_NOSAVE);
        pSnapDoc->Release();
    }
    if (pstrFile)
    {
        MemFreeString(pstrFile);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//   Member :SaveSnapShotDocument
//
//   Synopsis : entry point for the snapshot save logic.  called from
//          IPersistfile::save
//
//+---------------------------------------------------------------------------
HRESULT
CDoc::SaveSnapShotDocument(IStream * pstmSnapFile)
{
    HRESULT              hr = S_OK;
    CDoc               * pSnapDoc = NULL;
    IPersistStreamInit * pIPSI = NULL;
    IUnknown           * pDocUnk = NULL;

    if (!pstmSnapFile)
        return E_INVALIDARG;

    // create our design time document,
    hr = CreateSnapShotDocument(this, &pSnapDoc);
    if (hr)
        goto Cleanup;

    hr = THR(pSnapDoc->QueryInterface(IID_IUnknown, (void**)&pDocUnk));
    if (hr)
        goto Cleanup;

    // fire the save notification and let the peers transfer their
    // element's state into the design doc
    hr = THR(SaveSnapshotHelper( pDocUnk ));
    if (hr)
        goto Cleanup;

    // and now save the design doc.
    hr = THR(pSnapDoc->QueryInterface(IID_IPersistStreamInit, (void**)&pIPSI));
    if (hr)
        goto Cleanup;

    hr = THR(pIPSI->Save(pstmSnapFile, TRUE));

Cleanup:
    ReleaseInterface(pDocUnk);
    ReleaseInterface(pIPSI);
    if (pSnapDoc)
        pSnapDoc->Release();

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member : CDoc::SaveSnapshotHelper( IUnknown * pDocUnk)
//
//  Synopsis : this helper function does the task of firing the save 
//      notification and letting the peers put their element's state into
//      the design time document.  It is called by SaveSnapshotDocument 
//      (a save to stream operation) and by Exec::switch(IDM_SAVEASTHICKET)
//      the save-as call, will return this document to the browser where it
//      will be thicketized.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::SaveSnapshotHelper( IUnknown * pDocUnk, BOOL fVerifyParameters /* ==False */)
{
    HRESULT          hr = S_OK;
    IHTMLDocument2 * pIHTMLDoc = NULL;
    IPersistFile   * pIPFDoc   = NULL;
    TCHAR          * pstrFile = NULL;

    // SaveSnapshotDocument() is internal and has already done the work, 
    // Exec() has not.
    if (fVerifyParameters)
    {
        BSTR bstrMode = NULL;

        if (!pDocUnk)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        // dont do the snapshot (runtime) save while we are in design mode
        //  there may be no work to do at all.
        if (!PrimaryMarkup()->MetaPersistEnabled((long)htmlPersistStateSnapshot) ||
            _fDesignMode)
        {
            goto Cleanup;
        }

        // now we need to verify that this is indeed a 1> document, 2> loaded with 
        // current base file, and 3> in design mode.
        hr = THR(pDocUnk->QueryInterface(IID_IHTMLDocument2, (void**)&pIHTMLDoc));
        if (hr)
            goto Cleanup;

        hr = THR(pIHTMLDoc->get_designMode(&bstrMode));
        if (hr)
            goto Cleanup;

        if (! _tcsicmp(bstrMode, L"Off"))
            hr = E_INVALIDARG;

        SysFreeString(bstrMode);
        if ( hr )
            goto Cleanup;
    }

    // and finally do the work of the call to transfer the state 
    // from the current document into the (design) output doc
    {
        CNotification   nf;
        long            i;
        CStackPtrAry<CElement *, 64>  aryElements(Mt(CDocSaveSnapShotDocument_aryElements_pv));

        nf.SnapShotSave(PrimaryRoot(), &aryElements);
        BroadcastNotify(&nf);

        for (i = 0; i < aryElements.Size(); i++)
        {
            aryElements[i]->TryPeerSnapshotSave(pDocUnk);
        }
    }

Cleanup:
    ReleaseInterface( pIHTMLDoc );
    ReleaseInterface( pIPFDoc );
    if (pstrFile)
    {
        MemFreeString(pstrFile);
    }
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Load, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    LOADINFO LoadInfo = { 0 };

    LoadInfo.pchFile = (TCHAR *)pszFileName;
    LoadInfo.codepageURL = g_cpDefault;

    RRETURN(LoadFromInfo(&LoadInfo));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Save, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
#ifdef WINCE
    return S_OK;
#else
    HRESULT                 hr = S_OK;
    IStream *               pStm = NULL;
    LPCOLESTR               pszName;
    BOOL                    fBackedUp;
    TCHAR                   achBackupFileName[MAX_PATH];
    TCHAR                   achBackupPathName[MAX_PATH];
    TCHAR                   achFile[MAX_PATH];
    ULONG                   cchFile;
    const TCHAR *           pszExt;

    if (!pszFileName)
    {
        if (!_cstrUrl || GetUrlScheme(_cstrUrl) != URL_SCHEME_FILE)
            return E_UNEXPECTED;

        cchFile = ARRAY_SIZE(achFile);

        hr = THR(PathCreateFromUrl(_cstrUrl, achFile, &cchFile, 0));
        if (hr)
            RRETURN(hr);

        pszName = achFile;
    }
    else
    {
        pszName = pszFileName;
    }

    //
    // Point to the extension of the file name
    //
    pszExt = pszName + _tcslen(pszName);

    while (pszExt > pszName && *(--pszExt) != _T('.'))
        ;

    fBackedUp =
        GetTempPath(ARRAY_SIZE(achBackupPathName), achBackupPathName) &&
        GetTempFileName(achBackupPathName, _T("trb"), 0, achBackupFileName) &&
        CopyFile(pszName, achBackupFileName, FALSE);


#if 0
    if (!StrCmpIC(_T(".rtf"), pszExt) && RtfConverterEnabled())
    {
        CRtfToHtmlConverter * pcnv = new CRtfToHtmlConverter(this);

        if (!pcnv)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        }

        hr = THR(pcnv->InternalHtmlToExternalRtf(pszName));

        delete pcnv;

        if (hr)
            goto Error;
    }
    else
#endif
    {
        //
        // Create a stream on the file and save
        //
        hr = THR(CreateStreamOnFile(
                pszName,
                STGM_READWRITE | STGM_READWRITE | STGM_CREATE,
                &pStm));
        if (hr)
            goto Error;

#ifdef UNIX // Unix uses filter type as save-type
        if (!_strnicmp(".txt", MwFilterType(NULL, FALSE), 4))
#else
        if (!StrCmpIC(_T(".txt"), pszExt))
#endif
        {
            // Plaintext save should not touch the dirty bit
            _fPlaintextSave = TRUE; // smuggle the text-ness to CDoc::SaveToStream(IStream *pStm)
            hr = THR(SaveToStream(pStm, WBF_SAVE_PLAINTEXT|WBF_FORMATTED,
                GetCodePage()));
            _fPlaintextSave = FALSE;

            if (hr)
                goto Error;
        }
        else
        {
            // dont do the snapshot (runtime) save while we are in design mode
            if (!PrimaryMarkup()->MetaPersistEnabled((long)htmlPersistStateSnapshot) ||
                _fDesignMode)
            {
                // Save will reset the dirty flag if necessary.
                hr = THR(Save(pStm, fRemember));
            }
            else
            {
                // for Snapshot saving, after we save, we need to
                // reload what we just saved.
                if (pszName)
                {
                    hr = THR(SaveSnapShotDocument(pStm));
                }
            }
            if (hr)
                goto Error;
        }

        if (fRemember)
        {
            if (pszFileName)
            {
                hr = THR(SetFilename(pszFileName));
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:

    ReleaseInterface(pStm);

    if (fBackedUp)
    {
        // Delete backup file only if copy succeeded.
        DeleteFile(achBackupFileName);
    }

    RRETURN(hr);

Error:

    ClearInterface(&pStm);     // necessary to close pszFileName

    if (fBackedUp)
    {
        // Keep this flag true iff we restored successfully.  Setting
        // it to false if the copy fails ensures that the backup file
        // does not get deleted above.
        fBackedUp = CopyFile(achBackupFileName, pszName, FALSE);
    }

    goto Cleanup;
#endif // WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveCompleted, IPersistFile
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::SaveCompleted(LPCOLESTR pszFileName)
{
    HRESULT     hr = S_OK;

    if (pszFileName && _fDesignMode)
    {
        hr = THR(SetFilename(pszFileName));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Load, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDoc::Load(BOOL fFullyAvailable, IMoniker *pmkName, IBindCtx *pbctx,
    DWORD grfMode)
{
    PerfDbgLog1(tagPerfWatch, this, "+CDoc::Load (IPersistMoniker pbctx=%08lX)", pbctx);

    HRESULT     hr = E_INVALIDARG;
    TCHAR *pchURL = NULL;

    LOADINFO LoadInfo = { 0 };
    LoadInfo.pmk    = pmkName;
    LoadInfo.pbctx  = pbctx;

    if( pmkName == NULL )
        goto Cleanup;

    // Get the URL from the display name of the moniker:
    hr = pmkName->GetDisplayName(pbctx, NULL, &pchURL);
    if (FAILED(hr))
        goto Cleanup;

    LoadInfo.pchDisplayName = pchURL;

    hr = LoadFromInfo(&LoadInfo);

Cleanup:
    CoTaskMemFree(pchURL);

    PerfDbgLog(tagPerfWatch, this, "-CDoc::Load (IPersistMoniker)");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Save, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::Save(IMoniker *pmkName, LPBC pBCtx, BOOL fRemember)
{
    HRESULT     hr;
    IStream *   pStm = NULL;

    if (pmkName == NULL)
    {
        pmkName = _pmkName;
    }

    hr = THR(pmkName->BindToStorage(pBCtx, NULL, IID_IStream, (void **)&pStm));
    if (hr)
        goto Cleanup;

    hr = THR(Save(pStm, TRUE));
    if (hr)
        goto Cleanup;

    if (fRemember)
    {
        ReplaceInterface(&_pmkName, pmkName);
        ClearDwnPost();
    }

Cleanup:
    ReleaseInterface(pStm);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveCompleted, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SaveCompleted(IMoniker *pmkName, LPBC pBCtx)
{
    if (pmkName && _fDesignMode)
    {
        ReplaceInterface(&_pmkName, pmkName);
        ClearDwnPost();
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::LoadFromStream, CServer
//
//  Synopsis:   Loads the object's persistent state from a stream. Called
//              from CServer implementation of IPersistStream::Load
//              and IPersistStorage::Load
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromStream(IStream * pstm)
{
    BOOL fSync = _fPersistStreamSync;

    _fPersistStreamSync = FALSE;
    TraceTag((tagFormP, " LoadFromStream"));
#ifdef DEBUG
    CDataStream ds(pstm);
    ds.DumpStreamInfo();
#endif

    RRETURN(LoadFromStream(pstm, fSync));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::LoadFromStream
//
//  Synopsis:   Loads the object's persistent state from a stream. Called
//              from CServer implementation of IPersistStream::Load
//              and IPersistStorage::Load
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromStream(IStream * pstm, BOOL fSync, CODEPAGE cp)
{
    LOADINFO LoadInfo = { 0 };

    LoadInfo.pstm = pstm;
    LoadInfo.fSync = fSync;
    LoadInfo.codepage = cp;

    RRETURN(LoadFromInfo(&LoadInfo));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveToStream, CServer
//
//  Synopsis:   Saves the object's persistent state to a stream.  Called
//              from CServer implementation of IPersistStream::Save
//              and IPersistStorage::Save.
//
//----------------------------------------------------------------------------

#define BUF_SIZE 8192 // copy in chunks of 8192

HRESULT
CDoc::SaveToStream(IStream *pStm)
{
    HRESULT hr = E_FAIL;
    DWORD   dwFlags = WBF_FORMATTED;

    // There are three interesting places to save from:
    //  1) The tree if we're in design-mode and we're dirty or when we are saving
    //          to a temporary file for printing purposes, or when we are saving
    //          to html when the original file was plaintext. In the printing case we use the
    //          base tag that we have inserted to the tree to print images.
    //  2) The dirty stream if we're in run-mode and the dirty stream exists.
    //  3) The original source otherwise.
    //  4) We are in design mode, and not dirty and do not have original source,
    //          then save from the tree.

    if (    (_fDesignMode && IsDirty() != S_FALSE ) 
        ||  _fSaveTempfileForPrinting 
        ||  (HtmCtx() && HtmCtx()->GetMimeInfo() == g_pmiTextPlain && !_fPlaintextSave))  // Case 1
    {
        TraceTag((tagFormP, " SaveToStream Case 1"));
        // don't save databinding attributes during printing, so that we
        // print the current content instead of re-binding
        if (_fSaveTempfileForPrinting)
        {
            dwFlags |= WBF_SAVE_FOR_PRINTDOC | WBF_NO_DATABIND_ATTRS;
            if (PrimaryMarkup() && PrimaryMarkup()->IsXML())
                dwFlags |= WBF_SAVE_FOR_XML;
        }

        hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
        CDataStream ds(pStm);
        ds.DumpStreamInfo();
#endif
    }
    else if (!_fDesignMode && _pStmDirty)       // Case 2
    {
        ULARGE_INTEGER  cb;
        LARGE_INTEGER   liZero = {0, 0};

        cb.LowPart = ULONG_MAX;
        cb.HighPart = ULONG_MAX;

        TraceTag((tagFormP, " SaveToStream Case 2"));

        Verify(!_pStmDirty->Seek(liZero, STREAM_SEEK_SET, NULL));
        hr = THR(_pStmDirty->CopyTo(pStm, cb, NULL, NULL));
#if DBG==1
        CDataStream ds(pStm);
        ds.DumpStreamInfo();
#endif
    }
    else                                        // Case 3
    {
        if (HtmCtx() && !_fDesignMode)
        {
            if (!HtmCtx()->IsSourceAvailable())
            {
                TraceTag((tagFormP, " SaveToStream Case 3, no source available"));
                hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
                CDataStream ds(pStm);
                ds.DumpStreamInfo();
#endif
            }
            else
            {
                TraceTag((tagFormP, " SaveToStream Case 3, source available"));
                hr = THR(HtmCtx()->CopyOriginalSource(pStm, 0));
#if DBG==1
                CDataStream ds(pStm);
                ds.DumpStreamInfo();
#endif
            }
        }
        else
        {
            // Case 4:
            TraceTag((tagFormP, " SaveToStream Case 4"));
            hr = THR(SaveToStream(pStm, dwFlags, GetCodePage()));
#if DBG==1
            CDataStream ds(pStm);
            ds.DumpStreamInfo();
#endif
        }
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CDoc::WriteDocHeader(CStreamWriteBuff* pStm)
{
    // Write "<!DOCTYPE... >" stuff
    HRESULT hr = S_OK;
    DWORD dwFlagSave = pStm->ClearFlags(WBF_ENTITYREF);

    // Do not write out the header in plaintext mode.
    if(pStm->TestFlag(WBF_SAVE_PLAINTEXT))
        goto Cleanup;

    pStm->SetFlags(WBF_NO_WRAP);

    hr = pStm->Write(_T("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">"));
    if( hr )
        goto Cleanup;

    hr = pStm->NewLine( );
    if( hr )
        goto Cleanup;

    // If we are saving for printing, save out codepage metatag.
    if (dwFlagSave & WBF_SAVE_FOR_PRINTDOC)
    {
        CODEPAGE codepage = pStm->GetCodePage();
        TCHAR achCharset[MAX_MIMECSET_NAME];
        
        hr = GetMlangStringFromCodePage(codepage, achCharset, ARRAY_SIZE(achCharset));
        if (hr)
            goto Cleanup;

        hr = WriteTagNameToStream(pStm, _T("META"), FALSE, FALSE);
        if (hr)
            goto Cleanup;
       
        hr = WriteTagToStream(pStm, _T(" content=\"text/html; charset="));
        if (hr)
            goto Cleanup;
        
        hr = WriteTagToStream(pStm, achCharset);
        if (hr)
            goto Cleanup;
        
        hr = WriteTagToStream(pStm, _T("\" http-equiv=Content-Type>"));
        if (hr)
            goto Cleanup;
        
        hr = pStm->NewLine();
        if (hr)
            goto Cleanup;
    }

Cleanup:
    pStm->RestoreFlags(dwFlagSave);

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveToStream(pStm, dwFlags, codepage)
//
//  Synopsis:   Helper method to save to a stream passing flags to the
//              saver.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SaveToStream(IStream *pStm, DWORD dwStmFlags, CODEPAGE codepage)
{
    HRESULT          hr = S_OK;
    CStreamWriteBuff StreamWriteBuff(pStm, codepage);
    StreamWriteBuff.SetFlags(dwStmFlags);

    // Srinib, we have just created the stream buffer. But just to make sure
    // that we don't save DOCTYPE during plaintext save.
    if(!StreamWriteBuff.TestFlag(WBF_SAVE_PLAINTEXT))
    {
#ifndef WIN16
        // Write out the unicode signature if necessary
        if(     codepage == NATIVE_UNICODE_CODEPAGE 
            ||  codepage == NATIVE_UNICODE_CODEPAGE_BIGENDIAN
            ||  codepage == NONNATIVE_UNICODE_CODEPAGE
            ||  codepage == NONNATIVE_UNICODE_CODEPAGE_BIGENDIAN
            ||  codepage == CP_UTF_7 )
        {
            // BUGBUG: We have to write more or less data depending on whether
            //         non-native support is 2* sizeof(TCHAR) or sizeof(TCHAR) / 2
            TCHAR chUnicodeSignature = NATIVE_UNICODE_SIGNATURE;
            StreamWriteBuff.Write( (const TCHAR *) &chUnicodeSignature, 1 );
        }
#endif
        if (!StreamWriteBuff.TestFlag(WBF_SAVE_FOR_PRINTDOC))
        {
            // Update or create a meta tag for the codepage on the doc
            IGNORE_HR( UpdateCodePageMetaTag( codepage ) );
        }

        WriteDocHeader(&StreamWriteBuff);
    }

    // BUGBUG (paulpark) Should advance the code-page meta tag to the front of the head, or
    // at least make sure we save it first.

    Assert( PrimaryRoot() );
    CTreeSaver ts(PrimaryRoot(), &StreamWriteBuff);
    ts.SetTextFragSave( TRUE );
    hr = ts.Save();
    if( hr ) 
        goto Cleanup;

    hr = StreamWriteBuff.NewLine();

Cleanup:
    RRETURN(hr);
}

HRESULT
CDoc::SaveHtmlHead(CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT hr = S_OK;

    // If plaintext, don't write out the head.
    if (pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT))
        goto Cleanup;

    // When saving a file for the purpose of printing, we need to have
    // a BASE tag or we have little hope of printing any images.  It may
    // be of general use, though, to save the BASE from whence the document
    // orginally came.
    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC))
    {
        TCHAR * pchBaseUrl = NULL;

        hr = THR( GetBaseUrl( & pchBaseUrl ) );

        if (hr)
            goto Cleanup;

        if (pchBaseUrl)
        {
            hr = WriteTagNameToStream(pStreamWriteBuff, _T("BASE"), FALSE, FALSE);
            if (hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, _T(" HREF=\"")));
            if(hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, pchBaseUrl));
            if (hr)
                goto Cleanup;

            hr = THR(WriteTagToStream(pStreamWriteBuff, _T("\">")));
            if (hr)
                goto Cleanup;

            // 43859: For Athena printing, save font information if available
            //        NOTE that this must be the first FONT style tag so that
            //        any others can override it.
            //        NOTE that we only do this for the root document, since
            //        this is the only one for which Athena has set the font
            //        correctly. any sub-frames have _Trident_'s default font.

            Assert(_pCodepageSettings);
            if (GetRootDoc() == this && _pCodepageSettings->latmPropFontFace)
            {
                hr = THR(pStreamWriteBuff->NewLine());
                if (hr)
                    goto Cleanup;

                pStreamWriteBuff->BeginPre();

                hr = WriteTagNameToStream(pStreamWriteBuff, _T("STYLE"), FALSE, TRUE);
                if (hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, _T(" HTML { font-family : \"")));
                if(hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, (TCHAR *)fc().GetFaceNameFromAtom(_pCodepageSettings->latmPropFontFace)));
                if(hr)
                    goto Cleanup;

                hr = THR(WriteTagToStream(pStreamWriteBuff, _T("\" } ")));
                if(hr)
                    goto Cleanup;

                hr = WriteTagNameToStream(pStreamWriteBuff, _T("STYLE"), TRUE, TRUE);
                if (hr)
                    goto Cleanup;

                pStreamWriteBuff->EndPre();
            }
        }
    }

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::WriteTagToStream
//
//  Synopsis:   Writes given HTML tag to the stream turning off the entity
//               translation mode of the stream. If the WBF_SAVE_PLAINTEXT flag
//               is set in the stream nothing is done.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::WriteTagToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTag)
{
    HRESULT hr = S_OK;
    DWORD dwOldFlags;

    Assert(szTag != NULL && *szTag != 0);

    if(!pStreamWriteBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        // Save the state of the flag that controls the conversion of entities
        // and change the mode to "no entity translation"
        dwOldFlags = pStreamWriteBuff->ClearFlags(WBF_ENTITYREF);

        hr = THR(pStreamWriteBuff->Write(szTag));

        // Restore the entity translation mode
        pStreamWriteBuff->RestoreFlags(dwOldFlags);
    }

     RRETURN(hr);
}


HRESULT 
CDoc::WriteTagNameToStream(CStreamWriteBuff *pStreamWriteBuff, LPTSTR szTagName, BOOL fEnd, BOOL fClose)
{
    HRESULT hr = THR(WriteTagToStream(pStreamWriteBuff, fEnd ? _T("</") : _T("<")));
    if (hr)
        goto Cleanup;

    if (pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_PRINTDOC) && pStreamWriteBuff->TestFlag(WBF_SAVE_FOR_XML))
    {
        hr = THR(WriteTagToStream(pStreamWriteBuff, _T("HTML:")));
        if (hr)
            goto Cleanup;
    }
    
    hr = THR(WriteTagToStream(pStreamWriteBuff, szTagName));
    if (hr)
        goto Cleanup;

    if (fClose)
        hr = THR(WriteTagToStream(pStreamWriteBuff, _T(">")));

Cleanup:
    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CDoc::QueryRefresh
//
//  Synopsis:   Called to discover if refresh is supported
//
//--------------------------------------------------------------------------

HRESULT
CDoc::QueryRefresh(DWORD * pdwFlags)
{
    TraceTag((tagMsoCommandTarget, "CDoc::QueryRefresh"));

    *pdwFlags = (!_fDesignMode &&
                  (_cstrUrl || _pmkName || _pStmDirty || (HtmCtx() && HtmCtx()->WasOpened())))
            ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecStop
//
//  Synopsis:   Stop form
//
//--------------------------------------------------------------------------

HRESULT
CDoc::ExecStop(BOOL fFireOnStop /* = TRUE */)
{
    int             i;
    CNotification   nf;

    if (HtmCtx())
    {
        // If the document was opened through script, don't let the stop
        // button interfere.

        if (HtmCtx()->IsOpened())
            return(S_OK);

        HtmCtx()->DoStop(); // Do a "soft" stop instead of a "hard" SetLoad(FALSE)
    }

    StopUrlImgCtx();

    if (TLS(pImgAnim) && !_pDocParent)
    {
        TLS(pImgAnim)->SetAnimState((DWORD_PTR) this, ANIMSTATE_STOP);   // Stop img animation
    }

#ifndef NO_DATABINDING
    if (_pDBTask)
    {
        _pDBTask->Stop();
    }
#endif

    for (i = 0; i < _aryChildDownloads.Size(); i++)
    {
        _aryChildDownloads[i]->Terminate();
    }
    _aryChildDownloads.DeleteAll();

    StopPeerFactoriesDownloads();

    if (_fBroadcastStop || (GetProgSinkC() && GetProgSinkC()->GetClassCounter((DWORD)-1)))
    {
        nf.Stop1(PrimaryRoot());
        BroadcastNotify(&nf);
    }

    IGNORE_HR(CommitDeferredScripts(0));

    if (fFireOnStop)
        Fire_onstop();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecRefreshCallback
//
//  Synopsis:   ExecRefresh, but in a GWPostMethodCall compatible format
//
//--------------------------------------------------------------------------
void
CDoc::ExecRefreshCallback(DWORD_PTR dwOleCmdidf)
{
    IGNORE_HR(ExecRefresh((LONG)dwOleCmdidf));
}

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::ExecRefresh
//
//  Synopsis:   Refresh form
//
//--------------------------------------------------------------------------
extern BOOL IsInIEBrowser(CDoc * pDoc);

HRESULT
CDoc::ExecRefresh(LONG lOleCmdidf)
{
    IStream *   pStream     = NULL;
    DWORD       dwBindf;
    HRESULT     hr          = S_OK;

    #if DBG==1
    TraceTag((tagDocRefresh, "ExecRefresh(%lX) %ls", lOleCmdidf,
        _cstrUrl ? _cstrUrl : g_Zero.ach));
    TraceTag((tagDocRefresh, "    %s",
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_NORMAL) ?
            "OLECMDIDF_REFRESH_NORMAL (!! AOL Compat)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_NO_CACHE) ?
            "OLECMDIDF_REFRESH_NO_CACHE (F5)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_COMPLETELY) ?
            "OLECMDIDF_REFRESH_COMPLETELY (ctrl-F5)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_IFEXPIRED) ?
            "OLECMDIDF_REFRESH_IFEXPIRED (same as OLECMDIDF_REFRESH_RELOAD)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_CONTINUE) ?
            "OLECMDIDF_REFRESH_CONTINUE (same as OLECMDIDF_REFRESH_RELOAD)" :
        ((lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK) == OLECMDIDF_REFRESH_RELOAD) ?
            "OLECMDIDF_REFRESH_RELOAD (no forced cache validation)" :
            "?? Unknown OLECMDIDF_REFRESH_* level"));
    if (lOleCmdidf & OLECMDIDF_REFRESH_PROMPTIFOFFLINE)
        TraceTag((tagDocRefresh, "    OLECMDIDF_REFRESH_PROMPTIFOFFLINE"));
    if (lOleCmdidf & OLECMDIDF_REFRESH_CLEARUSERINPUT)
        TraceTag((tagDocRefresh, "    OLECMDIDF_REFRESH_CLEARUSERINPUT"));
    #endif

    if ((_pOmWindow && !_pOmWindow->Fire_onbeforeunload()) || _fInHTMLDlg) // Don't refersh in a dialog
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if ((lOleCmdidf & OLECMDIDF_REFRESH_PROMPTIFOFFLINE) && _pClientSite)
    {
        CTExec(_pClientSite, NULL, OLECMDID_PREREFRESH, 0, NULL, NULL);
    }

    switch (lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK)
    {
        case OLECMDIDF_REFRESH_NORMAL:
            dwBindf = BINDF_RESYNCHRONIZE;
            break;
        case OLECMDIDF_REFRESH_NO_CACHE:
            dwBindf = BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE;
            break;
        case OLECMDIDF_REFRESH_COMPLETELY:
            dwBindf = BINDF_GETNEWESTVERSION|BINDF_PRAGMA_NO_CACHE;
            break;
        default:
            dwBindf = 0;
            break;
    }

    _fFirstTimeTab = IsInIEBrowser(this);

    //
    // The Java VM needs to know when a refresh is
    // coming in order to clear its internal cache
    //

    if (_fHasOleSite)
    {
        CNotification   nf;

        nf.BeforeRefresh(PrimaryRoot(), (void *)(DWORD_PTR)(lOleCmdidf & OLECMDIDF_REFRESH_LEVELMASK));
        BroadcastNotify(&nf);
    }

    if (_fFiredOnLoad)
    {
        _fFiredOnLoad = FALSE;

        if (_pOmWindow)
        {
            CDoc::CLock Lock(this);
            _pOmWindow->Fire_onunload();
        }
    }

    //
    // Save the current doc state into a history stream
    //

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));
    if (hr)
        goto Cleanup;

    // note that ECHOHEADERS only has an effect if the original response for this page was a 449 response

    hr = THR(SaveHistoryInternal(pStream,
                SAVEHIST_ECHOHEADERS |
                ((lOleCmdidf & OLECMDIDF_REFRESH_CLEARUSERINPUT) ? 0 : SAVEHIST_INPUT)));
    if (hr)
        goto Cleanup;

    _fForceCurrentElem = TRUE;         // make sure next call succeeds
    PrimaryRoot()->BecomeCurrent(0);
    _fForceCurrentElem = FALSE;


    hr = THR(pStream->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(LoadHistoryInternal(pStream, NULL, dwBindf, _pmkName, NULL, NULL, 0));
    if (hr)
        goto Cleanup;

Cleanup:
    _fSafeToUseCalcSizeHistory = FALSE;
    ReleaseInterface(pStream);
    RRETURN1(hr,S_FALSE);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::SetMetaToTrident()
//
//  Synopsis:   When we create or edit (enter design mode on) a
//      document the meta generator tag in the header is to be
//      set to Trident and the appropriate build. If a meta generator
//      already exists then it should be replaced.
//             this is called after the creation of head,title... or
//      on switching to design mode. in both cases we can assume that
//      the headelement array is already created.
//
//---------------------------------------------------------------

static BOOL LocateGeneratorMeta ( CMetaElement * pMeta )
{
    LPCTSTR szMetaName;
    BOOL    fRet;

    szMetaName = pMeta->GetAAname();

    if(szMetaName == NULL)
    {
        fRet = FALSE;
    }
    else
    {
        fRet = ! StrCmpIC( pMeta->GetAAname(), _T( "GENERATOR" ) );
    }

    return fRet;
}

HRESULT
CDoc::SetMetaToTrident(void)
{
    HRESULT    hr = S_OK;
    TCHAR      achVersion [ 256 ];
    CMetaElement * pMeta=NULL;

    hr = THR(
        PrimaryMarkup()->LocateOrCreateHeadMeta( LocateGeneratorMeta, & pMeta ) );

    if (hr || !pMeta)
        goto Cleanup;

    hr = THR(
        pMeta->AddString(
            DISPID_CMetaElement_name, _T( "GENERATOR" ),
            CAttrValue::AA_Attribute ) );
    if (hr)
        goto Cleanup;

    hr =
        Format(
            0, achVersion, ARRAY_SIZE( achVersion ),
            _T("MSHTML <0d>.<1d2>.<2d4>.<3d>"), VER_PRODUCTVERSION);
    if (hr)
        goto Cleanup;

    hr = THR( pMeta->SetAAcontent( achVersion ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::UpdateCodePageMetaTag
//
//  Synopsis:   Update or create a META tag for the document codepage.
//
//---------------------------------------------------------------

static BOOL LocateCodepageMeta ( CMetaElement * pMeta )
{
    return pMeta->IsCodePageMeta();
}

HRESULT
CDoc::UpdateCodePageMetaTag( CODEPAGE codepage )
{
    HRESULT        hr = S_OK;
    TCHAR          achContentNew [ 256 ];
    TCHAR          achCharset[ MAX_MIMECSET_NAME ];
    CMetaElement * pMeta;
    AAINDEX        iCharsetIndex;

    hr = GetMlangStringFromCodePage(codepage, achCharset,
                                    ARRAY_SIZE(achCharset));
    if (hr)
        goto Cleanup;

    hr = THR(
        PrimaryMarkup()->LocateOrCreateHeadMeta( LocateCodepageMeta, & pMeta, FALSE ) );

    if (hr || !pMeta)
        goto Cleanup;

    // If the meta tag is already for the same codepage, leave the original form
    //  intact.
    if( pMeta->GetCodePageFromMeta( ) == codepage )
        goto Cleanup;

    hr = THR(
        pMeta->AddString(
            DISPID_CMetaElement_httpEquiv, _T( "Content-Type" ),
            CAttrValue::AA_Attribute ) );

    if (hr)
        goto Cleanup;

    hr = THR(
        Format(
            0, achContentNew, ARRAY_SIZE( achContentNew ),
            _T( "text/html; charset=<0s>" ), achCharset ) );

    if (hr)
        goto Cleanup;

    hr = THR( pMeta->SetAAcontent( achContentNew ) );

    if (hr)
        goto Cleanup;

    // If the meta was of the form <META CHARSET=xxx>, convert it to the new form.
    iCharsetIndex = pMeta->FindAAIndex( DISPID_CMetaElement_charset, CAttrValue::AA_Attribute );
    if( iCharsetIndex != AA_IDX_UNKNOWN )
    {
        pMeta->DeleteAt( iCharsetIndex );
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::HaveCodePageMetaTag
//
//  Synopsis:   Returns TRUE if the doc has a meta tag specifying
//              a codepage.
//
//---------------------------------------------------------------

BOOL
CDoc::HaveCodePageMetaTag()
{
    CMetaElement *pMeta;

    return PrimaryMarkup()->LocateHeadMeta(LocateCodepageMeta, &pMeta) == S_OK &&
           pMeta;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::InitNew, IPersistStream
//
//  Synopsis:   Initialize ole state of control. Overriden to allow this to
//              occur after already initialized or loaded (yuck!). From an
//              OLE point of view this is totally illegal. Required for
//              MSHTML classic compat.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::InitNew()
{
    LOADINFO LoadInfo = { 0 };
    CROStmOnBuffer stm;
    HRESULT hr;

    if (GetDefaultBlockTag() == ETAG_DIV)
    {
        hr = THR(stm.Init((BYTE *)"<div>&nbsp;</div>", 17));
    }
    else
    {
        hr = THR(stm.Init((BYTE *)"<p>&nbsp;</p>", 13));
    }

    if (hr)
        goto Cleanup;

    LoadInfo.pstm  = &stm;
    LoadInfo.fSync = TRUE;

    hr = THR(LoadFromInfo(&LoadInfo));

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::EnsureDirtyStream
//
//  Synopsis:   Save to dirty stream if needed. Create the dirty stream
//              if it does not already exist.
//
//---------------------------------------------------------------

HRESULT
CDoc::EnsureDirtyStream()
{
    HRESULT hr = S_OK;

    // if we're in design mode and we're dirty
    if (_fDesignMode)
    {
        if (IsDirty() != S_FALSE)
        {
            // Since we are dirty in design mode we have to persist
            // ourselves out to a temporary file

            //
            // If the stream is read only, release it because we'll
            // want to create a read-write one in a moment.
            //

            if (_pStmDirty)
            {
                STATSTG stats;

                hr = THR(_pStmDirty->Stat(&stats, STATFLAG_DEFAULT));

                if (stats.grfMode == STGM_READ || FAILED(hr))
                {
                    ReleaseInterface(_pStmDirty);
                    _pStmDirty = NULL;
                }
            }

            //
            // If we don't have a stream, create a read-write one.
            //

            if (!_pStmDirty)
            {
                TCHAR achFileName[MAX_PATH];
                TCHAR achPathName[MAX_PATH];
                DWORD dwRet;

                dwRet = GetTempPath(ARRAY_SIZE(achPathName), achPathName);
                if (!(dwRet && dwRet < ARRAY_SIZE(achPathName)))
                    goto Cleanup;

                if (!GetTempFileName(achPathName, _T("tri"), 0, achFileName))
                    goto Cleanup;

                hr = THR(CreateStreamOnFile(
                         achFileName,
                         STGM_READWRITE | STGM_SHARE_DENY_WRITE |
                                 STGM_CREATE | STGM_DELETEONRELEASE,
                         &_pStmDirty));
                if (hr)
                    goto Cleanup;
            }

            ULARGE_INTEGER   luZero = {0, 0};

            hr = THR(_pStmDirty->SetSize(luZero));
            if (hr)
                goto Cleanup;

            hr  = THR(SaveToStream(_pStmDirty));
        }
        else
        {
            // Since we are in design mode, and are not dirty, our dirty
            // stream is probably stale.  Clear it here.
            ClearInterface( &_pStmDirty );
        }
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member:     CDoc::GetHtmSourceText
//
//  Synopsis:   Get the decoded html source text specified
//
//---------------------------------------------------------------
HRESULT
CDoc::GetHtmSourceText(ULONG ulStartOffset, ULONG ulCharCount, WCHAR *pOutText, ULONG *pulActualCharCount)
{
    HRESULT hr;

    Assert(HtmCtx());

    if (!HtmCtx())
    {
        RRETURN(E_FAIL);
    }

    hr = THR(HtmCtx()->ReadUnicodeSource(pOutText, ulStartOffset, ulCharCount,
                pulActualCharCount));

    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SetDownloadNotify
//
//  Synopsis:   Sets the Download Notify callback object
//              to be used next time the document is loaded
//
//---------------------------------------------------------------
HRESULT
CDoc::SetDownloadNotify(IUnknown *punk)
{
    IDownloadNotify *pDownloadNotify = NULL;
    HRESULT hr = S_OK;

    if (punk)
    {
        hr = THR(punk->QueryInterface(IID_IDownloadNotify, (void**) &pDownloadNotify));
        if (hr)
            goto Cleanup;
    }

    ReleaseInterface(_pDownloadNotify);
    _pDownloadNotify = pDownloadNotify;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::GetViewSourceFileName
//
//  Synopsis:   Get the fully qualified name of the file to display when the
//              user wants to view source.
//
//---------------------------------------------------------------

HRESULT
CDoc::GetViewSourceFileName ( TCHAR * pszPath )
{
    Assert( pszPath );

    HRESULT hr;
    IStream *pstm = NULL;
    TCHAR achFileName[MAX_PATH];
    TCHAR * pchFile = NULL;
    CStr cstrVSUrl;

    hr = THR(EnsureDirtyStream());
    if (hr)
        RRETURN(hr);

    achFileName[0] = 0;

    if (_pStmDirty)
    {
        STATSTG statstg;

        hr = THR(_pStmDirty->Stat(&statstg, STATFLAG_DEFAULT));
        if (SUCCEEDED(hr) && statstg.pwcsName)
        {
            _tcscpy(pszPath, statstg.pwcsName);
            CoTaskMemFree( statstg.pwcsName );
        }
    }
    else if (HtmCtx())
    {
        // Use original filename, if it exists, Otherwise, use temporily file name
        if (IsAggregatedByXMLMime())
            hr = HtmCtx()->GetPretransformedFile(&pchFile);
        else
            hr = HtmCtx()->GetFile(&pchFile);
        if (!hr)
        {
            //
            //  If URL "file:" protocol and the document wasn't document.open()ed
            //  then use the original file name and path.Otherwise, use temp path
            //
            if (!HtmCtx()->WasOpened() && GetUrlScheme(_cstrUrl) == URL_SCHEME_FILE)
            {
                _tcsncpy(pszPath, pchFile, MAX_PATH);
                pszPath[MAX_PATH-1] = _T('\0');
                goto Cleanup;
            }
        }
        else
        {
            // Continue
            hr = S_OK;
        }
    }
    else
    {
        AssertSz(0, "EnsureDirtyStream failed to create a stream when no original was available.");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // We have either a pstmDirty or a pHtmCtx, but no file yet,
    // so create a temp file and write into it
    //
    cstrVSUrl.Set(_T("view-source:"));
    cstrVSUrl.Append(_cstrUrl);
    if (!CreateUrlCacheEntry(cstrVSUrl, 0, NULL, achFileName, 0))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(CreateStreamOnFile(
             achFileName,
             STGM_READWRITE | STGM_SHARE_DENY_WRITE | STGM_CREATE,
             &pstm));
    if (hr)
        goto Cleanup;

    if (_pStmDirty)
    {
        ULARGE_INTEGER uliSize;

        hr = THR(_pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_END, &uliSize));
        if (hr)
            goto Cleanup;

        hr = THR(_pStmDirty->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
        if (hr)
            goto Cleanup;

        hr = THR(_pStmDirty->CopyTo(pstm, uliSize, NULL, NULL));
        if (hr)
            goto Cleanup;
    }
    else
    {
        Assert(HtmCtx());
        if (!HtmCtx()->IsSourceAvailable())
        {
            hr = THR(SaveToStream(pstm, 0, GetCodePage()));
        }
        else
        {
            DWORD dwFlags = HTMSRC_FIXCRLF | HTMSRC_MULTIBYTE;
            if (IsAggregatedByXMLMime())
                dwFlags |= HTMSRC_PRETRANSFORM;
            hr = THR(HtmCtx()->CopyOriginalSource(pstm, dwFlags));
        }
        if (hr)
            goto Cleanup;
    }

    _tcscpy(pszPath, achFileName);

    hr = CloseStreamOnFile(pstm);
    if (hr)
        goto Cleanup;

    FILETIME fileTime;
    fileTime.dwLowDateTime = 0;
    fileTime.dwHighDateTime = 0;
    if (!CommitUrlCacheEntry(cstrVSUrl,
                             achFileName,
                             fileTime,
                             fileTime,
                             NORMAL_CACHE_ENTRY,
                             NULL,
                             0,
                             NULL,
                             0))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    MemFreeString(pchFile);
    ReleaseInterface(pstm);
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member:     CDoc::SavePretransformedSource
//
//  Synopsis:   Save the pre-transformed, pre-html file if any. For mime filters.
//
//---------------------------------------------------------------
HRESULT
CDoc::SavePretransformedSource(BSTR bstrPath)
{
    HRESULT hr = E_FAIL;

    if (HtmCtx() && HtmCtx()->IsSourceAvailable()) 
    {
        IStream *pIStream = NULL;
        LPTSTR pchPathTgt = bstrPath;
        LPTSTR pchPathSrc = NULL;

        // first see if it's the same file, we can't both create a read and write stream, so do nothing
        // both files should be canonicalized
        hr = HtmCtx()->GetPretransformedFile(&pchPathSrc);
        if (hr)
            goto CleanUp;
        if (0 == StrCmpI(pchPathTgt, pchPathSrc))
        {
            hr = S_OK;
            goto CleanUp;
        }

        // create the output stream
        hr = THR(CreateStreamOnFile(pchPathTgt, STGM_READWRITE | STGM_CREATE, &pIStream));
        if (hr)
            goto CleanUp;
            
        // copy the pretransformed source into it
        hr = THR(HtmCtx()->CopyOriginalSource(pIStream, HTMSRC_PRETRANSFORM));

CleanUp:
        ReleaseInterface(pIStream);
        MemFreeString(pchPathSrc);
    }
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     LoadFromInfo
//
//  Synopsis:   Workhorse method which loads (or reloads) the document.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::LoadFromInfo(LOADINFO * pLoadInfo)
{
    HTMLOADINFO         htmloadinfo;
    LOADINFO            LoadInfo          = *pLoadInfo;
    DWORD               dwOfflineFlag;
    DWORD               dwBindf           = 0;
    DWORD               dwDocBindf        = 0;
    DWORD               dwDownf           = 0;
    CDwnBindInfo *      pDwnBindInfo      = NULL;
    CScriptCollection * pScriptCollection = NULL;
    BOOL                fPrecreated       = FALSE;
    BOOL                fDocOffline       = FALSE;
    HRESULT             hr;
    IMoniker *          pmkNew            = NULL;
    IMoniker *          pmkSubstitute     = NULL;
    IBindCtx *          pbcNew            = NULL;
    CROStmOnBuffer *    prosOnBuffer      = NULL;
    TCHAR *             pchTask           = NULL;
    IHtmlLoadOptions *  pHtmlLoadOptions  = NULL;

    // Don't allow re-entrant loading of the document

    if (TestLock(FORMLOCK_LOADING))
        return(E_PENDING);

    CLock Lock(this, FORMLOCK_LOADING);

    PerfDbgLog(tagPerfWatch, this, "+CDoc::LoadFromInfo");
    TraceTag((tagCDoc, "%lx CDoc::LoadFromInfo URL=%ls", this, pLoadInfo->pchDisplayName));

    //
    // Grab refs before any "goto Cleanup"s
    // Don't put any failure code above these addrefs
    //

    if (LoadInfo.pstm)
        LoadInfo.pstm->AddRef();
    if (LoadInfo.pmk)
        LoadInfo.pmk->AddRef();

    //
    // Take ownership of any memory passed in before "goto Cleanup"s
    // Don't put any failure code above these assignments
    //

    pLoadInfo->pDwnPost      = NULL;
    pLoadInfo->pchDocReferer = NULL;
    pLoadInfo->pchSubReferer = NULL;
    pLoadInfo->pbRequestHeaders = NULL;
    pLoadInfo->cbRequestHeaders = 0;

    //
    // Can't be sure who might want the services of MLANG; Ensure it now
    // before a non-OleInitialize-ing thread (the download thread, for
    // example) decides it needs it.
    //

    THR( EnsureMultiLanguage() );

    //
    //
    // Handle FullWindowEmbed special magic:
    //
    if (_fFullWindowEmbed && LoadInfo.pchDisplayName && !LoadInfo.pstmDirty)
    {
        TCHAR *pszCookedHTML = NULL;
        int cbCookedHTML;

        // Synthesize the html which displays the plugin imbedded:
        // Starts with a Unicode BOM (byte order mark) to identify the buffer
        // as containing Unicode.
        static TCHAR altData[] =
         _T(" <<html><<body leftmargin=0 topmargin=0 scroll=no> <<embed width=100% height=100% fullscreen=yes src=\"<0s>\"><</body><</html>");
        altData[0] = NATIVE_UNICODE_SIGNATURE;

        // Create our cooked up HTML, which is an embed tag with src attr set to the current URL
        hr = Format( FMT_OUT_ALLOC, &pszCookedHTML, 0, altData, LoadInfo.pchDisplayName );
        if( FAILED( hr ) )
            goto Cleanup;
        cbCookedHTML = _tcslen( pszCookedHTML );

        // Create a stream onto that string.  Note that this CROStmOnBuffer::Init() routine
        // duplicates the string and owns the new version.  Thus it is safe to pass it this
        // local scope string buffer.
        prosOnBuffer = new CROStmOnBuffer;
        if (prosOnBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = prosOnBuffer->Init( (BYTE*)pszCookedHTML, cbCookedHTML*sizeof(TCHAR) );
        if (hr)
            goto Cleanup;

        // We may need some information stored in CDwnBindInfo when we
        // start loading up the plugin site.
        if (LoadInfo.pbctx)
        {
            IUnknown *pUnk = NULL;
            LoadInfo.pbctx->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);
            if (pUnk)
            {
                pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);
                ReleaseInterface(pUnk);
            }
            if (pDwnBindInfo)
            {
                // Get content type and cache filename of plugin data.
                _cstrPluginContentType.Set(pDwnBindInfo->GetContentType());
                _cstrPluginCacheFilename.Set(pDwnBindInfo->GetCacheFilename());
                pDwnBindInfo->Release();
                pDwnBindInfo = NULL;
            }
        }

        Assert( LoadInfo.pstmDirty == NULL );
        ReleaseInterface( LoadInfo.pstmDirty );  // just in case.
        LoadInfo.pstmDirty = prosOnBuffer;
        delete pszCookedHTML;
    }

    dwBindf = LoadInfo.dwBindf;

    // Extract the client site from the bind context and set it (if available).
    // It is not an error not to be able to do this.

    if (LoadInfo.pbctx)
    {
        IUnknown* pUnkParam = NULL;
        LoadInfo.pbctx->GetObjectParam(WSZGUID_OPID_DocObjClientSite,
                                        &pUnkParam);
        if( pUnkParam )
        {
            IOleClientSite* pOleClientSite = NULL;

            pUnkParam->QueryInterface(IID_IOleClientSite,
                                       (void**)&pOleClientSite);
            ReleaseInterface(pUnkParam);

            if (pOleClientSite)
            {
                hr = THR(SetClientSite(pOleClientSite));
                pOleClientSite->Release();
                if (FAILED(hr))
                    goto Cleanup;

                //
                // SetClientSite() function initializes _codepage with parents codepage value.
                // We have to update also LoadInfo.codepage to prevent overriding _codepage member 
                // by SwitchCodePage() function.
                //
                if (!LoadInfo.codepage && _pDocParent)
                    LoadInfo.codepage = (_pDocParent->IsCpAutoDetect() && !_pDocParent->_fCodepageOverridden)
                                        ? CP_AUTO
                                        : _pDocParent->_fCodePageWasAutoDetect
                                          ? CP_AUTO_JP
                                          : _codepage;
            }
        }

        //
        // Detect if shdocvw CoCreated trident if so then the bctx with have the
        // special string __PrecreatedObject.  If that is so then we don't want
        // to blow away the expando when the document is actually loaded (just
        // in case expandos were created on the window prior to loading).
        //
        hr = LoadInfo.pbctx->GetObjectParam(_T("__PrecreatedObject"), &pUnkParam);
        if (!hr)
        {
            ReleaseInterface(pUnkParam);

            // Signal we were precreated.
            fPrecreated = TRUE;
        }
    }

    // Load preferences from the registry if we haven't already
    if (!_fGotAmbientDlcontrol)
    {
        SetLoadfFromPrefs();
    }

    // Create CVersions object if we haven't already
    if (!_pVersions)
    {
        hr = THR(QueryVersionHost());
        if (hr)
            goto Cleanup;
    }

    // Unload the current contents, if any.

    if (_state >= OS_LOADED)
    {
#ifdef WIN16
        // in case we were are on some other thread's stack (e.g. Java)...
        // impersonate our own
        CThreadImpersonate cimp(_dwThreadId);
#endif
        if (_fFiredOnLoad)
        {
            _fFiredOnLoad = FALSE;

            if (_pOmWindow)
            {
                CDoc::CLock Lock(this);
                _pOmWindow->Fire_onunload();
            }
        }

        // BUGBUG: PSITEROOT null hack (johnbed)
        if( PrimaryRoot() )
        {
            _fForceCurrentElem = TRUE;         // make sure next call succeeds
            hr = THR(PrimaryRoot()->BecomeCurrent(0));
            _fForceCurrentElem = FALSE;
            if (hr)
                goto Cleanup;
        }

        // Free the undo buffer before unload the contents

        FlushUndoData();

        UnloadContents(fPrecreated, !!LoadInfo.pDwnBindData);   // fRestartLoad TRUE if pDwnBindData != NULL

        UpdateStatusText();
        DeferUpdateUI();
        DeferUpdateTitle();
        Invalidate();
    }

    //
    // At this point, we should be defoliated.  Start up a new tree.
    //

    Assert( PrimaryRoot() );

    _ulProgressPos  = 0;
    _ulProgressMax  = 0;
    _fShownProgPos  = FALSE;
    _fShownProgMax  = FALSE;
    _fShownProgText = FALSE;
    _fProgressFlash = FALSE;

    _dwHistoryIndex = 0;

    // Right now the globe should not be spinning.

    if (_fSpin)
    {
        SetSpin(FALSE);
    }

    // It's a go for loading, so say we're loaded

    if (_state < OS_LOADED)
        _state = OS_LOADED;

#if DBG==1
    DebugSetTerminalServer();
#endif

    // Before creating any elements, create the history load context

    Assert(!_pHistoryLoadCtx); // nobody should have tried to use it yet

    if (LoadInfo.pstmHistory)
    {
        _pHistoryLoadCtx = new CHistoryLoadCtx();

        if (_pHistoryLoadCtx && !OK(_pHistoryLoadCtx->Init(LoadInfo.pstmHistory, LoadInfo.pbctx)))
        {
            delete _pHistoryLoadCtx;
            _pHistoryLoadCtx = NULL;
        }
    }

    // If we don't have a history load context, then we prevent sending the DELAY_HISTORY_LOAD notification
    // by noting that we are finished with it even before we started.

    _fDelayLoadHistoryDone = !_pHistoryLoadCtx;

    //copy the string if there is one of these.
    // Note that we might not have a history, so a NULL return is okay.
    if (LoadInfo.pchHistoryUserData)
    {
        hr = _cstrHistoryUserData.Set(LoadInfo.pchHistoryUserData);
        if (hr)
            goto Cleanup;
    }

    // Now load the new contents from the appropriate source

    // Make a moniker if all we have is a filename
    // (Also do conversion of an RTF file to a temp HTML file if needed)

    if (LoadInfo.pchFile)
    {
        TCHAR achTemp[MAX_PATH];
        TCHAR achPath[MAX_PATH];
        TCHAR * pchExt;
        TCHAR achUrl[pdlUrlLen];
        ULONG cchUrl;
        ULONG cchPath;

        _tcsncpy(achTemp, LoadInfo.pchFile, MAX_PATH);
        achTemp[MAX_PATH-1] = 0;

#if 0
#if !defined(WINCE) && !defined(WIN16)
        if (LoadInfo.pstmDirty == NULL)
        {
            pchExt = _tcschr(achTemp, _T('.'));

            if (!_tcsnicmp(_T(".rtf"), 4, pchExt, -1) && RtfConverterEnabled())
            {
                CRtfToHtmlConverter *pcnv = new CRtfToHtmlConverter(NULL);

                if (pcnv == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(pcnv->ExternalRtfToInternalHtml(achTemp));

                delete pcnv;

                if (hr)
                    goto Cleanup;
            }
        }
#endif // ndef WINCE & WIN16
#endif

        // We now have a file to load from.
        // Make a URL moniker out of it.

        cchPath = GetFullPathName(achTemp, ARRAY_SIZE(achPath), achPath, &pchExt);
        if (!cchPath || cchPath > ARRAY_SIZE(achPath))
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }

        cchUrl = ARRAY_SIZE(achUrl);
#ifdef WIN16
        Assert(0);
#else
        hr = THR(UrlCreateFromPath(achPath, achUrl, &cchUrl, 0));
        if (hr)
#endif
            goto Cleanup;

        hr = THR(CreateURLMoniker(NULL, achUrl, &pmkNew));
        if (hr)
            goto Cleanup;

        ReplaceInterface(&LoadInfo.pmk, pmkNew);
        ClearInterface(&pmkNew);
    }

    // Make a moniker if all we have is a URL

    if (LoadInfo.pchDisplayName && !LoadInfo.pmk)
    {
        hr = THR(CreateURLMoniker(NULL, LoadInfo.pchDisplayName, &pmkNew));
        if (hr)
            goto Cleanup;

        ReplaceInterface(&LoadInfo.pmk, pmkNew);
        ClearInterface(&pmkNew);
    }

    // For SetBaseUrlFromMoniker hack (for dialogs):
    // if _pmkName is already filled in, grab that one

    if (!LoadInfo.pmk && _pmkName)
    {
        ReplaceInterface(&LoadInfo.pmk, _pmkName);
    }

    // Extract information from the pbctx
    if (LoadInfo.pbctx)
    {
        IUnknown * pUnk = NULL;

        Assert(LoadInfo.pmk); // we should only have a pbctx when we have a pmk

        LoadInfo.pbctx->GetObjectParam(SZ_HTMLLOADOPTIONS_OBJECTPARAM, &pUnk);

        if (pUnk)
        {
            pUnk->QueryInterface(IID_IHtmlLoadOptions, (void **)&pHtmlLoadOptions);
            pUnk->Release();
            pUnk = NULL;
        }

        if (pHtmlLoadOptions)
        {
            CODEPAGE cp = 0;
            BOOL     fHyperlink = FALSE;
            ULONG    cb = sizeof(CODEPAGE);
            TCHAR    achCacheFile[MAX_PATH];
            ULONG    cchCacheFile = sizeof(achCacheFile);

            // HtmlLoadOption from the shell can override the codepage
            hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_CODEPAGE, &cp, &cb));
            if (hr == S_OK && cb == sizeof(CODEPAGE))
            {
                HRESULT hrT = THR( MlangValidateCodePage(this, cp, _pInPlace ? _pInPlace->_hwnd : 0, FALSE) );
                if (OK(hrT))
                {
                    LoadInfo.fNoMetaCharset = TRUE;
                    LoadInfo.codepage = cp;
                }
            }

            // now determine if this is a shortcut-initiated load
            hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_INETSHORTCUTPATH,
                                                   &achCacheFile,
                                                   &cchCacheFile));
            if (hr == S_OK && cchCacheFile)
            {
                // we are a shortcut!
                IPersistFile *pISFile = NULL;

                // create the shortcut object for the provided cachefile
                hr = THR(CoCreateInstance(CLSID_InternetShortcut,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                              IID_IPersistFile,
                                          (void **)&pISFile));
                if (SUCCEEDED(hr))
                {
                    hr = THR(pISFile->Load(achCacheFile, 0));
                    if (SUCCEEDED(hr))
                    {
                        Assert(!_pShortcutUserData);
                        hr = THR(pISFile->QueryInterface(IID_INamedPropertyBag,
                                                      (void **)&_pShortcutUserData));
                        if (!hr)
                        {
                            IGNORE_HR(_cstrShortcutProfile.Set(achCacheFile));
                        }
                    }

                    pISFile->Release();
                }
            }
            // determine if we're loading as a result of a hyperlink
            cb = sizeof(fHyperlink);
            hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_HYPERLINK,
                                                   &fHyperlink,
                                                   &cb));
            if (hr == S_OK && cb == sizeof(fHyperlink))
                LoadInfo.fHyperlink = fHyperlink;
            hr = S_OK;
        }

        LoadInfo.pbctx->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);

        if (pUnk)
        {
            pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);
            pUnk->Release();
            pUnk = NULL;
        }

        if (pDwnBindInfo)
        {
            CDwnDoc * pDwnDoc = pDwnBindInfo->GetDwnDoc();

            dwBindf &= ~(BINDF_PRAGMA_NO_CACHE|BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE|BINDF_FORMS_SUBMIT);
            dwBindf |= pDwnDoc->GetBindf() & (BINDF_PRAGMA_NO_CACHE|BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE|BINDF_FORMS_SUBMIT);

            // (dinartem)
            // Need to distinguish the frame bind case from the normal
            // hyperlinking case.  Only frame wants to inherit dwRefresh
            // from parent, not the normal hyperlinking of a frame from
            // the <A TARGET="foo"> tag.
            // (dbau) this is now done via HTMLLOADOPTION_FRAMELOAD

            if (_pDocParent && !LoadInfo.fHyperlink)
            {
                // inherit dwRefresh from parent
                if (_pDocParent->_pDwnDoc && LoadInfo.dwRefresh == 0)
                {
                    LoadInfo.dwRefresh = _pDocParent->_pDwnDoc->GetRefresh();
                }
            }

            if (pDwnBindInfo->GetDwnPost())
            {
                if (LoadInfo.pDwnPost)
                    LoadInfo.pDwnPost->Release();
                LoadInfo.pDwnPost = pDwnBindInfo->GetDwnPost();
                LoadInfo.pDwnPost->AddRef();
            }

            if (pDwnDoc->GetDocReferer())
            {
                hr = THR(MemReplaceString(Mt(LoadInfo), pDwnDoc->GetDocReferer(),
                            &LoadInfo.pchDocReferer));
                if (hr)
                    goto Cleanup;
            }

            if (pDwnDoc->GetSubReferer())
            {
                hr = THR(MemReplaceString(Mt(LoadInfo), pDwnDoc->GetSubReferer(),
                            &LoadInfo.pchSubReferer));
                if (hr)
                    goto Cleanup;
            }

            // If we have a codepage in the hlink info, use it if one wasn't already specified
            if (!LoadInfo.codepage)
                LoadInfo.codepage = pDwnDoc->GetDocCodePage();

            if (!LoadInfo.codepageURL)
                LoadInfo.codepageURL = pDwnDoc->GetDocCodePage();
        }
    }

    /*
    This is a description of how we handle opening MHTML documents in IE4.
    The developers involved in this work are:
    - DavidEbb for the Trident part (Office developer on loan).
    - SBailey for the MimeOle code (in INETCOMM.DLL).
    - JohannP for the URLMON.DLL changes.

    Here are the different scenarios we are covering:
    1. Opening the initial MHTML file:
    - When we initially open the MHTML file in the browser, URLMON will instantiate the MHTML handler (which is Trident), and call its IPersistMoniker::Load(pmkMain).
        - Trident calls GetMimeOleAndMk(pmkMain, &punkMimeOle, &pmkNew) (a new MimeOle API).
        - MimeOle looks at the moniker (which may look like http://server/foo.mhtml ), and checks in a global table whether there already exists a Mime Message object for the moniker.  Suppose this is not the case now.
        - MimeOle creates a new Mime Message object, and loads its state from the moniker.  It then adds this new Mime Message object to the global table.
        - Now, MimeOle creates a new moniker, which will look like "mhtml: http://server/foo.mhtml !", and it returns it to Trident.  The meaning of this moniker is "the root object of the mhtml file".  It also returns an AddRef'd IUnknown pointer to
        - Trident holds on to the IUnknown ptr until it (Trident) is destroyed.  This guarantees that the Mime Message object stays around as long as Trident is alive.
        - Now Trident goes on to do what it normally does in its IPersistMoniker::Load, except that it uses pmkNew instead of pmkMain.
        - At some point in the Load code, Trident will call pmkNew->BindToStorage().
        - Now, we're in URLMON, which needs to deal with this funky mhtml: moniker.  The trick here is that MimeOle will have registered an APP (Asynchronous Pluggable Protocol) for mhtml:.  So URLMON instantiates the APP, and uses it to get the bits.
        - The APP looks at the moniker, breaks it into its two parts, " http://server/foo.mhtml " and "" (empty string, meaning root).  It finds http://server/foo.mhtml  in its global table, and uses the associated MimeMessage object for the download.


    2. Retrieving images referenced by the root HTML doc of the MHTML file:
    So at this point, Trident is loaded with the root HTML document of the MHTML file.  Now let's look at what happens when trident tries to resolve an embedded JPG image (say, bar.jpg):
    - Trident calls CoInternetCombineUrl, passing it "mhtml: http://server/foo.mhtml!" as the base, and "bar.jpg".
        - Now again, URLMON will need some help from the MHTML APP to combine those two guys.  So it will end up calling MimeOle to perform the combine operation.
        - MimeOle will combine the two parts into the moniker "mhtml: http://server/foo.mhtml!bar.jpg " (note: if there had been a non empty string after the bang, it would have been removed).
    - Trident then calls BindToStorage on this new moniker.  From there on, things are essentially the same as above.  It goes into the APP, which finds http://server/foo.mhtml  in its global table, and knows how to get the bits for the part named "ba


    3. Navigating to another HTML document inside the MHTML file:
    Now let's look at what happens when we navigate to another HTML document (say part2.html) which is stored in the MHTML file:
    - As above, the APP will be used to combine the two parts into the moniker "mhtml: http://server/foo.mhtml!part2.html ".
    - Now SHDCOVW calls BindToObject on this moniker.
        - URLMON uses the APP to get the part2.html data (similar to case above, skipping details).
    - The APP reports that the data is of type text/html, so URLMON instantiates Trident, and calls its IPersistMoniker::Load with the moniker mhtml: http://server/foo.mhtml!part2.html .
        - Trident looks at the moniker and sees that it has the mhtml: protocol.
    - Again, it calls GetMimeOleAndMk, but this time it gets back the very same moniker.  So the only purpose of this call is to get the Mime Message IUnknown ptr to hold on to.  Suggestion: we should use a different API in this case, which simply ret
        - Trident then keeps on going with the moniker, and things happen as in case 1.

    4. Retrieving images referenced by the other (non root) HTML doc of the MHTML file:
    This is almost identical to case 2., except the now the base moniker is "mhtml: http://server/foo.mhtml!part2.html " instead of "mhtml: http://server/foo.mhtml !".

    5. Refreshing the current page:
    This should now work quite nicely because the current moniker that IE uses has enough information to re-get the data "from scratch".  The reason it was broken before is that in our old scheme, IE only new the name of the part, and failed when tryi

    6. Resolving a shortcut:
    The very nice thing about this solution is that it will allow the creation of shortcuts to parts of an MHTML file.  This works for the same reason that case 5 works.
    */

    // If we've been instantiated as an mhtml handler, or if we are dealing
    // with a 'mhtml:' url, we need to get a special moniker from MimeOle.
    // Also, we get an IUnknown ptr to the Mime Message object, which allows
    // us to keep it alive until we go away.
    if (LoadInfo.pmk && (_fMhtmlDoc || _tcsnicmp(LoadInfo.pchDisplayName, 6,
        _T("mhtml:"), -1) == 0))
    {
#ifdef WIN16
        Assert(0);
        // BUGWIN16: so do we need to implement MimeOleObjectFromMoniker ?? or can
        // we just nuke all this code - vreddy - 7/21/97
#else
        Assert(_punkMimeOle == NULL);
        hr = MimeOleObjectFromMoniker((BINDF)dwBindf, LoadInfo.pmk,
            LoadInfo.pbctx, IID_IUnknown, (void**)&_punkMimeOle, &pmkNew);
        if (FAILED(hr))
            goto Cleanup;

        // From here on, work with the new moniker instead of the one passed in
        ReplaceInterface(&LoadInfo.pmk, pmkNew);
        ReplaceInterface(&_pmkName, pmkNew);

        // Create a new bind context if we are an mhtml handler, because this
        // will be a new binding operation.
        if (_fMhtmlDoc)
        {
            hr = CreateBindCtx(0, &pbcNew);
            if (FAILED(hr))
                goto Cleanup;

            // We don't hold a reference to LoadInfo.pbctx, so don't use
            // ReplaceInterface here.
            LoadInfo.pbctx = pbcNew;
        }
#endif
    }

    // Set the document's URL and moniker

    if (!LoadInfo.pmk)
    {
        hr = THR(SetUrl(_T("about:blank")));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(LoadInfo.pmk->GetDisplayName(LoadInfo.pbctx, NULL, &pchTask));
        if (hr)
            goto Cleanup;

        // now chop of #location part, if any
        TCHAR *pchLoc = const_cast<TCHAR *>(UrlGetLocation(pchTask));
        if (pchLoc)
            *pchLoc = _T('\0');

        hr = THR(_cstrUrl.Set(pchTask));
        if (hr)
            goto Cleanup;

        UpdateSecurityID();

        DeferUpdateTitle();

        ReplaceInterface(&_pmkName, LoadInfo.pmk);
    }

    Assert(!!_cstrUrl);
    MemSetName((this, "CDoc SSN=%d URL=%ls", _ulSSN, (LPTSTR)_cstrUrl));

#if DBG == 1
    if (this == GetRootDoc())
    {
        DbgExSetTopUrl(_cstrUrl);
    }
#endif

    IGNORE_HR(CompatBitsFromUrl(_cstrUrl, &_dwCompat));

    if (IsRootDoc())
    {
        if (GetUrlScheme(_cstrUrl) == URL_SCHEME_HTTPS && !LoadInfo.fUnsecureSource && !(_dwLoadf & DLCTL_SILENT))
        {
            SetRootSslState(SSL_SECURITY_SECURE, SSL_PROMPT_QUERY, TRUE);
        }
        else
        {
            SetRootSslState(SSL_SECURITY_UNSECURE, SSL_PROMPT_ALLOW, TRUE);
        }
    }
    else
    {
        // If we're not the root doc, we want to ignore security problems if
        // the root doc is unsecure
        if (AllowFrameUnsecureRedirect())
            dwBindf |= BINDF_IGNORESECURITYPROBLEM;
            
        // warn about mixed security (must come after call to SetClientSite)
        if (!ValidateSecureUrl(LoadInfo.pchDisplayName, FALSE, FALSE, LoadInfo.fUnsecureSource))
        {
            hr = THR(CreateURLMoniker(NULL, _T("about:NavigationCancelled"), &pmkSubstitute));
            if (hr)
                goto Cleanup;

            OnIgnoreFrameSslSecurity();

            _fSslSuppressedLoad = TRUE;
        }
    }


    // Allocate a script collection to use during loading

    pScriptCollection = new CScriptCollection();

    if (pScriptCollection == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pScriptCollection->Init(this));
    if (FAILED(hr))
        goto Cleanup;

    // Swap in the new ScriptCollection now

    Assert(pScriptCollection && !_pScriptCollection);
    _pScriptCollection = pScriptCollection;
    pScriptCollection = NULL;

#ifndef WIN16
    #ifdef DEBUG
    if (!IsPerfDbgEnabled(tagPushData))
    #endif
    {
        if (GetUrlScheme(_cstrUrl) == URL_SCHEME_MK)
#endif
        {
            dwBindf |= BINDF_PULLDATA;
        }
#ifndef WIN16
    }
#endif

    // Require load from local cache if offline,

    IsFrameOffline(&dwOfflineFlag);
    dwBindf |= dwOfflineFlag;

    // Require load from local cache if doing a history load (and non-refresh) of the result of a POST
    
    if (LoadInfo.pstmRefresh && LoadInfo.pDwnPost
             && !(dwBindf & (BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE)))
    {
        LoadInfo.pchFailureUrl = _T("about:PostNotCached");
        fDocOffline = TRUE;
    }

    if (_dwLoadf & DLCTL_SILENT)
    {
        dwBindf |= BINDF_SILENTOPERATION | BINDF_NO_UI;
    }

    if (_dwLoadf & DLCTL_RESYNCHRONIZE)
    {
        dwBindf |= BINDF_RESYNCHRONIZE;
    }

    if (_dwLoadf & DLCTL_PRAGMA_NO_CACHE)
    {
        dwBindf |= BINDF_PRAGMA_NO_CACHE;
    }

    if (LoadInfo.dwRefresh == 0)
    {
        LoadInfo.dwRefresh = IncrementLcl();
    }

    dwDownf = GetDefaultColorMode();

    if (IsPrintDoc() && dwDownf <= 16)
    {
        dwDownf = 24;
    }

    if (!_pOptionSettings->fSmartDithering)
    {
        dwDownf |= DWNF_FORCEDITHER;
    }

    if (_dwLoadf & DLCTL_DOWNLOADONLY)
    {
        dwDownf |= DWNF_DOWNLOADONLY;
    }

    if (LoadInfo.fSync) // IE5 #53582 (avoid RestartLoad if loading from a stream)
    {
        _dwLoadf |= DLCTL_NO_METACHARSET;
    }

    if (!LoadInfo.codepage)
    {
        // use CP_AUTO when so requested
        if (IsCpAutoDetect())
        {
            LoadInfo.codepage = CP_AUTO;
        }
        else if (_pDocParent)
        {
            // Inherit the codepage from our parent frame
            LoadInfo.codepage = _pDocParent->_fCodePageWasAutoDetect
                                ? CP_AUTO_JP
                                : _pDocParent->GetCodePage();
        }
        else
        {
            // Get it from the option settings default
            LoadInfo.codepage =
                NavigatableCodePage(_pOptionSettings->codepageDefault);
        }
    }

    if (!LoadInfo.codepageURL)
        LoadInfo.codepageURL = _codepage;

    SwitchCodePage(LoadInfo.codepage);

    // The default document direction is LTR. Only set this if the document is RTL
    _fRTLDocDirection = (unsigned)LoadInfo.fRTLDocDirection;

    _pDwnDoc = new CDwnDoc;

    if (_pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    dwDocBindf = dwBindf;

    // note (dbau) to fix bug 18360, we set BINDF_OFFLINEOPERATION only for the doc bind
    if (fDocOffline)
        dwDocBindf |= BINDF_OFFLINEOPERATION;

    _pDwnDoc->SetDoc(this);
    _pDwnDoc->SetBindf(dwBindf);
    _pDwnDoc->SetDocBindf(dwDocBindf);
    _pDwnDoc->SetDownf(dwDownf);
    _pDwnDoc->SetLoadf(_dwLoadf|(LoadInfo.fNoMetaCharset ? DLCTL_NO_METACHARSET : 0));
    _pDwnDoc->SetRefresh(LoadInfo.dwRefresh);
    _pDwnDoc->SetDocCodePage(NavigatableCodePage(LoadInfo.codepage));
    _pDwnDoc->SetURLCodePage(NavigatableCodePage(LoadInfo.codepageURL));
    _pDwnDoc->TakeRequestHeaders(&LoadInfo.pbRequestHeaders, &LoadInfo.cbRequestHeaders);
    _pDwnDoc->SetDownloadNotify(GetRootDoc()->_pDownloadNotify);
    _pDwnDoc->SetDwnTransform(IsAggregatedByXMLMime());

    if (_pOptionSettings->fHaveAcceptLanguage)
    {
        hr = THR(_pDwnDoc->SetAcceptLanguage(_pOptionSettings->cstrLang));
        if (hr)
            goto Cleanup;
    }

    if (_bstrUserAgent)
    {
        hr = THR(_pDwnDoc->SetUserAgent(_bstrUserAgent));
        if (hr)
            goto Cleanup;
    }

    if (LoadInfo.pchDocReferer)
    {
        hr = THR(_pDwnDoc->SetDocReferer(LoadInfo.pchDocReferer));
        if (hr)
            goto Cleanup;
    }

    if (LoadInfo.pchSubReferer || _cstrUrl)
    {
        hr = THR(_pDwnDoc->SetSubReferer(LoadInfo.pchSubReferer ?
                    LoadInfo.pchSubReferer : _cstrUrl));
        if (hr)
            goto Cleanup;
    }

    if (LoadInfo.pDwnPost)
    {
        Assert(!_pDwnPost); // just cleared in UnloadContents

        _pDwnPost = LoadInfo.pDwnPost;
        _pDwnPost->AddRef();
    }

    if (LoadInfo.fBindOnApt)
        htmloadinfo.pInetSess = NULL;
    else
        htmloadinfo.pInetSess = TlsGetInternetSession();

    htmloadinfo.pDwnDoc     = _pDwnDoc;
    htmloadinfo.pDwnPost    = _pDwnPost;
    htmloadinfo.pDoc        = this;
    htmloadinfo.pMarkup     = _pPrimaryMarkup;
    htmloadinfo.pchBase     = _cstrUrl;
    htmloadinfo.pmi         = LoadInfo.pmi;
    htmloadinfo.fClientData = LoadInfo.fKeepOpen;
    htmloadinfo.fParseSync  = LoadInfo.fSync;
    htmloadinfo.ftHistory   = LoadInfo.ftHistory;
    htmloadinfo.pchFailureUrl = LoadInfo.pchFailureUrl;
    htmloadinfo.pstmRefresh = LoadInfo.pstmRefresh;
    htmloadinfo.fKeepRefresh = LoadInfo.fKeepRefresh;
    htmloadinfo.pVersions   = _pVersions;
    htmloadinfo.fUnsecureSource = LoadInfo.fUnsecureSource;

    if (pmkSubstitute)
    {
        htmloadinfo.pmk = pmkSubstitute;
    }
    else if (LoadInfo.pDwnBindData)
    {
        htmloadinfo.pDwnBindData = LoadInfo.pDwnBindData;
        htmloadinfo.pstmLeader = LoadInfo.pstmLeader;
        htmloadinfo.pchUrl = _cstrUrl;      // used for security assumptions
    }
    else if (LoadInfo.pstmDirty)
    {
        htmloadinfo.pstm = LoadInfo.pstmDirty;
        ReplaceInterface(&_pStmDirty, LoadInfo.pstmDirty);
        htmloadinfo.pchUrl = _cstrUrl;      // used for security assumptions
    }
    else if (LoadInfo.pstm)
    {
        htmloadinfo.pstm = LoadInfo.pstm;
        htmloadinfo.pchUrl = _cstrUrl;      // used for security assumptions
    }
    else if (LoadInfo.pmk)
    {
        htmloadinfo.pmk = LoadInfo.pmk;
        htmloadinfo.pchUrl = _cstrUrl;
        htmloadinfo.pbc = LoadInfo.pbctx;
    }

    // Note: unlike other flavors of CDwnCtx, CHtmCtx's progsink is disconnectd
    // automatically as soon as load is done: we don't need to listen for
    // a dwnchan callback to disconnect the progsink.

    SetReadyState(READYSTATE_LOADING);

    //
    // LAUNCH THE DOWNLOAD
    //

    hr = THR(PrimaryMarkup()->Load (&htmloadinfo));

    //
    // finalize
    //

    if( _pStmDirty )
        _lDirtyVersion = MAXLONG;
    else
        _lDirtyVersion = 0;

    {
        // and finally, if this doc is in a [I]frame, then we need to tell the
        // frame that we've loaded.  this is necessary for persistence, and
        // shortcuts that deal with frames differently it they have the
        // original URL or not.
        CFrameSite*  pParentFrame = ParentFrameSite();

        if (!pParentFrame)
            pParentFrame = ParentIFrameSite();

        if (pParentFrame && LoadInfo.dwRefresh != 0)
            pParentFrame->NoteNavEvent();
    }

Cleanup:
    ReleaseInterface(LoadInfo.pstm);
    ReleaseInterface(LoadInfo.pmk);
    ReleaseInterface((IBindStatusCallback *)pDwnBindInfo);
    ReleaseInterface(pmkNew);
    ReleaseInterface(pmkSubstitute);
    ReleaseInterface(pbcNew);
    ReleaseInterface(prosOnBuffer);
    ReleaseInterface((IUnknown *)LoadInfo.pDwnPost);
    ReleaseInterface(pHtmlLoadOptions);
    MemFree(LoadInfo.pchDocReferer);
    MemFree(LoadInfo.pchSubReferer);
    MemFree(LoadInfo.pbRequestHeaders);
    CoTaskMemFree(pchTask);

    if (pScriptCollection)
    {
        pScriptCollection->Release();
    }

    //
    // If everything went A-OK, transition to the running state in
    // case we haven't done so already. Set active object if the
    // doc is ui-activate (fix for #49313, #51062)

    if (OK(hr))
    {
        if (State() < OS_RUNNING)
        {
            IGNORE_HR(TransitionTo(OS_RUNNING));
        }
        else if (State() >= OS_UIACTIVE)
        {
            SetActiveObject();
        }
    }

    PerfDbgLog(tagPerfWatch, this, "-CDoc::LoadFromInfo");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveSelection
//
//  Synopsis:   saves the current selection into
//              a file. The created file is an independent
//              .HTM file on it's own
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SaveSelection(TCHAR *pszFileName)
{
    HRESULT     hr = E_FAIL;
    IStream     *pStm = NULL;

    if (pszFileName && HasTextSelection())
    {
        // so there is a current selection, go and create a stream
        // and call the stream helper

        hr = THR(CreateStreamOnFile(
                pszFileName,
                STGM_READWRITE | STGM_READWRITE | STGM_CREATE,
                &pStm));
        if (hr)
            goto Cleanup;

        hr = THR(SaveSelection(pStm));

        ReleaseInterface(pStm);
    }

Cleanup:

    RRETURN(hr);
}





//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SaveSelection
//
//  Synopsis:   saves the current selection into
//              a stream. The created stream is saved in HTML format//
//
//----------------------------------------------------------------------------
HRESULT
CDoc::SaveSelection(IStream *pstm)
{
    HRESULT     hr = E_FAIL;
    ISelectionRenderingServices * pSelRenSvc = NULL;

    if (pstm  && HasTextSelection())
    {
        DWORD dwFlags = RSF_SELECTION | RSF_CONTEXT;
        CMarkup * pMarkup = GetCurrentMarkup();

        hr = GetCurrentSelectionRenderingServices( &pSelRenSvc );
        if( hr || pSelRenSvc == NULL )
            goto Cleanup;
            
        // so there is a current selection, go and get the text...
        // if there is a selection, we can safely assume (because
        // it is already checked in IsThereATextSelection)
        // that _pElemCurrent exists and is a txtSite
        
#if DBG == 1
        {
            INT iCount = 0;
            SELECTION_TYPE eType = SELECTION_TYPE_None;
            IGNORE_HR( pSelRenSvc->GetSegmentCount( &iCount , &eType ));
            Assert( eType == SELECTION_TYPE_Selection );
            Assert( iCount >= 1 );
        }
#endif // DBG == 1

        {
            CStreamWriteBuff      StreamWriteBuff(pstm, GetCodePage());
            int    i, iSegmentCount;                    

            // don't save databinding attributes during printing, so that we
            // print the current content instead of re-binding
            if (_fSaveTempfileForPrinting)
            {
                StreamWriteBuff.SetFlags(WBF_SAVE_FOR_PRINTDOC | WBF_NO_DATABIND_ATTRS);
                if (PrimaryMarkup() && PrimaryMarkup()->IsXML())
                    StreamWriteBuff.SetFlags(WBF_SAVE_FOR_XML);
            }

            hr = THR( pSelRenSvc->GetSegmentCount( & iSegmentCount, NULL ) );
            if( hr )
                goto Cleanup;

            if (!StreamWriteBuff.TestFlag(WBF_SAVE_FOR_PRINTDOC))
            {
                // HACK (cthrash) Force META tag persistance.
                //
                // Save any html header information needed for the rtf converter.
                // For now this is <HTML> and a charset <META> tag.
                //
                TCHAR achCharset[MAX_MIMECSET_NAME];

                if (GetMlangStringFromCodePage(GetCodePage(), achCharset,
                                               ARRAY_SIZE(achCharset)) == S_OK)
                {
                    DWORD dwOldFlags = StreamWriteBuff.ClearFlags(WBF_ENTITYREF);

                    StreamWriteBuff.Write(_T("<META CHARSET=\""));
                    StreamWriteBuff.Write(achCharset);
                    StreamWriteBuff.Write(_T("\">"));
                    StreamWriteBuff.NewLine();

                    StreamWriteBuff.RestoreFlags(dwOldFlags);
                }
            }
            
            //
            // Save the segments using the range saver
            //
            for ( i = 0; i < iSegmentCount; i++ )
            {
                CMarkupPointer mpStart(this), mpEnd(this);

                hr = THR( pSelRenSvc->MovePointersToSegment(i, &mpStart, &mpEnd) );
                if (S_OK == hr)
                {
                    // Skip saving text of password inputs.
                    CMarkup * pMarkup = mpStart.Markup();
                    CElement * pElementContainer = pMarkup ? pMarkup->Master() : NULL;
                    if (   pElementContainer
                        && pElementContainer->Tag() == ETAG_INPUT
                        && htmlInputPassword == DYNCAST(CInput, pElementContainer)->GetType())
                    {
                        continue;
                    }
                }

                CRangeSaver rs( &mpStart, &mpEnd, dwFlags, &StreamWriteBuff, pMarkup );

                hr = THR( rs.Save());
                if (hr)
                    goto Error;
            }

Error:
            StreamWriteBuff.Terminate();
        }
    }

Cleanup:
    ReleaseInterface( pSelRenSvc );
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ClearDwnPost()
//
//  Synopsis:   Used to discard original post data that lead to the page.
//              Called after moniker has changed or after HTTP redirect.
//
//----------------------------------------------------------------------------

void
CDoc::ClearDwnPost()
{
    if (_pDwnPost)
    {
        _pDwnPost->Release();
        _pDwnPost = NULL;
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::RestartLoad
//
//  Synopsis:   Reloads a doc from a DwnBindData in progress.
//
//-------------------------------------------------------------------------
HRESULT CDoc::RestartLoad(IStream *pstmLeader, CDwnBindData *pDwnBindData, CODEPAGE codepage)
{
    CStr cstrUrl;
    HRESULT hr;
    IStream * pStream = NULL;
    IStream * pstmRefresh;

    hr = THR(cstrUrl.Set(_cstrUrl));
    if (hr)
        goto Cleanup;

    //  are we going to a place already in the history?
    if (HtmCtx() && ((pstmRefresh = HtmCtx()->GetRefreshStream()) != NULL))
    {
        hr = THR(pstmRefresh->Clone(&pStream));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        //
        // Save the current doc state into a history stream
        //

        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));
        if (hr)
            goto Cleanup;

        hr = THR(SaveHistoryInternal(pStream, 0));    //  Clear user input
        if (hr)
            goto Cleanup;

        hr = THR(pStream->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
        if (hr)
            goto Cleanup;
    }

    hr = THR(LoadHistoryInternal(pStream, NULL, NULL, _pmkName, pstmLeader, pDwnBindData, codepage));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}



BOOL
CDoc::IsLoading()
{
    return(HtmCtx() && HtmCtx()->IsLoading());
}

HRESULT
CDoc::NewDwnCtx(UINT dt, LPCTSTR pchSrc, CElement * pel, CDwnCtx ** ppDwnCtx, BOOL fSilent, DWORD dwProgsinkClass)
{
    DWNLOADINFO dli     = { 0 };
    BOOL        fLoad   = TRUE;
    HRESULT     hr;
    TCHAR * pchExpUrl = new TCHAR[pdlUrlLen];

    if (pchExpUrl == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppDwnCtx = NULL;

    if (pchSrc == NULL)
    {
        hr = S_OK;
        goto Cleanup;
    }

    InitDownloadInfo(&dli);

    hr = THR(ExpandUrl(pchSrc, pdlUrlLen, pchExpUrl, pel));
    if (hr)
        goto Cleanup;
    dli.pchUrl = pchExpUrl;
    dli.dwProgClass = dwProgsinkClass;

    if (!ValidateSecureUrl((LPTSTR)dli.pchUrl, FALSE, fSilent))
    {
        hr = E_ABORT;
        goto Cleanup;
    }

    if (HtmCtx())
    {
        *ppDwnCtx = HtmCtx()->GetDwnCtx(dt, dli.pchUrl);

        if (*ppDwnCtx)
        {
            hr = S_OK;
            goto Cleanup;
        }
    }

    if (dt == DWNCTX_IMG && !(_dwLoadf & DLCTL_DLIMAGES))
        fLoad = FALSE;

    if (PrimaryMarkup() && LoadStatus() >= LOADSTATUS_PARSE_DONE)
    {
        // This flag tells the download mechanism not use the CDwnInfo cache until it has at
        // least verified that the modification time of the underlying bits is the same as the
        // cached version.  Normally we allow connection to existing images if they are on the
        // same page and have the same URL.  Once the page is finished loading and script makes
        // changes to SRC properties, we perform this extra check.

        dli.fResynchronize = TRUE;
    }

    hr = THR(::NewDwnCtx(dt, fLoad, &dli, ppDwnCtx));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pchExpUrl != NULL)
        delete pchExpUrl;
    RRETURN(hr);
}

void
CDoc::InitDownloadInfo(DWNLOADINFO * pdli)
{
    memset(pdli, 0, sizeof(DWNLOADINFO));
    pdli->pInetSess = TlsGetInternetSession();
    pdli->pDwnDoc   = _pDwnDoc;
}

//+------------------------------------------------------------------------
//
//  Member: LocalAddDTI()
//
//  Synopsis:
//      Local helper function to call IActiveDesktop interface to add a
//      desktop item at the given location.
//
//-------------------------------------------------------------------------
#ifndef WINCE

static HRESULT LocalAddDTI(LPCTSTR pszUrl, HWND hwnd, int x, int y, int nType)
{
    HRESULT hr;
    IActiveDesktop * pad;
    COMPONENT comp = {
        sizeof(COMPONENT),  // Size of this structure
        0,                  // For Internal Use: Set it always to zero.
        nType,              // One of COMP_TYPE_*
        TRUE,               // Is this component enabled?
        FALSE,              // Had the component been modified and not yet saved to disk?
        FALSE,              // Is the component scrollable?
        {
            sizeof(COMPPOS),//Size of this structure
            x,              //Left of top-left corner in screen co-ordinates.
            y,              //Top of top-left corner in screen co-ordinates.
            (DWORD)-1,      // Width in pixels.
            (DWORD)-1,      // Height in pixels.
            10000,          // Indicates the Z-order of the component.
            TRUE,           // Is the component resizeable?
            TRUE,           // Resizeable in X-direction?
            TRUE,           // Resizeable in Y-direction?
            -1,             // Left of top-left corner as percent of screen width
            -1              // Top of top-left corner as percent of screen height
        },                  // Width, height etc.,
        _T("\0"),           // Friendly name of component.
        _T("\0"),           // URL of the component.
        _T("\0"),           // Subscrined URL.
        IS_NORMAL           // ItemState
    };

    StrCpyN(comp.wszSource, pszUrl, ARRAY_SIZE(comp.wszSource));
    StrCpyN(comp.wszFriendlyName, pszUrl, ARRAY_SIZE(comp.wszFriendlyName));
    StrCpyN(comp.wszSubscribedURL, pszUrl, ARRAY_SIZE(comp.wszSubscribedURL));

    if(SUCCEEDED(hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (LPVOID *) &pad)))
    {
        hr = pad->AddDesktopItemWithUI(hwnd, &comp, DTI_ADDUI_DISPSUBWIZARD);

        if (pad)
            pad->Release();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member: _CreateDeskItem_ThreadProc
//
//  Synopsis:
//        Local function that serves as the threadProc to add desktop item.
//      We need to start a thread to do this because it may take a while and
//      we don't want to block the UI thread because dialogs may be displayed.
//
//-------------------------------------------------------------------------

static DWORD CALLBACK _CreateDeskItem_ThreadProc(LPVOID pvCreateDeskItem)
{
    CREATEDESKITEM * pcdi = (CREATEDESKITEM *) pvCreateDeskItem;

    HRESULT hres = OleInitialize(0);
    if (SUCCEEDED(hres))
    {
        hres = LocalAddDTI(pcdi->pszUrl, pcdi->hwnd, pcdi->x, pcdi->y, pcdi->dwItemType);
        OleUninitialize();
    }

    if(pcdi->pszUrl)
        MemFree((void *)(pcdi->pszUrl));
    MemFree((void *)pcdi);
    return 0;
}
#endif //WINCE
//+------------------------------------------------------------------------
//
//  Member:     CreateDesktopComponents
//
//  Synopsis:
//        Create Desktop Components for one item.  We need to start
//    a thread to do this because it may take a while and we don't want
//    to block the UI thread because dialogs may be displayed.
//
//-------------------------------------------------------------------------

static HRESULT CreateDesktopItem(LPCTSTR pszUrl, HWND hwnd, DWORD dwItemType, int x, int y)
{
    HRESULT hr = E_OUTOFMEMORY;
#ifndef WINCE
    //The following are allocated here; But, they will be freed at the end of _CreateDeskComp_ThreadProc.
    CREATEDESKITEM * pcdi = (CREATEDESKITEM *)MemAlloc(Mt(SetAsDesktopItem), sizeof(CREATEDESKITEM));
    LPTSTR  lpszURL = (LPTSTR)MemAlloc(Mt(SetAsDesktopItem), sizeof(TCHAR)*(_tcslen(pszUrl)+1));

    // Create Thread....
    if (pcdi && lpszURL)
    {
        _tcscpy(lpszURL, pszUrl); //Make a temporary copy of the URL
        pcdi->pszUrl = (LPCTSTR)lpszURL;
        pcdi->hwnd = hwnd;
        pcdi->dwItemType = dwItemType;
        pcdi->x = x;
        pcdi->y = y;

        SHCreateThread(_CreateDeskItem_ThreadProc, pcdi, CTF_INSIST, NULL);
        hr = S_OK;
    }
#endif //WINCE
    return hr;
}


