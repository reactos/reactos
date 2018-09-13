//------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       paddoc.cxx
//
//  Contents:   CPadDoc class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_PADDEBUG_HXX_
#define X_PADDEBUG_HXX_
#include "paddebug.hxx"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_NTVERP_H_
#define X_NTVERP_H_
#include "ntverp.h"
#endif

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include "stdio.h"
#endif

#ifndef X_PRIVCID_H_
#define X_PRIVCID_H_
#include "privcid.h"
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#include "platform.h"
#endif

extern void ScrubRegistry();
extern DYNLIB g_dynlibMSHTML;

DeclareTag(tagPadPositionPersist, "Pad", "Save last pad position to mshtmdbg.ini")
DeclareTag(tagPadNoSuspendForMessage, "Pad", "Don't Suspend/Resume CAP around PeekMessage/GetMessage")
DeclareTag(tagStatus, "Status",      "Write status line text to logfile")
DeclareTag(tagPalette, "Palette", "Trace Palette and ColorSet behaviour");
DeclareTag(tagSpanTag, "Edit",    "Insert span tags around block elements when editing")
DeclareTag(tagSpanTag2, "Edit",    "Insert span tags with compose settings outside")
PerfTag(tagPerfWatchPad, "Perf", "PerfWatch: Trace MSHTMPAD performance points")
ExternTag(tagDefaultDIV);
ExternTag(tagDefault);
MtDefine(Pad, WorkingSet, "mshtmpad.exe")
MtDefine(CPadDoc, Pad, "CPadDoc")
MtDefine(CPadScriptSite, Pad, "CPadScriptSite")
MtDefine(CDebugWindow, Pad, "CDebugWindow")
MtDefine(CDebugDownloadNotify, Pad, "CDebugDownloadNotify")

//
// static to store the pointer to WndProc of ComboBox'es in Format Toolbar
//
static WNDPROC lpfnDefCombo;

// messages for zoom combo box in Standard Toolbar
#define WM_ZOOMRETURN   (WM_USER+1)
#define WM_ZOOMESCAPE   (WM_USER+2)
#define WM_ZOOMUP       (WM_USER+3)
#define WM_ZOOMDOWN     (WM_USER+4)

static FORMATETC s_FormatEtcMF =
    { (CLIPFORMAT) CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT };

BOOL  CPadDoc::s_fPadWndClassRegistered = FALSE;
BOOL  CPadDoc::s_fPaletteDevice;

extern DWORD WINAPI CreatePadDocThreadProc(void * pv);

int CALLBACK
FillFontProc(const LOGFONT *    lplf,
             const TEXTMETRIC * lptm,
             DWORD            iFontType,
             LPARAM           lParam);

static const ComboItem ComboColorItems[] =
{
    {IDS_COLOR_BLACK,      RGB(0, 0, 0)},
    {IDS_COLOR_NAVY,       RGB(0, 0, 128)},
    {IDS_COLOR_BLUE,       RGB(0, 0, 255)},
    {IDS_COLOR_CYAN,       RGB(0, 255, 255)},
    {IDS_COLOR_RED,        RGB(255, 0, 0)},
    {IDS_COLOR_LIME,       RGB(0, 255, 0)},
    {IDS_COLOR_GRAY,       RGB(128, 128, 128)},
    {IDS_COLOR_GREEN,      RGB(0, 128, 0)},
    {IDS_COLOR_YELLOW,     RGB(255, 255, 0)},
    {IDS_COLOR_PINK,       RGB(255, 192, 203)},
    {IDS_COLOR_VIOLET,     RGB(238, 130, 238)},
    {IDS_COLOR_WHITE,      RGB(255, 255, 255)},
    {0, 0L}
};

#define INDEX_ZOOM_100  4

static const ComboItem ComboZoomItems[] =
{
    {IDS_ZOOM_1000,         1000},  //  0
    {IDS_ZOOM_500,          500},   //  1
    {IDS_ZOOM_200,          200},   //  2
    {IDS_ZOOM_150,          150},   //  3
    {IDS_ZOOM_100,          100},   //  4
    {IDS_ZOOM_75,           75},    //  5
    {IDS_ZOOM_50,           50},    //  6
    {IDS_ZOOM_25,           25},    //  7
    {IDS_ZOOM_10,           10},    //  8
    {IDS_ZOOM_5,            5},     //  9
    {IDS_ZOOM_1,            1},     // 10
    {0, 0L}
};

char *
IntToString(int i)
{
    static char ach[32];
    wsprintfA(ach, "%d", i);
    return(ach);
}

BOOL
IsUrlPrefix(const TCHAR * pchPath)
{
    return(memcmp(pchPath, _T("file:"),         5 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("http:"),         5 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("https:"),        6 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("ftp:"),          4 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("gopher:"),       7 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("sock:"),         5 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("about:"),        6 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("javascript:"),   11 * sizeof(TCHAR)) == 0 ||
           memcmp(pchPath, _T("vbscript:"),     9 * sizeof(TCHAR)) == 0  ||
           memcmp(pchPath, _T("mk:"),           3 * sizeof(TCHAR)) == 0);
}

HRESULT
GetOmDocumentFromDoc (IUnknown * pUnkDoc, IHTMLDocument2 ** ppOmDoc)
{
    HRESULT             hr;

    hr = THR(pUnkDoc->QueryInterface(IID_IHTMLDocument2, (void **)ppOmDoc));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

static HRESULT
CreateDoc(IUnknown **ppUnk)
{
    CThreadProcParam tpp(FALSE, ACTION_NONE);

    RRETURN(THR(CreatePadDoc(&tpp, ppUnk)));
}

CPadFactory PadDocFactory(CLSID_Pad, CreateDoc);

CPadDoc::CPadDoc(BOOL fUseShdocvw)
  : _aryPadPointers( Mt( Mem ) ), _aryPadContainers( Mt( Mem ) )
{
    PADTHREADSTATE * pts = GetThreadState();

    IncrementObjectCount();
    _ulRefs = 1;
    _ulAllRefs = 1;
    _fUserMode = TRUE;  // start in browse mode
    _pBrowser = NULL;
    _fUseShdocvw = fUseShdocvw;
    _lViewChangesFired = 0;
    _lDataChangesFired = 0;
    _dwCookie = 0;
    _fToolbarDestroy = FALSE;
    _fComboLoaded = FALSE;
    _fToolbarhidden = TRUE;
    _fFormatInit = FALSE;
    _fStandardInit = FALSE;
    _palState = palUnknown;
    _idPadIDNext = 1;
    //
    //  Add self to the head of the linked list of forms
    //
    _pDocNext = pts->pDocFirst;
    pts->pDocFirst = this;
}


CPadDoc::~CPadDoc( )
{
    PADTHREADSTATE * pts = GetThreadState();

    CPadDoc ** ppDoc;

    //  Remove from list of active documents
    for (ppDoc = &pts->pDocFirst; *ppDoc != NULL; ppDoc = &(*ppDoc)->_pDocNext)
    {
        if (*ppDoc == this)
        {
            *ppDoc = _pDocNext;
            break;
        }
    }

    if (!_fDecrementedObjectCount)
    {
        DecrementObjectCount();
    }

}



HRESULT
CPadDoc::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IPad || iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppvObj = (IPad *)this;
    }
    else if (iid == IID_IOleObject)
    {
        *ppvObj = (IOleObject *)this;
    }
    else if (iid == IID_IOleContainer)
    {
        *ppvObj = (IOleContainer *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

ULONG
CPadDoc::AddRef()
{
    _ulRefs += 1;
    return _ulRefs;
}

ULONG
CPadDoc::Release()
{
    if (--_ulRefs == 0)
    {
        _ulRefs = ULREF_IN_DESTRUCTOR;
        Passivate();
        _ulRefs = 0;
        SubRelease();
        return 0;
    }

    return _ulRefs;
}

ULONG
CPadDoc::SubAddRef( )
{
    return ++_ulAllRefs;
}

ULONG
CPadDoc::SubRelease()
{
    if (--_ulAllRefs == 0)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        _ulRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }
    return 0;
}


DYNPROC s_dynprocClearCache = { NULL, &g_dynlibMSHTML, "SvrTri_ClearCache" };
typedef void (WINAPI *PFN_SVRTRI_CLEARCACHE)();

void
CPadDoc::Passivate( )
{
    PFN_SVRTRI_CLEARCACHE  TRI_ClearCache;
    int i;

    for ( i = 0 ; i < _aryPadPointers.Size() ; i++ )
        _aryPadPointers[i]._pPointer->Release();

    for ( i = 0 ; i < _aryPadContainers.Size() ; i++ )
        _aryPadContainers[i]._pContainer->Release();
    
    PersistWindowPosition();

    if (    GetModuleHandleA("mshtml.dll")
        &&  OK(LoadProcedure(&s_dynprocClearCache)))
    {
        TRI_ClearCache = (PFN_SVRTRI_CLEARCACHE)s_dynprocClearCache.pfn;
        TRI_ClearCache();
    }
    
    CloseScripts();
    Deactivate();

    //
    // Some poorly behaved controls don't call OleSetMenuDescriptor when
    // cleaning up.  Do it now on behalf of them.
    //

    OleSetMenuDescriptor(NULL, _hwnd, NULL, &_Frame, NULL);

    ClearInterface(&_pObject);
    ClearInterface(&_pInPlaceObject);
    ClearInterface(&_pInPlaceActiveObject);
    ClearInterface(&_pTypeLibPad);
    ClearInterface(&_pTypeInfoCPad);
    ClearInterface(&_pTypeInfoIPad);
    ClearInterface(&_pTypeInfoILine);
    ClearInterface(&_pTypeInfoICascaded);
    ClearInterface(&_pTypeLibDLL);

    for ( i = 0; i < ARRAY_SIZE(_apTypeComp); i++)
    {
        ClearInterface(&_apTypeComp[i]);
    }

    ReleaseBindContext();

    DestroyToolbars();
    SetMenu(_hwnd, NULL);
    DestroyWindow(_hwnd);

    if(_pDebugWindow)
    {
        _pDebugWindow->Destroy();
    }

    if (!_fDecrementedObjectCount)
    {
        DecrementObjectCount();
        _fDecrementedObjectCount = TRUE;
    }

    if (_hpal)
    {
        DeleteObject(_hpal);
        _hpal = 0;
    }
    if ( _hmenuEdit )
    {
        ::DestroyMenu( _hmenuEdit );
        _hmenuEdit = NULL;
    }
    if ( _hmenuMain )
    {
        ::DestroyMenu( _hmenuMain );
        _hmenuMain = NULL;
    }
    if ( _hmenuHelp )
    {
        ::DestroyMenu( _hmenuHelp );
        _hmenuHelp = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::SetWindowPosition
//
//  Synopsis:   Restore window position from c:\ef if tagPadPosition is set.
//
//----------------------------------------------------------------------------

void
CPadDoc::SetWindowPosition()
{
    if (GetPrivateProfileIntA("PadPosition", "RestorePosition", FALSE, "mshtmdbg.ini"))
    {
        RECT rc;
        rc.left = GetPrivateProfileIntA("PadPosition", "PadLeft", 0, "mshtmdbg.ini");
        rc.top = GetPrivateProfileIntA("PadPosition", "PadTop", 0, "mshtmdbg.ini");
        rc.right = GetPrivateProfileIntA("PadPosition", "PadRight", 800, "mshtmdbg.ini");
        rc.bottom = GetPrivateProfileIntA("PadPosition", "PadBottom", 600, "mshtmdbg.ini");
        MoveWindow(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::PersistWindowPosition
//
//  Synopsis:   Write the pad window's coordinates to c:\ef if
//              tagPadPositionPersist is set.
//
//----------------------------------------------------------------------------

void
CPadDoc::PersistWindowPosition()
{
    if (IsTagEnabled(tagPadPositionPersist))
    {
        RECT rc;
        GetWindowRect(_hwnd, &rc);

        WritePrivateProfileStringA("PadPosition", "RestorePosition", IntToString(TRUE), "mshtmdbg.ini");
        WritePrivateProfileStringA("PadPosition", "PadLeft", IntToString(rc.left), "mshtmdbg.ini");
        WritePrivateProfileStringA("PadPosition", "PadTop", IntToString(rc.top), "mshtmdbg.ini");
        WritePrivateProfileStringA("PadPosition", "PadRight", IntToString(rc.right), "mshtmdbg.ini");
        WritePrivateProfileStringA("PadPosition", "PadBottom", IntToString(rc.bottom), "mshtmdbg.ini");
    }
}

BOOL
CPadDoc::GetDirtyState()
{
    BOOL fDirty = FALSE;
    IPersistFile *pPF = NULL;

    if (!_pObject)
        return FALSE;

    if (OK(THR_NOTRACE(_pObject->QueryInterface(IID_IPersistFile, (void **)&pPF))))
    {
        // BUGBUG Netscape cannot dirty the document but it reports dirty
        if (_fNetscape)
        {
            fDirty = FALSE;
        }
        else
        {
            fDirty = THR(pPF->IsDirty()) == S_OK;
        }
        pPF->Release();
    }

    return fDirty;
}

HRESULT
CPadDoc::QuerySave(DWORD dwSaveOptions)
{
    if (!GetDirtyState())
        return S_OK;

    switch( dwSaveOptions )
    {
    default:
        Assert(FALSE);

    case SAVEOPTS_SAVEIFDIRTY:
        break;

    case SAVEOPTS_NOSAVE:
        return S_OK;

    case SAVEOPTS_PROMPTSAVE:
        switch (MessageBox(_hwnd,
                TEXT("Do you want to save the changes ?"),
                SZ_APPLICATION_NAME,
                MB_YESNOCANCEL | MB_ICONQUESTION | MB_APPLMODAL))
        {
        case IDYES:
            break;

        case IDNO:
            return S_OK;

        case IDCANCEL:
        default:
            return S_FALSE;
        }
    }

    return DoSave(FALSE);
}



HRESULT
CPadDoc::DoSave(BOOL fPrompt)
{
    HRESULT hr = S_OK;
    IOleCommandTarget * pCommandTarget = 0;

    // BUGBUG (jenlc) Save through URL has not been implemented yet.
    // Request users to SAVEAS fils.
    //
    if ( fPrompt || _fInitNew || _fOpenURL)
    {
        //  Have to do Save As...
        //  The trick is the user can cancel this
        if ( _pInPlaceObject &&
             OK(_pInPlaceObject->QueryInterface(
                IID_IOleCommandTarget, (void **)&pCommandTarget)) )
        {
            hr = pCommandTarget->Exec(
                    NULL,
                    OLECMDID_SAVEAS,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    NULL);

            if ( hr )
                goto Cleanup;

            _fInitNew = FALSE;
        }
    }
    else
    {
        //  Save silently
        hr = THR(Save((TCHAR*)NULL));
    }

Cleanup:
    if ( hr == OLECMDERR_E_CANCELED )
    {
        hr = S_FALSE;
    }
    UpdateDirtyUI();
    ReleaseInterface(pCommandTarget);
    RRETURN1(hr, S_FALSE);
}


HRESULT
CPadDoc::PromptOpenFile(HWND hwnd, const CLSID * pclsid)
{
    HRESULT         hr;
    OPENFILENAME    ofn;
    BOOL            f;

    hr = QuerySave(SAVEOPTS_PROMPTSAVE);
    if (hr)
        RRETURN1(hr, S_FALSE);

    static TCHAR achPath[MAX_PATH] = _T("");
    static TCHAR achStartDir[MAX_PATH] = _T("");

#if !defined(WINCE)
    if (    !achPath[0] 
        &&  !achStartDir[0] 
        &&  GetEnvironmentVariable(_T("PAD_DEFAULTFILEDIR"), achStartDir, MAX_PATH))
    {
        if (_tcscmp(achStartDir, _T("CurrentDir")) == 0)
        {
            GetCurrentDirectory(MAX_PATH, achStartDir);
        }
    }
#endif

    if (!achPath[0] && !achStartDir[0])
    {
        char achStartDirA[MAX_PATH] = "";
        GetPrivateProfileStringA("PadDirs","FileDir","",achStartDirA,MAX_PATH, "mshtmdbg.ini");

        if (achStartDirA[0])
        {
            MultiByteToWideChar(CP_ACP, 0, achStartDirA, MAX_PATH, achStartDir, MAX_PATH);
        }
    }
     
    if (!achPath[0] && !achStartDir[0])
    {
        // Prime file name with samples directory.

        // Start with the directory containing this file.

        _tcscpy(achStartDir, _T(__FILE__));

        // Chop off the name of this file and three directories.
        // This will leave the root of our SLM tree in achPath.

        if ( ! _tcsrchr(achStartDir, _T(FILENAME_SEPARATOR))) {
            achStartDir[0] = (TCHAR)0;

        } else {

            for (int i = 0; i < 4; i++)
                {
                    TCHAR *pch = _tcsrchr(achStartDir, _T(FILENAME_SEPARATOR));
                    if (pch)
                        *pch = 0;
                }
        }

        // Append the name of the directory containing the samples.

        _tcscat(achStartDir, _T(FILENAME_SEPARATOR_STR) _T("src")
                             _T(FILENAME_SEPARATOR_STR) _T("f3")
                             _T(FILENAME_SEPARATOR_STR) _T("drt")
                             _T(FILENAME_SEPARATOR_STR) _T("samples"));
    }

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd ? hwnd : _hwnd;
    ofn.lpstrFilter = TEXT("HTML Documents (*.htm, *.html)\0*.htm;*.html\0RTF Documents (*.rtf)\0*.rtf\0Text files (*.txt)\0*.txt\0Dialogs (*.fmd)\0*.fmd\0All Files (*.*)\0*.*\0");
    ofn.lpstrFile = achPath;
    ofn.lpstrInitialDir = *achPath ? NULL : achStartDir;
    ofn.lpstrDefExt = TEXT("htm");
    ofn.nMaxFile = ARRAY_SIZE(achPath);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    DbgMemoryTrackDisable(TRUE);
    f = GetOpenFileName(&ofn);
    DbgMemoryTrackDisable(FALSE);

    if (!f)
        return S_FALSE;

    hr = _fUseShdocvw ? THR(OpenFile(achPath, NULL)) : ((!pclsid) ? THR(Open(achPath)) : THR(Open(*pclsid, achPath)));

    RRETURN(hr);
}

HRESULT
CPadDoc::PromptOpenURL(HWND hwnd, const CLSID *pclsid)
{
    TCHAR   achURL[MAX_PATH];
    HRESULT hr;

    hr = QuerySave(SAVEOPTS_PROMPTSAVE);
    if (hr)
        RRETURN1(hr, S_FALSE);

    if (!GetURL(hwnd ? hwnd : _hwnd, achURL, ARRAY_SIZE(achURL)))
        return S_FALSE;

    hr = _fUseShdocvw ? THR(OpenFile(achURL, NULL)) : ((!pclsid) ? THR(Open(achURL)) : THR(Open(*pclsid, achURL)));

    if (hr)
    {
        MessageBox(
            _hwnd,
            TEXT("Could not open the specified URL."),
            TEXT("Unable to open URL"),
            MB_APPLMODAL | MB_ICONERROR | MB_OK);
    }

    return S_OK;
}

void
CPadDoc::ReleaseBindContext()
{
    if (_pBinding)
    {
        THR(_pBinding->Abort());
        ClearInterface(&_pBinding);
    }

    if (_pBCtx)
    {
        // TODO (dbau) rearrange so that RevokeBSC is called in BSC::OnStopBinding

        THR(RevokeBindStatusCallback(_pBCtx, &_BSC));

        ClearInterface(&_pBCtx);
    }
}

HRESULT
CPadDoc::InitializeBindContext()
{
    HRESULT hr;

    // Cleanup previous binding and bind context.

    ReleaseBindContext();

    // Create new bind context.

    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &_pBCtx, 0));
    if (hr)
        goto Cleanup;

    hr = THR(RegisterBindStatusCallback(_pBCtx, &_BSC, NULL, 0));

    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


HRESULT
CPadDoc::Open(TCHAR *pchPath)
{
    PerfLog(tagPerfWatchPad, this, "+CPadDoc::Open");

    HRESULT         hr = S_OK;
    IOleObject *    pObject = NULL;
    ULONG           ulEaten;

    hr = THR(RegisterLocalCLSIDs());
    if (hr)
        goto Cleanup;

    // Shutdown the previous object.
    hr = Deactivate();
    if (hr)
        goto Cleanup;

    if (!_fDisablePadEvents)
    {
        if (_pEvent)
        {
            _pEvent->Event(pchPath, TRUE);
        }

        FireEvent(DISPID_PadEvents_Status, _T("Open"));
    }

    hr = THR(InitializeBindContext());
    if (hr)
        goto Cleanup;

    // Parse the text into a moniker. A single call to MkParseDisplayNameEx
    // should be all that it takes to handle this.  Due to bugs in
    // MkParseDisplayNameEx and the possiblity that URLMON.DLL is not
    // installed on the user's system, we have this more complicated code.

    if (IsUrlPrefix(pchPath))
    {
        DbgMemoryTrackDisable(TRUE);
        hr = THR(CreateURLMoniker(NULL, pchPath, &_pMk));
        DbgMemoryTrackDisable(FALSE);
    }
    else
    {
        hr = E_FAIL;
    }

    SendAmbientPropertyChange(DISPID_AMBIENT_USERMODE);

    if (hr)
    {
        hr = THR(MkParseDisplayName(_pBCtx, pchPath, &ulEaten, &_pMk));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pBCtx->RegisterObjectParam(SZ_HTML_CLIENTSITE_OBJECTPARAM, (IUnknown *)((IOleClientSite *)&_Site)));
    if (FAILED(hr))
        goto Cleanup;

    // Register this moniker as the one that binds asynchronously.

    // Get the object.

    DbgMemoryTrackDisable(TRUE);
    PerfLog(tagPerfWatchPad, this, "+CPadDoc::Open BindToObject");
    hr = THR_NOTRACE(_pMk->BindToObject(_pBCtx, NULL, IID_IOleObject, (void **)&pObject));
    PerfLog(tagPerfWatchPad, this, "-CPadDoc::Open BindToObject");
    DbgMemoryTrackDisable(FALSE);

    if (FAILED(hr))
        goto Cleanup;

    Assert(hr == S_OK || hr == S_ASYNCHRONOUS);
    hr = S_OK;

    _fOpenURL = IsUrlPrefix(pchPath);
    _fInitNew = FALSE;
    _fUserMode = !_fInitNew;

    // If the binding is asynchronous, then we get the object
    // pointer in a later call to the bind status callback.

    if (pObject)
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;

        hr = THR(Activate(pObject));
    }

Cleanup:
    ReleaseInterface(pObject);
    PerfLog(tagPerfWatchPad, this, "-CPadDoc::Open");
    RRETURN(hr);
}

HRESULT
CPadDoc::Open(REFCLSID clsid, TCHAR *pchPath)
{
    HRESULT                 hr;
    IPersistFile *          pPFile = NULL;
    IPersistStreamInit *    pPStm = NULL;
    IPersistMoniker *       pPMk = NULL;
    IPersistStorage *       pPStg = NULL;
    IStorage *              pStg = NULL;
    IOleObject   *          pObject = NULL;
    TCHAR *                 pchClass = NULL;
    IUnknown *              pUnk = NULL;
    DWORD                   dwFlags;
    BOOL                    fInitNew = _fInitNew;
    BOOL                    fUserMode = _fUserMode;
    BOOL                    fOpenURL = _fOpenURL;

    // Shutdown the previous object.
    hr = THR(Deactivate());
    if (hr)
        goto Cleanup;

    if (!_fDisablePadEvents)
    {
        if (_pEvent)
        {
            _pEvent->Event(pchPath, TRUE);
        }

        FireEvent(DISPID_PadEvents_Status, _T("Open"));
    }

    //
    // *********** BUGBUG UGLY TERRIBLE HACK FOR Netscape ! (istvanc)**************
    //
    if (clsid == CLSID_NSCP)
    {
        _fNetscape = TRUE;
    }

    hr = THR(RegisterLocalCLSIDs());
    if (hr)
        goto Cleanup;

    hr = THR(CoCreateInstance(
            clsid,
            NULL,
            _fNetscape ? CLSCTX_LOCAL_SERVER :
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IUnknown,
            (void **)&pUnk));
    if (hr)
        goto Cleanup;

    hr = pUnk->QueryInterface(IID_IOleObject, (void **)&pObject);
    if (hr)
        goto Cleanup;

    _fInitNew = ((pchPath == NULL) || (*pchPath == 0)) && !(clsid == CLSID_WebBrowser);
    _fUserMode = !_fInitNew;
    _fOpenURL = pchPath && IsUrlPrefix(pchPath);

    hr = THR(pObject->GetMiscStatus(DVASPECT_CONTENT, &dwFlags));
    if (hr)
        goto Cleanup;

    if (dwFlags & OLEMISC_SETCLIENTSITEFIRST)
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;

        if (_ulDownloadNotifyMask)
        {
            hr = THR(AttachDownloadNotify(pObject));
            if (hr)
                goto Cleanup;
        }
    }

#if 0
    // *********** BUGBUG UGLY TERRIBLE HACK FOR MSHTML ! (istvanc)**************
    if (clsid == CLSID_IE_HTML)
    {
        _fMSHTML = TRUE;
    }
#endif

    if (pchPath)
    {
        // NOTE: (alexz) please note that moniker should be created
        // regardless of if we use it in this routine or not; this moniker
        // will be used by GetMoniker function.
        // So please do not move the CreateURLMoniker call under the following
        // 'if' statement without consideration of how GetMoniker will be affected

        hr = THR(CreateURLMoniker(NULL, pchPath, &_pMk));
        if (hr)
            goto Cleanup;

        if (IsUrlPrefix(pchPath))
        {
            hr = THR(pObject->QueryInterface(
                    IID_IPersistMoniker,
                    (void **)&pPMk));
            if (hr)
                goto Cleanup;

            hr = THR(pPMk->Load(FALSE, _pMk, NULL, STGM_READ));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(pUnk->QueryInterface(
                    IID_IPersistFile,
                    (void **) &pPFile));
            if (hr)
                goto Cleanup;

            hr = THR(pPFile->Load(pchPath, 0));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = THR(pObject->QueryInterface(
                IID_IPersistStreamInit,
                (void **)&pPStm));
        if (OK(hr))
        {
            hr = THR(pPStm->InitNew());
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(pObject->QueryInterface(
                    IID_IPersistStorage,
                    (void **)&pPStg));
            if (hr)
                goto Cleanup;

            hr = THR(StgCreateDocfile(
                    NULL,
                    STGM_READWRITE | STGM_DELETEONRELEASE | STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_TRANSACTED,
                    0,
                    &pStg));
            if (hr)
                goto Cleanup;

            hr = THR(pPStg->InitNew(pStg));
            if (hr)
                goto Cleanup;
        }
    }

    if (!(dwFlags & OLEMISC_SETCLIENTSITEFIRST))
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;
    }

    hr = THR(Activate(pObject));
    if (hr)
        goto Cleanup;

    //  Reset Zooming
    ResetZoomPercent();

Cleanup:
    if (hr)
    {
        _fInitNew = fInitNew;
        _fUserMode = fUserMode;
        _fOpenURL = fOpenURL;
    }

    CoTaskMemFree(pchClass);
    ReleaseInterface(pUnk);
    ReleaseInterface(pObject);
    ReleaseInterface(pPFile);
    ReleaseInterface(pPStg);
    ReleaseInterface(pStg);
    ReleaseInterface(pPStm);
    ReleaseInterface(pPMk);
    RRETURN(hr);
}

HRESULT
CPadDoc::Deactivate()
{
    HRESULT hr = S_OK;
    CVariant varBool;

    // give events a chance to fire
    hr = THR_NOTRACE(ExecuteCommand(OLECMDID_ONUNLOAD, &varBool));
    if ((V_VT(&varBool) == VT_BOOL && !V_BOOL(&varBool)))
    {
        hr = S_OK;
        goto Cleanup;
    }


    if (_hwndWelcome)
    {
        DestroyWindow(_hwndWelcome);
        _hwndWelcome = NULL;
    }

    if (_pInPlaceActiveObject)
    {
        IGNORE_HR(_pInPlaceObject->UIDeactivate());
    }

    if (_pInPlaceObject)
    {
        IGNORE_HR(_pInPlaceObject->InPlaceDeactivate());
    }

    if (_PNS._dwCookie)
    {
        DisconnectSink(_pObject, IID_IPropertyNotifySink, &_PNS._dwCookie);
        Assert(_PNS._dwCookie == 0);
    }

    if (_pObject)
    {
        IViewObject *pVO = NULL;

        if (OK(_pObject->QueryInterface(IID_IViewObject, (void **)&pVO)))
        {
            pVO->SetAdvise(DVASPECT_CONTENT, 0, NULL);
            pVO->Release();
        }

        if (_dwCookie)
        {
            IDataObject *pDO = NULL;

            if (OK(_pObject->QueryInterface(IID_IDataObject, (void **)&pDO)))
            {
                pDO->DUnadvise(_dwCookie);
                pDO->Release();
            }
            
            _dwCookie = 0;
        }

        hr = THR(_pObject->Close(OLECLOSE_NOSAVE));
        if (hr)
            goto Cleanup;
    }

    ClearInterface(&_pBrowser);
    ClearInterface(&_pInPlaceObject);
    ClearInterface(&_pInPlaceActiveObject);
    ClearInterface(&_pObject);
    ClearInterface(&_pMk);

    _fOpenURL = FALSE;

    _lReadyState = READYSTATE_UNINITIALIZED;

Cleanup:
    RRETURN(hr);
}

HRESULT
CPadDoc::Activate(IOleObject *pObject)
{
    PerfLog(tagPerfWatchPad, this, "+CPadDoc::Activate");

    HRESULT hr;
    RECT rc;

    // Activate the new object.

    Assert(!_pObject);
    _pObject = pObject;
    _pObject->AddRef();

    IViewObject *pVO = NULL;

    if (OK(_pObject->QueryInterface(IID_IViewObject, (void **)&pVO)))
    {
        PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (IID_IViewObject Advise)");
        pVO->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, (IAdviseSink *)&_Site);
        pVO->Release();
    }

    IDataObject *pDO = NULL;

    if (OK(_pObject->QueryInterface(IID_IDataObject, (void **)&pDO)))
    {
        PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (IID_IDataObject Advise)");
        pDO->DAdvise(&s_FormatEtcMF, ADVF_NODATA, (IAdviseSink *)&_Site, &_dwCookie);
        pDO->Release();
    }

    GetViewRect(&rc, TRUE);

    PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (DoVerb OLEIVERB_SHOW)");
    hr = THR(_pObject->DoVerb(
            OLEIVERB_SHOW,
            NULL,
            &_Site,
            0,
            _hwnd,
            ENSUREOLERECT(&rc)));
    if (hr)
        goto Cleanup;

    Assert(_PNS._dwCookie == 0);

    PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (ConnectSink IID_IPropertyNotifySink)");
    hr = THR(ConnectSink(_pObject, IID_IPropertyNotifySink, &_PNS,
            &_PNS._dwCookie));
    if (hr)
        goto Cleanup;

    PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (UpdateToolbarUI)");
    UpdateToolbarUI();
    PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (SetupComposeFont)");
    SetupComposeFont();
    // Use the correct default block element.
    PerfLog(tagPerfWatchPad, this, "CPadDoc::Activate (SetupDefaultBlock)");
    SetupDefaultBlock();

Cleanup:
    PerfLog(tagPerfWatchPad, this, "-CPadDoc::Activate");
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDocPNS::QueryInterface
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CPadDocPNS, CPadDoc, _PNS)

STDMETHODIMP
CPadDocPNS::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPropertyNotifySink)
    {
        *ppv = (IPropertyNotifySink *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDocPNS::OnChanged
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP CPadDocPNS::OnChanged(DISPID dispid)
{
    if (dispid == DISPID_READYSTATE)
    {
        MyCPadDoc()->OnReadyStateChange();
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::OnReadyStateChange
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void CPadDoc::OnReadyStateChange()
{
    HRESULT     hr = S_OK;
    CVariant    Var;
    IDispatch * pdisp = NULL;
    LONG        lReadyState;

    _pObject->QueryInterface(IID_IDispatch, (void **)&pdisp);

    if (pdisp == NULL)
        return;

    hr = GetDispProp(pdisp, DISPID_READYSTATE, 0, &Var, NULL);
    pdisp->Release();
    if (hr)
        return;

    //
    // Look for either VT_I4 or VT_I2
    //

    if (V_VT(&Var) == VT_I4)
    {
        lReadyState = V_I4(&Var);
    }
    else if (V_VT(&Var) == VT_I2)
    {
        lReadyState = V_I2(&Var);
    }
    else
    {
        return;
    }

    if (_lReadyState == lReadyState)
        return;

    _lReadyState = lReadyState;

    if (!_fDisablePadEvents && _lReadyState == READYSTATE_COMPLETE)
    {
        FireEvent(DISPID_PadEvents_DocLoaded, TRUE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::Save
//
//  Synopsis:   This function saves the document to given file name without
//              displaying a dialog. It changes current document name to given
//              name.
//
//----------------------------------------------------------------------------

HRESULT
CPadDoc::Save(LPCTSTR szFileName)
{
    HRESULT            hr = S_OK;
    IPersistFile *    pPF = NULL;

    if ( ! _pObject )
        goto Cleanup;

    hr = THR(_pObject->QueryInterface(IID_IPersistFile, (void **)&pPF));
    if (hr)
        goto Cleanup;

    hr = THR(pPF->Save(szFileName, TRUE));
    if (hr)
        goto Cleanup;

    hr = THR(pPF->SaveCompleted(NULL));
    if (hr)
        goto Cleanup;

    _fInitNew = FALSE;

Cleanup:
    ReleaseInterface(pPF);
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
// Member:    CPadDoc::Init
//
// Synopsis:
//
//--------------------------------------------------------------------

HRESULT
CPadDoc::Init(int nCmdShow, CEventCallBack * pEvent)
{
    HRESULT     hr = S_OK;

    _pEvent = pEvent;

    // Ensure that the common control DLL is loaded for status window.
    InitCommonControls();

    _hmenuMain = LoadMenu(g_hInstResource, MAKEINTRESOURCE(GetMenuID()));
    _hmenuEdit = LoadMenu(g_hInstResource, MAKEINTRESOURCE(IDR_PADMENU_EDIT));
    hr = THR(RegisterPadWndClass());
    if(hr)
        goto Cleanup;

    CreateWindowEx(
            0,
            SZ_PAD_WNDCLASS,
            SZ_APPLICATION_NAME,
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL,
            NULL,
            g_hInstCore,
            this);

    if (!_hwnd)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    SetWindowPosition();    

    CreateToolBarUI();

    // Create the status window.
    _hwndStatus = CreateWindowEx(
            0,                       // no extended styles
            STATUSCLASSNAME,         // name of status window class
            (LPCTSTR) NULL,          // no text when first created
            SBARS_SIZEGRIP |         // includes a sizing grip
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,   // creates a child window
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            _hwnd,                   // handle to parent window
            (HMENU) 0,               // child window identifier
            g_hInstCore,             // handle to application instance
            NULL);                   // no window creation data
    if (!_hwndStatus)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    this->ShowWindow(nCmdShow);

    SetForegroundWindow(_hwnd);

    SendMessage(_hwndToolbar, TB_CHECKBUTTON, (WPARAM)IDM_PAD_USESHDOCVW, _fUseShdocvw);

Cleanup:
    RRETURN(hr);
}


HRESULT
CPadDoc::RegisterPadWndClass()
{
    if(!s_fPadWndClassRegistered)
    {
        LOCK_GLOBALS;

        HDC hdc = GetDC(NULL);
        s_fPaletteDevice = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
        extern HRESULT InitPalette();
        InitPalette();
        ReleaseDC(NULL, hdc);

        if(!s_fPadWndClassRegistered)
        {
            WNDCLASS    wc;

            memset(&wc, 0, sizeof(wc));
            wc.lpfnWndProc = CPadDoc::WndProc;                                                                            // windows of this class.
            wc.hInstance = g_hInstCore;
            wc.hIcon = LoadIcon(g_hInstResource, MAKEINTRESOURCE(IDR_PADICON));
            wc.lpszMenuName =  MAKEINTRESOURCE(IDR_PADMENU);
            wc.lpszClassName = SZ_PAD_WNDCLASS;
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);

            if (!RegisterClass(&wc))
            {
                return E_FAIL;
            }
            s_fPadWndClassRegistered = TRUE;
        }
    }
    return S_OK;
}

void
CPadDoc::SetStatusText(LPCTSTR pchStatusText)
{
    TCHAR * pchar;

    if (!_fDisablePadEvents)
    {
        if (_pEvent && pchStatusText && *pchStatusText)
        {
            _pEvent->Event(pchStatusText);
        }

        FireEvent(DISPID_PadEvents_Status, pchStatusText);
    }

    if ((!pchStatusText || _tcscmp(pchStatusText, _T("Ready")) == 0) &&
        _pScriptSite &&
        _pScriptSite->_achPath[0])
    {
        TCHAR achBuf[MAX_PATH];
        TCHAR *pch;

        _tcscpy(achBuf, _T("Script: "));
        pch = _tcsrchr(_pScriptSite->_achPath, FILENAME_SEPARATOR);
        _tcscat(achBuf, pch ? pch + 1 : _pScriptSite->_achPath);
        SetWindowText(_hwndStatus, achBuf);

        pchar = achBuf;
    }
    else
    {
        SetWindowText(_hwndStatus, pchStatusText);

        pchar = const_cast<TCHAR *>(pchStatusText);
    }

    if (IsTagEnabled(tagStatus))
    {
        char    achStatus[256]= "";   // for log status
        if (pchar)
        {
            WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pchar,
                    _tcslen(pchar),
                    achStatus,
                    256,
                    NULL,
                    NULL);
        }
        TraceTag((tagStatus, "%s", achStatus));
    }
}

void
CPadDoc::GetViewRect(RECT *prc, BOOL fIncludeObjectAdornments)
{
    RECT rcToolbar;
    RECT rcFormatToolbar;
    RECT rcStatus;

    GetClientRect(_hwnd, prc);

    if (_hwndToolbar)
    {
        GetWindowRect(_hwndToolbar, &rcToolbar);
        prc->top += rcToolbar.bottom - rcToolbar.top;
    }
    if ((_hwndFormatToolbar) && ( ! _fToolbarhidden) )
    {
        GetWindowRect(_hwndFormatToolbar, &rcFormatToolbar);
        prc->top += rcFormatToolbar.bottom - rcFormatToolbar.top;
    }
    
    if (_hwndStatus)
    {
        GetWindowRect(_hwndStatus, &rcStatus);
        prc->bottom += rcStatus.top - rcStatus.bottom;
    }

    if (fIncludeObjectAdornments)
    {
        if ( !_fToolbarhidden )
        {
            prc->top    += (_bwToolbarSpace.top * 2) ;
            prc->bottom -= (_bwToolbarSpace.bottom * 2) ;
            prc->left   += ( _bwToolbarSpace.left * 2)  ;
            prc->right  -= ( _bwToolbarSpace.right * 2);
        }
        else
        {
            prc->top    += _bwToolbarSpace.top  ;
            prc->bottom -= _bwToolbarSpace.bottom ;
            prc->left   += _bwToolbarSpace.left ;
            prc->right  -= _bwToolbarSpace.right ;
        }

    }

    if (prc->bottom < prc->top)
        prc->bottom = prc->top;

    if (prc->right < prc->left)
        prc->right = prc->left;
}

void
CPadDoc::SendAmbientPropertyChange(DISPID dispid)
{
    IOleControl *pControl;

    if ( _pObject &&
         OK(_pObject->QueryInterface(IID_IOleControl, (void **)&pControl)))
    {
        pControl->OnAmbientPropertyChange(dispid);
        pControl->Release();
    }
}

void
CPadDoc::Resize()
{
    RECT rc;

    GetViewRect(&rc, TRUE);

    if (_pView)
    {
        // It's an ActiveX Document.

        IGNORE_HR(_pView->SetRect(&rc));
    }
    else if (_pInPlaceObject)
    {
        // It's an ActiveX Control.

        SIZEL sizel;

        HDC hdc = GetDC(_hwnd);
        sizel.cx = MulDiv(rc.right - rc.left, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
        sizel.cy = MulDiv(rc.bottom - rc.top, 2540, GetDeviceCaps(hdc, LOGPIXELSY));
        ReleaseDC(_hwnd, hdc);

        IGNORE_HR(_pObject->SetExtent(DVASPECT_CONTENT, &sizel));
        IGNORE_HR(_pInPlaceObject->SetObjectRects(ENSUREOLERECT(&rc), ENSUREOLERECT(&rc)));
    }
}

LRESULT
CPadDoc::OnClose()
{
    if (QuerySave(SAVEOPTS_PROMPTSAVE) == S_OK)
    {
        this->ShowWindow(SW_HIDE);
    }
    return 0;
}

LRESULT
CPadDoc::OnDestroy()
{
    if (_pInPlaceObject)
    {
        IGNORE_HR(_pInPlaceObject->InPlaceDeactivate());
    }

    Assert(_PNS._dwCookie == 0);

    ClearInterface(&_pObject);

    if (_fVisible)
    {
        _fVisible = FALSE;
        Release();
    }
    return 0;
}

LRESULT
CPadDoc::OnActivate(WORD wFlags)
{
    HRESULT hr;

#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
    SetControlPaletteOwner(_hwnd);
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED

    if (_pInPlaceActiveObject)
    {
        hr = THR(_pInPlaceActiveObject->OnFrameWindowActivate(wFlags != WA_INACTIVE));
    }

    _fActive = (wFlags != WA_INACTIVE);

    return 0;
}

LRESULT
CPadDoc::OnSize(WORD fwSizeType, WORD nWidth, WORD nHeight)
{
    RECT rc;

    if (_hwndToolbar)
    {
        PostMessage(_hwndToolbar, WM_SIZE, fwSizeType, MAKELONG(nWidth, nHeight));
    }
    if ( (_hwndFormatToolbar) && ( ! _fToolbarhidden ))
    {
        PostMessage(_hwndFormatToolbar, WM_SIZE, fwSizeType, MAKELONG(nWidth, nHeight));
    }
    if (_hwndStatus)
    {
        PostMessage(_hwndStatus, WM_SIZE, fwSizeType, MAKELONG(nWidth, nHeight));
    }

    GetViewRect(&rc, FALSE);
    if (_pInPlaceActiveObject)
    {
        THR_NOTRACE(_pInPlaceActiveObject->ResizeBorder(
                ENSUREOLERECT(&rc),
                &_Frame,
                TRUE));
    }

    Resize();

    return 0;
}

void
CPadDoc::UpdateFontSizeBtns(IOleCommandTarget *pCommandTarget)
{
    VARIANTARG var;
    MSOCMD msocmd;
    HRESULT hr;

    _iZoom = _iZoomMax = _iZoomMin = 0;
    msocmd.cmdID = OLECMDID_ZOOM;
    msocmd.cmdf  = 0;

    hr = pCommandTarget->QueryStatus(
            NULL,
            1,
            &msocmd,
            NULL);
    if (hr)
        goto Cleanup;

    if (msocmd.cmdf && msocmd.cmdf != MSOCMDSTATE_DISABLED)
    {
        var.vt   = VT_I4;
        var.lVal = 0;
        pCommandTarget->Exec(
                NULL,
                OLECMDID_ZOOM,
                MSOCMDEXECOPT_DONTPROMPTUSER,
                NULL,
                &var);
        if (var.vt == VT_I4)
            _iZoom = var.lVal;

        var.vt   = VT_I4;
        var.lVal = 0;
        pCommandTarget->Exec(
                NULL,
                OLECMDID_GETZOOMRANGE,
                MSOCMDEXECOPT_DONTPROMPTUSER,
                NULL,
                &var);
        if (var.vt == VT_I4)
        {
            _iZoomMin = (int)(short)LOWORD(var.lVal);
            _iZoomMax = (int)(short)HIWORD(var.lVal);
        }
    }

Cleanup:
    SendMessage(
            _hwndToolbar,
            TB_ENABLEBUTTON,
            IDM_PAD_FONTINC,
            (LPARAM) MAKELONG((_iZoom < _iZoomMax), 0));
    SendMessage(
            _hwndToolbar,
            TB_PRESSBUTTON,
            IDM_PAD_FONTINC,
            (LPARAM) MAKELONG(FALSE, 0));
    SendMessage(
            _hwndToolbar,
            TB_ENABLEBUTTON,
            IDM_PAD_FONTDEC,
            (LPARAM) MAKELONG((_iZoom > _iZoomMin), 0));
    SendMessage(
            _hwndToolbar,
            TB_PRESSBUTTON,
            IDM_PAD_FONTDEC,
            (LPARAM) MAKELONG(FALSE, 0));
}


struct MsoCmdInfo {
        UINT localIDM;
        UINT MsoCmdIDM;
        const GUID * MsoCmdGUID;
};

BOOL
CPadDoc::IsDebugWindowVisible()
{
    return _pDebugWindow->_hwnd && IsWindowVisible(_pDebugWindow->_hwnd);
}

void
CPadDoc::ToggleDebugWindowVisibility()
{
    HRESULT hr;

    if (_pDebugWindow == NULL)
    {
        _pDebugWindow = new CDebugWindow (this);

        hr = THR(_pDebugWindow->Init());
        if(hr)
        {
            delete _pDebugWindow;
            _pDebugWindow = NULL;
            return;
        }
    }

    ::ShowWindow(_pDebugWindow->_hwnd, IsDebugWindowVisible() ? SW_HIDE : SW_SHOW);
}



static INT_PTR CALLBACK
WelcomeProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CPadDoc *pDoc = (CPadDoc *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        break;

    case WM_COMMAND:
        pDoc->OnCommand(GET_WM_COMMAND_CMD(wParam, lParam),
                        GET_WM_COMMAND_ID(wParam, lParam), 
                        GET_WM_COMMAND_HWND(wParam, lParam));
        break;

    case WM_ERASEBKGND:
        SendMessage(pDoc->_hwnd, msg, wParam, lParam);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void
CPadDoc::Welcome()
{
    Assert(!_hwndWelcome);
    _hwndWelcome = CreateDialogParam(
            g_hInstResource,
            MAKEINTRESOURCE(IDR_WELCOME_DLG),
            _hwnd,
            &WelcomeProc,
            (LPARAM)this);
}

LRESULT
CPadDoc::OnCommand(WORD wNotifyCode, WORD idm, HWND hwndCtl)
{
    HRESULT hr = S_OK;
    IOleCommandTarget *pCommandTarget;
    DWORD               nCmdexecopt = MSOCMDEXECOPT_DONTPROMPTUSER;
        
    static const MsoCmdInfo CmdInfo[] = {
            { IDM_PAD_STOP,       OLECMDID_STOP,      NULL         },
            { IDM_PAD_PRINT,      OLECMDID_PRINT,     NULL         },
            { IDM_PAD_PAGESETUP,  OLECMDID_PAGESETUP, NULL         },
            { IDM_PAD_CUT,        OLECMDID_CUT,       NULL         },
            { IDM_PAD_COPY,       OLECMDID_COPY,      NULL         },
            { IDM_PAD_PASTE,      OLECMDID_PASTE,     NULL         },
            { IDM_PAD_SAVE,       OLECMDID_SAVE,      NULL         },
            { IDM_PAD_SAVEAS,     OLECMDID_SAVEAS,    NULL         },
            { IDM_LAUNCHDEBUGGER, IDM_LAUNCHDEBUGGER, &CGID_MSHTML },
            { 0, 0, NULL }
    };

    switch (idm)
    {
    case IDM_PAD_USESHDOCVW:
        _fUseShdocvw = !_fUseShdocvw;
        SendMessage(_hwndToolbar, TB_CHECKBUTTON, (WPARAM)IDM_PAD_USESHDOCVW, (LPARAM)MAKELONG(_fUseShdocvw, 0));
        break;

    case IDM_PAD_BACK:
        if (_pBrowser)
        {
            hr = THR(_pBrowser->GoBack());
            ApplyZoomPercent();
        }
        return 0;

    case IDM_PAD_FORWARD:
        if (_pBrowser)
        {
            hr = THR(_pBrowser->GoForward());
            ApplyZoomPercent();
        }
        return 0;

    case IDM_PAD_HOME:
        if (_pBrowser)
        {
            hr = THR(_pBrowser->GoHome());
        }
        break;

    case IDM_PAD_FIND:
        if (_pBrowser)
        {
            hr = THR(_pBrowser->GoSearch());
        }
        break;

    case IDM_PAD_CLOSE:
        CloseFile();
        CloseScripts();
        UnregisterLocalCLSIDs();
        CoFreeUnusedLibraries();
        break;

    case IDM_PAD_EXIT:
        SendMessage(_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
        break;

    case IDM_PAD_NEW_HTML:
        hr = THR(QuerySave(SAVEOPTS_PROMPTSAVE));
        if ( hr )
            goto Cleanup;

        hr = THR(Open(CLSID_HTMLDocument));
        break;

    case IDM_PAD_OPEN_FILE:
        hr = THR(PromptOpenFile(_hwnd, &CLSID_HTMLDocument));
        break;

    case IDM_PAD_OPEN_URL:
        hr = THR(PromptOpenURL(_hwnd, &CLSID_HTMLDocument));
        break;

    case IDM_PAD_OPENNSCP:
        hr = THR(PromptOpenFile(_hwnd, &CLSID_NSCP));
        break;

    case IDM_PAD_EXECUTE_SCRIPT:
        hr = THR(PromptExecuteScript(FALSE));
        break;

    case IDM_PAD_EXECUTE_DRT:
        hr = THR(ExecuteDRT());
        break;

    case IDM_PAD_SAVE:
        hr = THR(DoSave(FALSE));
        if ( hr == OLECMDERR_E_CANCELED )
        {
            hr = S_OK;
        }
        break;

    case IDM_PAD_EDITBROWSE:
        IHTMLDocument2 *pOmDoc;

        if (_pObject && S_OK == get_Document((LPDISPATCH *)&pOmDoc))
        {
            _fUserMode = !_fUserMode;
            pOmDoc->put_designMode(_fUserMode ? _T("off") : _T("on"));
            pOmDoc->Release();
            UpdateFormatToolbar();
        }
        break;

    case IDM_PAD_ABOUT:
        #define STRINGIZE2(a) #a
        #define STRINGIZE1(a) _T(STRINGIZE2(a))
        MessageBoxA(
            _hwnd,
            "Microsoft Trident Version " VER_PRODUCTVERSION_STR
            "\r\r"
            "/register - Register Pad Document\r"
            "/mail - Register Trident Exchange Form\r"
            "/nomail - Unregister Trident Exchange Form\r"
            "/local - Register Local MSHTML\r"
            "/system - Register System MSHTML\r"
            "/s - Do not display success message for above commands\r"
            "/n - Open new document\r"
            "/k - Keep running after script completes\r"
            "/l <fname> - Use <fname> to store log\r"
            "/x <fname> - Execute script in <fname>\r"
            "/trace - Show trace dialog\r"
            "/shdocvw - Host shdocvw in Pad\r"
            "/loadsystem - Load MSHTML from system directory\r",
            "About HTMLPad",
            MB_APPLMODAL | MB_OK);
        break;

#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
    case IDM_PAD_TOOLBOX:
        ToggleControlPaletteVisibility();
        break;
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED

    case IDM_PAD_IMMEDIATE_WINDOW:
        ToggleDebugWindowVisibility();
        break;

    case IDM_PAD_SCRUB:
        ScrubRegistry();
        break;

    case IDM_PAD_REGISTER_PAD:
        hr = THR(RegisterPad());
        break;

    case IDM_PAD_REGISTER_LOCAL_TRIDENT:
        hr = THR(RegisterTrident(_hwnd, TRUE, FALSE));
        break;

    case IDM_PAD_REGISTER_SYSTEM_TRIDENT:
        hr = THR(RegisterTrident(_hwnd, TRUE, TRUE));
        break;

    case IDM_PAD_DEBUG_TRACE:
        hr = THR(RegisterLocalCLSIDs());
        if (hr == S_OK)
        {
            DbgExDoTracePointsDialog(FALSE);
        }
        break;

#if DBG==1
    case IDM_PAD_TESTNILE:
        hr = THR(TestNile());
        break;
#endif

    case IDM_PAD_VIEW_MON:
        if (_pObject)
        {
            DbgExOpenViewObjectMonitor(_hwnd, _pObject, TRUE);
        }
        break;

    case IDM_PAD_MEM_MON:
        DbgExOpenMemoryMonitor();
        break;

    case IDM_PAD_METERS:
        DbgExMtOpenMonitor();
        break;

    case IDM_PAD_NEWWIN:
        {
            CThreadProcParam tpp(_fUseShdocvw);
            hr = THR(CreatePadDoc(&tpp, NULL));
        }
        break;

    case IDM_PAD_CLEAR_CACHE:
        hr = THR(ClearDownloadCache());
        break;

    case IDM_PAD_REFRESH:
        if ( _pInPlaceObject &&
             OK(_pInPlaceObject->QueryInterface(
                IID_IOleCommandTarget, (void **)&pCommandTarget)) )
        {
            VARIANT var;

            VariantInit(&var);
            var.vt = VT_I4;

            if (GetAsyncKeyState(VK_CONTROL) < 0)
                var.lVal = OLECMDIDF_REFRESH_COMPLETELY;
            else
                var.lVal = OLECMDIDF_REFRESH_NO_CACHE;

            hr = pCommandTarget->Exec(NULL, OLECMDID_REFRESH,
                    MSOCMDEXECOPT_DONTPROMPTUSER, &var, NULL);

            pCommandTarget->Release();
        }

        if (_pEvent)
            _pEvent->Event(_T("Refresh"), TRUE);

        break;

    case IDM_PAD_CUT:
    case IDM_PAD_COPY:
    case IDM_PAD_PASTE:
    case IDM_PAD_PAGESETUP:
    case IDM_PAD_PRINT:
    case IDM_PAD_STOP:
    case IDM_LAUNCHDEBUGGER:

	
        if ( _pInPlaceObject &&
             OK(_pInPlaceObject->QueryInterface(
                IID_IOleCommandTarget, (void **)&pCommandTarget)) )
        {
            int i;

            for (i = 0; CmdInfo[i].localIDM && CmdInfo[i].localIDM != idm; i ++);
            if (CmdInfo[i].localIDM) // CmdInfo[i].localIDM == idm
            {
                hr = pCommandTarget->Exec(
                        CmdInfo[i].MsoCmdGUID,
                        CmdInfo[i].MsoCmdIDM,
                        idm == IDM_PAD_PRINT ? 0 : MSOCMDEXECOPT_DONTPROMPTUSER,
                        NULL,
                        NULL);

            }
            pCommandTarget->Release();
        }
        break;

    case IDM_IMAGE:
    case IDM_INSERTOBJECT:    
    case IDM_FONT:
    case IDM_FIND :
    case IDM_REPLACE :
    case IDM_HYPERLINK :
          nCmdexecopt = 0;
                        // fall thru.
                        
    //
    // Edit Menu
    //
    case IDM_UNDO :
    case IDM_REDO :
    case IDM_CUT :
    case IDM_COPY :
    case IDM_PASTE :
    case IDM_PASTEINSERT :
    case IDM_DELETE :
    case IDM_SELECTALL :

    case IDM_BOOKMARK :

    case IDM_UNLINK :
    case IDM_UNBOOKMARK :

    //
    // View Menu
    //

    case IDM_TOOLBARS :
    case IDM_STATUSBAR :
    case IDM_FORMATMARK :
    case IDM_TEXTONLY :
    case IDM_VIEWSOURCE:
    case IDM_BASELINEFONT5 :
    case IDM_BASELINEFONT4 :
    case IDM_BASELINEFONT3 :
    case IDM_BASELINEFONT2 :
    case IDM_BASELINEFONT1 :
    case IDM_EDITSOURCE :
    case IDM_FOLLOWLINKC :
    case IDM_FOLLOWLINKN :
    case IDM_PROPERTIES :
    case IDM_OPTIONS :
    
    //
    //
    // Insert Menu
    //
    case IDM_HORIZONTALLINE:
    case IDM_LINEBREAKNORMAL:
	case IDM_LINEBREAKLEFT:
	case IDM_LINEBREAKRIGHT:
	case IDM_LINEBREAKBOTH:
	case IDM_NONBREAK:
	case IDM_PAGEBREAK:
	case IDM_SPECIALCHAR:
	case IDM_MARQUEE:
	case IDM_1D:
	case IDM_TEXTBOX:
	case IDM_TEXTAREA:
#ifdef NEVER	
	case IDM_HTMLAREA:
#endif	
	case IDM_CHECKBOX:
	case IDM_RADIOBUTTON:
	case IDM_DROPDOWNBOX:
	case IDM_LISTBOX:
	case IDM_BUTTON:

	case IDM_IFRAME:

    //
    // Format Menu
    //
    case IDM_DIRLTR:
    case IDM_DIRRTL:
    case IDM_BLOCKDIRLTR:
    case IDM_BLOCKDIRRTL:

    case IDM_INDENT:
	case IDM_OUTDENT:
	
	{
       if ( IsEdit())
       {
            if ( _pInPlaceObject &&
             OK(_pInPlaceObject->QueryInterface(
                IID_IOleCommandTarget, (void **)&pCommandTarget)) )
            {

                hr = pCommandTarget->Exec(
                        (GUID *)&CGID_MSHTML,
                        idm,
                        nCmdexecopt,
                        NULL,
                        NULL);
                        
                pCommandTarget->Release();
             }
            
        }
	}
	break;

    case IDM_PAD_SAVEAS:
        hr = THR(DoSave(TRUE));
        if ( hr == OLECMDERR_E_CANCELED )
        {
            hr = S_OK;
        }
        break;

    case IDM_PAD_RELOADHIST:
        hr = THR(DoReloadHistory());
        break;

#ifdef IE5_ZOOM

    case IDM_ZOOMPERCENT:
        hr = OnStandardCombobox(wNotifyCode, idm, hwndCtl );
        break;

#endif  // IE5_ZOOM

    case IDM_BLOCKFMT:
    case IDM_FONTSIZE:
    case IDM_FONTNAME:
    case IDM_FORECOLOR:
    {
        hr = OnFormatCombobox(wNotifyCode, idm, hwndCtl );
    }

    // fall through

    case IDM_PAD_FONTINC:
    case IDM_PAD_FONTDEC:
        VARIANTARG var;
        BOOL       fExec = TRUE;

        if ( _pInPlaceObject &&
             OK(_pInPlaceObject->QueryInterface(
                IID_IOleCommandTarget, (void **)&pCommandTarget)) )
        {
            var.vt = VT_I4;
            var.lVal = 0;
            hr = pCommandTarget->Exec(
                    NULL,
                    OLECMDID_ZOOM,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    &var);
            if (hr)
                goto Cleanup;

            if (var.vt == VT_I4)
                _iZoom = var.lVal;

            if (idm == IDM_PAD_FONTINC && _iZoom < _iZoomMax)
            {
                var.lVal = _iZoom + 1;
            }
            else if (idm == IDM_PAD_FONTDEC && _iZoom > _iZoomMin)
            {
                var.lVal = _iZoom - 1;
            }
            else
            {
                fExec = FALSE;
            }

            if (fExec)
            {
                pCommandTarget->Exec(
                        NULL,
                        OLECMDID_ZOOM,
                        MSOCMDEXECOPT_DONTPROMPTUSER,
                        &var,
                        NULL);
            }

            pCommandTarget->Release();
        }

        break;
    }



Cleanup:
    if (hr)
    {
        CheckError(_hwnd, hr);
    }

    return 0;
}

HRESULT 
CPadDoc::OnFormatCombobox(WORD wNotifyCode, WORD idm, HWND hwndCtl )
{
    BOOL fRestoreFocus = FALSE ;
    VARIANTARG  var;
    TCHAR achBuffer[64];
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT hr = S_OK;
    VARIANTARG *pvarIn  = NULL;

    switch (idm)
    {
    case IDM_FONTSIZE:
        if (wNotifyCode != CBN_SELENDOK)
            return 0;
        fRestoreFocus = TRUE;
        GetWindowText(
                _hwndComboSize,
                achBuffer,
                ARRAY_SIZE(achBuffer));
        V_VT(&var)   = VT_I4;
        V_I4(&var) = _wtoi(achBuffer);
        pvarIn   = &var;
        break;

    case IDM_BLOCKFMT:
        if (wNotifyCode != CBN_SELENDOK)
            return 0;
        fRestoreFocus = TRUE;
        *(DWORD *)achBuffer =
        GetWindowText(
                _hwndComboTag,
                achBuffer + sizeof(DWORD) / sizeof(TCHAR),
                ARRAY_SIZE(achBuffer) - sizeof(DWORD) / sizeof(TCHAR));
        V_VT(&var)      = VT_BSTR;
        THR(FormsAllocString(achBuffer + sizeof(DWORD) / sizeof(TCHAR),
                             &V_BSTR(&var)));
        pvarIn      = &var;
        break;

    case IDM_FONTNAME:
        if (wNotifyCode != CBN_SELENDOK)
            return 0;
        fRestoreFocus = TRUE;
        *(DWORD *)achBuffer =
        GetWindowText(
                _hwndComboFont,
                achBuffer + sizeof(DWORD) / sizeof(TCHAR),
                ARRAY_SIZE(achBuffer) - sizeof(DWORD) / sizeof(TCHAR));
        V_VT(&var)      = VT_BSTR;
        THR(FormsAllocString(achBuffer + sizeof(DWORD) / sizeof(TCHAR),
                             &V_BSTR(&var)));
        pvarIn      = &var;
        break;

    case IDM_FORECOLOR:
        if (wNotifyCode != CBN_SELENDOK)
            return 0;
        fRestoreFocus = TRUE;
        V_I4(&var) = SendMessage(
                _hwndComboColor,
                CB_GETITEMDATA,
                (WPARAM) SendMessage(_hwndComboColor, CB_GETCURSEL, 0, 0),
                0);
        V_VT(&var)   = VT_I4;
        pvarIn      = &var;
        break;

    }
    
    if ( !_pInPlaceObject )
        goto Cleanup;
    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if ( hr )
        goto Cleanup;

    hr = pCommandTarget->Exec((GUID *)&CGID_MSHTML, idm, 0, pvarIn, NULL);

    //
    // When the user selects a combo item, pop the focus pack into the document.
    //
    if (fRestoreFocus)
    {
        //
        // marka can Pad Host Chrome ? Should we care about editing UI
        // inside of Chrome ? (ie not in trident). I have commented
        // out the Chrome GUNK for now.
        //

        // CHROME
        // If Chrome hosted there is no valid HWND so ::SetFocus() cannot
        // be used. Instead use CServer::SetFocus() which handles the 
        // windowless case
        //if (!IsChromeHosted())

        HWND    hWndInPlace;

        _pInPlaceObject->GetWindow(&hWndInPlace);
        if (hWndInPlace)
        {
            ::SetFocus (hWndInPlace);
        }

        //else
            //SetFocus (TRUE);                 
    }

Cleanup:
    if ( pCommandTarget) pCommandTarget->Release();

    RRETURN ( hr );
}

HRESULT 
CPadDoc::OnStandardCombobox(WORD wNotifyCode, WORD idm, HWND hwndCtl )
{
    IOleCommandTarget * pCommandTarget = NULL;
    BOOL                fRestoreFocus = FALSE;
    VARIANTARG          var;
    VARIANTARG          *pvarIn  = NULL;
    WPARAM              wCurSel;
    HRESULT             hr = S_OK;

    switch (idm)
    {
    case IDM_ZOOMPERCENT:
        if (wNotifyCode != CBN_SELENDOK)
            return 0;
        fRestoreFocus = TRUE;
        wCurSel = (WPARAM)::SendMessage(_hwndComboZoom, CB_GETCURSEL, 0, 0);
        if (wCurSel == CB_ERR)
        {
            TCHAR   szText[256];
            DWORD   dwZoomPercent;

            ::SendMessage(_hwndComboZoom, WM_GETTEXT, (WPARAM)(sizeof(szText) / sizeof(szText[0])), (LPARAM)&szText);
            dwZoomPercent = StrToInt(szText);

            if (dwZoomPercent < 1 || dwZoomPercent > 1000)
            {
                HWND    hWndInPlace;
                _pInPlaceObject->GetWindow(&hWndInPlace);

                ::MessageBox(_hwnd, 
                    TEXT("The number must be between 1 and 1000."), 
                    TEXT("MSHTMLPAD"), 
                    MB_ICONEXCLAMATION);

                //  Set default 100%
                dwZoomPercent = 100;
            }

            Format(0, szText, ARRAY_SIZE(szText), TEXT("<0d>%"), dwZoomPercent);
            ::SendMessage(_hwndComboZoom, WM_SETTEXT, 0, (LPARAM)&szText);

            V_I4(&var) = dwZoomPercent;
        }
        else
        {
            V_I4(&var) = ::SendMessage(_hwndComboZoom, CB_GETITEMDATA, wCurSel, 0);
        }

        V_VT(&var)   = VT_I4;
        pvarIn      = &var;
        break;
    }
    
    if ( !_pInPlaceObject )
        goto Cleanup;
    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if ( hr )
        goto Cleanup;

#ifdef IE5_ZOOM
    hr = pCommandTarget->Exec((GUID *)&CGID_MSHTML, idm, 0, pvarIn, NULL);
#endif

    //
    // When the user selects a combo item, pop the focus pack into the document.
    //
    if (fRestoreFocus)
    {
        //
        // marka can Pad Host Chrome ? Should we care about editing UI
        // inside of Chrome ? (ie not in trident). I have commented
        // out the Chrome GUNK for now.
        //

        // CHROME
        // If Chrome hosted there is no valid HWND so ::SetFocus() cannot
        // be used. Instead use CServer::SetFocus() which handles the 
        // windowless case
        //if (!IsChromeHosted())

        HWND    hWndInPlace;

        _pInPlaceObject->GetWindow(&hWndInPlace);
        if (hWndInPlace)
        {
            ::SetFocus (hWndInPlace);
        }

        //else
            //SetFocus (TRUE);                 
    }

Cleanup:
    if ( pCommandTarget) pCommandTarget->Release();

    RRETURN ( hr );
}

LRESULT
CPadDoc::OnInitMenuPopup(HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu)
{
    if (fSystemMenu)
        return 0;

#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
    CheckMenuItem(
            hmenuPopup,
            IDM_PAD_TOOLBOX,
            IsControlPaletteVisible() ? MF_BYCOMMAND | MF_CHECKED : MF_BYCOMMAND | MF_UNCHECKED);
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED

    static const MsoCmdInfo MenuItemInfo[] = {
            { IDM_PAD_PAGESETUP,    OLECMDID_PAGESETUP, NULL },
            { IDM_PAD_PRINT,        OLECMDID_PRINT,     NULL },
            { IDM_PAD_SHORTCUT,     IDM_UNKNOWN,        NULL },
            { IDM_PAD_PROPERTIES,   IDM_UNKNOWN,        NULL },
            { IDM_PAD_BACK,         IDM_UNKNOWN,        NULL },
            { IDM_PAD_FORWARD,      IDM_UNKNOWN,        NULL },
            { IDM_PAD_UPLEVEL,      IDM_UNKNOWN,        NULL },
            { IDM_PAD_OPTIONS,      IDM_UNKNOWN,        NULL },
            { IDM_PAD_OPENFAVORITE, IDM_UNKNOWN,        NULL },
            { IDM_PAD_ADDFAVORITE,  IDM_UNKNOWN,        NULL },
            { IDM_PAD_SAVE,         OLECMDID_SAVE,      NULL },
            { IDM_PAD_SAVEAS,       OLECMDID_SAVEAS,    NULL },
            { IDM_PAD_SENDFLOPPY,   IDM_UNKNOWN,        NULL },
            { IDM_PAD_SENDMAIL,     IDM_UNKNOWN,        NULL },
            { IDM_PAD_SENDBRIEFCASE,IDM_UNKNOWN,        NULL },
            { IDM_PAD_SENDWEB,      IDM_UNKNOWN,        NULL },
            { 0, 0, NULL }
    };

    //
    // marka Editing Commands
    //
    static const MsoCmdInfo EditMenuItemInfo[] = {                      
	        { IDM_HORIZONTALLINE,   IDM_UNKNOWN, NULL},
	        { IDM_LINEBREAKNORMAL,   IDM_UNKNOWN, NULL},
	        { IDM_LINEBREAKLEFT,   IDM_UNKNOWN, NULL},
	        { IDM_LINEBREAKRIGHT,   IDM_UNKNOWN, NULL},
	        { IDM_LINEBREAKBOTH,   IDM_UNKNOWN, NULL},
	        { IDM_NONBREAK,   IDM_UNKNOWN, NULL},
	        { IDM_PAGEBREAK,   IDM_UNKNOWN, NULL},
	        { IDM_SPECIALCHAR,   IDM_UNKNOWN, NULL},
	        { IDM_MARQUEE,   IDM_UNKNOWN, NULL},
	        { IDM_1D,   IDM_UNKNOWN, NULL},
	        { IDM_TEXTBOX,   IDM_UNKNOWN, NULL},
	        { IDM_TEXTAREA,   IDM_UNKNOWN, NULL},
#ifdef NEVER	        
	        { IDM_HTMLAREA,   IDM_UNKNOWN, NULL},
#endif	        
	        { IDM_CHECKBOX,   IDM_UNKNOWN, NULL},
	        { IDM_RADIOBUTTON,   IDM_UNKNOWN, NULL},
	        { IDM_DROPDOWNBOX,   IDM_UNKNOWN, NULL},
	        { IDM_LISTBOX,   IDM_UNKNOWN, NULL},
	        { IDM_BUTTON,   IDM_UNKNOWN, NULL},
	        { IDM_IMAGE,   IDM_UNKNOWN, NULL},
	        { IDM_IFRAME,   IDM_UNKNOWN, NULL},
	        { IDM_INSERTOBJECT,   IDM_UNKNOWN, NULL},
            { 0, 0, NULL }
    };
    
    int cMenuItem;

    HRESULT hr = S_OK;
    MSOCMD msocmd;
    IOleCommandTarget * pCommandTarget;

    if ( !_pInPlaceObject )
        goto Cleanup;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    //
    // bugbug - marka - why loop through all these commands if 
    // they're not on the current menu ? leave it for now
    // this will be moved around in beta 2
    //
    for (cMenuItem = 0; MenuItemInfo[cMenuItem].localIDM; cMenuItem ++)
    {
        msocmd.cmdID = MenuItemInfo[cMenuItem].MsoCmdIDM;
        msocmd.cmdf  = 0;

        hr = pCommandTarget->QueryStatus(
            MenuItemInfo[cMenuItem].MsoCmdGUID,
            1,
            &msocmd,
            NULL);

        switch (msocmd.cmdf)
        {
        case MSOCMDSTATE_UP:
        case MSOCMDSTATE_DOWN:
        case MSOCMDSTATE_NINCHED:
        case MSOCMDF_ENABLED:
            EnableMenuItem(
                    hmenuPopup,
                    MenuItemInfo[cMenuItem].localIDM,
                    MF_ENABLED);
            break;

        case MSOCMDSTATE_DISABLED:
        default:
            EnableMenuItem(
                    hmenuPopup,
                    MenuItemInfo[cMenuItem].localIDM,
                    MF_GRAYED);
            break;
        }
    }
    //
    // marka 
    //
    switch( uPos )
    {
        case 1:
            OnInitMenuEdit( uPos, hmenuPopup );
            break;
        case 2:
            OnInitMenuView( uPos, hmenuPopup );
            break;
        case 3:
            OnInitMenuInsert( uPos, hmenuPopup );
            break;
        case 4:
            OnInitMenuFormat( uPos, hmenuPopup );
            break;
    }
    pCommandTarget->Release();

Cleanup:
    return 0;
}

LRESULT 
CPadDoc::OnInitMenuEdit(UINT uItem, HMENU hmenu)
{
    HRESULT hr = S_OK;

        UINT    MenuItem [] =  {
        IDM_UNDO,
        IDM_REDO,
        IDM_CUT,
        IDM_COPY,
        IDM_PASTE,
        IDM_PASTEINSERT,
        IDM_DELETE,
        IDM_SELECTALL,
        IDM_FIND,
        IDM_REPLACE,

        IDM_BOOKMARK,
        IDM_HYPERLINK,
        IDM_UNLINK,
        IDM_UNBOOKMARK,
        0
    };

    EnableMenuItems( MenuItem , hmenu);
    RRETURN ( hr );
}

LRESULT 
CPadDoc::OnInitMenuView(UINT uItem, HMENU hmenu)
{
    HRESULT hr = S_OK;

    UINT    MenuItem [] =  {
         IDM_TOOLBARS,
        IDM_STATUSBAR,
        IDM_FORMATMARK,
        IDM_TEXTONLY,
        IDM_VIEWSOURCE,
        IDM_BASELINEFONT5,
        IDM_BASELINEFONT4,
        IDM_BASELINEFONT3,
        IDM_BASELINEFONT2,
        IDM_BASELINEFONT1,
        IDM_PAD_REFRESH,
        IDM_EDITSOURCE,
        IDM_FOLLOWLINKC,
        IDM_FOLLOWLINKN,
        IDM_PROPERTIES,
        IDM_OPTIONS,  
        0 };

    EnableMenuItems( MenuItem , hmenu);
    
    RRETURN ( hr );
}

LRESULT 
CPadDoc::OnInitMenuInsert(UINT uItem, HMENU hmenu)
{
    HRESULT hr = S_OK;
    
    EnableMenuItem(
        hmenu,
        IDM_PAGEBREAK,
        MF_GRAYED);
        
    EnableMenuItem(
        hmenu,
        IDM_SPECIALCHAR,
        MF_GRAYED);
        
    RRETURN ( hr );
}

LRESULT CPadDoc::OnInitMenuFormat(UINT uItem, HMENU hmenu)
{
    HRESULT hr = S_OK;

    UINT    MenuItem [] =  {       
        IDM_FONT,
        IDM_DIRLTR,
        IDM_DIRRTL,
        IDM_BLOCKDIRLTR,
        IDM_BLOCKDIRRTL,
        IDM_INDENT,
	    IDM_OUTDENT,
	    0 };

	EnableMenuItems( MenuItem, hmenu );
	  
    RRETURN ( hr );
}
    
LRESULT
CPadDoc::OnMenuSelect(UINT uItem, UINT fuFlags, HMENU hmenu)
{
    TCHAR achMessage[MAX_PATH];

    if (uItem == 0 && fuFlags == 0xFFFF)
    {
        _tcscpy(achMessage, TEXT("Ready"));
    }
    else if ((fuFlags & (MF_POPUP|MF_SYSMENU)) == 0 && uItem != 0)
    {
        LoadString(g_hInstResource, uItem, achMessage, ARRAY_SIZE(achMessage));
    }
    else
    {
        achMessage[0] = TEXT('\0');
    }

    SetStatusText(achMessage);
    return 0;
}

//+========================================================================
//
// Method: CPadDoc::EnableMenuItems
//
// Given an array of Commands - do a QueryStatus in Trident
// and activate / deactivate the corresponding MenuItem.
//
//-------------------------------------------------------------------------

HRESULT 
CPadDoc::EnableMenuItems( UINT * pMenuItem , HMENU hmenuPopup)
{
    int         cMenuItem;
    MSOCMD      msocmd;
    UINT        mf;
    IOleCommandTarget * pCommandTarget =NULL;
    HRESULT hr = S_OK;
    
    if (! _pInPlaceObject )
        goto Cleanup;
        
    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    for (cMenuItem = 0; pMenuItem[cMenuItem]; cMenuItem ++)
    {
        msocmd.cmdID = pMenuItem[cMenuItem];
        msocmd.cmdf  = 0;

        // Only if object is active
        if (_fActive)
        {
            hr = pCommandTarget->QueryStatus(
                    (GUID *)&CGID_MSHTML,
                    1,
                    &msocmd,
                    NULL);
        }

        switch (msocmd.cmdf)
        {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_NINCHED:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_UNCHECKED;
                break;

            case MSOCMDSTATE_DOWN:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_CHECKED;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                mf = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
                break;
        }
        CheckMenuItem(hmenuPopup, msocmd.cmdID, mf);
        EnableMenuItem(hmenuPopup, msocmd.cmdID, mf);
		hr = S_OK;
    }
Cleanup:
    if ( pCommandTarget ) pCommandTarget->Release();

    RRETURN ( hr );
}

LRESULT CALLBACK
ObjectWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    CPadDoc * pDoc = (CPadDoc *)GetProp(hwnd, _T("TRIDENT_DOC"));
    LRESULT l;

    if (!pDoc)
    {   // This is very unusual but happens in some scenarios
        // such as running activex.js stand alone.  We are not
        // 100% certain about the cuase but it happens in very
        // deep call stacks during complex shutdown.
        // -Tom
        return( DefWindowProc(hwnd, wm, wParam, lParam) );
    }

    switch(wm)
    {
    case WM_COMMAND:
        // check for justify changes
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDM_JUSTIFYRIGHT:
            case IDM_JUSTIFYLEFT:
            case IDM_JUSTIFYCENTER:
                BOOL fChecked;
                
                fChecked = SendMessage(GET_WM_COMMAND_HWND(wParam, lParam), 
                                       TB_ISBUTTONCHECKED, 
                                       (WPARAM)GET_WM_COMMAND_ID(wParam, lParam), 
                                       0);
                            
                if (!fChecked)
                    wParam = MAKEWPARAM(IDM_JUSTIFYNONE, LOWORD(wParam));
       }
    
       break;
        
    case WM_PAINT:
    {
        long lPaintArea=0;
        RECT rClient = {0};

        if (!pDoc->_fDisablePadEvents)
        {
            if (pDoc->_pEvent)
            {
                pDoc->_pEvent->Event(_T("Paint"));
            }
            pDoc->FireEvent(DISPID_PadEvents_Status, _T("Paint"));

            HRGN hrgn;
            int iRgnType;
            DWORD i;
            RECT rIntersect;

            struct REGION_DATA
            {
                RGNDATAHEADER rdh;
                RECT arc[64];
            } rd;

            Verify((hrgn = CreateRectRgnIndirect(&g_Zero.rc)) != NULL);
            Verify((iRgnType = GetUpdateRgn(hwnd, hrgn, FALSE)) != ERROR);
            Verify(GetClientRect(hwnd, &rClient));
            if (iRgnType != NULLREGION &&
                GetRegionData(hrgn, sizeof(rd), (RGNDATA *)&rd) &&
                rd.rdh.iType == RDH_RECTANGLES &&
                rd.rdh.nCount <= ARRAY_SIZE(rd.arc))
            {
                for (i=0; i<rd.rdh.nCount; i++)
                {
                    if (IntersectRect(&rIntersect, &rClient, &rd.arc[i]))
                    {
                        lPaintArea += (rIntersect.right - rIntersect.left)*(rIntersect.bottom - rIntersect.top);
                    }
                }
            }

            Verify(DeleteObject(hrgn));
        }

        l = CallWindowProc(pDoc->_pfnOrigWndProc, hwnd, wm, wParam, lParam);

        if (!pDoc->_fDisablePadEvents)
        {
            if (lPaintArea)
            {
                VARIANT vargs[2];
                VariantInit(vargs);
                V_VT(vargs) = VT_I4;
                V_I4(vargs) = lPaintArea;
                VariantInit(vargs+1);
                V_VT(vargs+1) = VT_I4;
                V_I4(vargs+1) = rClient.right * rClient.bottom;
                pDoc->FireEvent(DISPID_PadEvents_OnPaint, 2, vargs);
                VariantClear(vargs);
                VariantClear(vargs+1);
            }

            if (pDoc->_pEvent )
            {
                pDoc->_pEvent->Event(_T("After Paint"));
            }
        }

        return l;
        break;
    }
    }
    return CallWindowProc(pDoc->_pfnOrigWndProc, hwnd, wm, wParam, lParam);
}

LRESULT CALLBACK
CPadDoc::WndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    CPadDoc * pDoc;

    pDoc = (CPadDoc *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (wm)
    {
    case WM_NCCREATE:
        pDoc = (CPadDoc *) ((LPCREATESTRUCTW) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pDoc);
        pDoc->_hwnd = hwnd;
        pDoc->SubAddRef();
        break;

    case WM_NCDESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        pDoc->_hwnd = NULL;
        pDoc->SubRelease();
        break;

    case WM_GETMINMAXINFO:
        break;

    default:
        {
            LRESULT lResult;

            Assert(pDoc);
            lResult = pDoc->PadWndProc(hwnd, wm, wParam, lParam);
            return lResult;
        }
    }

    return DefWindowProc(hwnd, wm, wParam, lParam);
}

LRESULT
CPadDoc::PadWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    static UINT g_msgMouseWheel = 0;
    HWND cmdHWND    ;
    switch (wm)
    {
    case WM_SYSCOMMAND:
        if (SC_KEYMENU == (wParam & 0xFFF0) && InSendMessage())
        {
            PostMessage(hwnd, wm, wParam, lParam);
            return 0;
        }
        break;

    case WM_TIMER:
        FireEvent(DISPID_PadEvents_Timer, 0, NULL);
        break;

    case WM_DOKEYACION:
        DoKeyAction();
        break;

    case WM_RUNSCRIPT:
        FireEvent(DISPID_PadEvents_Load, 0, NULL);
        PopScript();
        break;

    case WM_ERASEBKGND:
        if(_pObject)
            return TRUE;
        break;

    case WM_INITMENU:
        _fObjectHelp = FALSE;
        break;

    case WM_INITMENUPOPUP:
        if ((HMENU) wParam == _hmenuObjectHelp)
        {
            HWND hwndDoc;

            _pInPlaceObject->GetWindow(&hwndDoc);
            return SendMessage(hwndDoc, wm, wParam, lParam);
        }
        else
        {

            return OnInitMenuPopup(
                    (HMENU) wParam,
                    LOWORD(lParam),
                    (BOOL) HIWORD(lParam));
        }

    case WM_MENUSELECT:
        UINT fuFlags;
        int  uItem;

        fuFlags = GET_WM_MENUSELECT_FLAGS(wParam, lParam);
        uItem   = GET_WM_MENUSELECT_CMD(wParam, lParam);

        // if fuFlag == 0x0000FFFF and (HWND) lParam == NULL, windows closes
        // the popup menu, do nothing.
        //
        if (fuFlags != 0x0000FFFF || GET_WM_MENUSELECT_HMENU(wParam, lParam))
        {
            if (fuFlags & MF_POPUP)
            {
                // selected menu item is a pop-up menu
                if (_hmenuObjectHelp && _hmenuHelp &&
                        _hmenuHelp == GET_WM_MENUSELECT_HMENU(wParam, lParam))
                {
                    _fObjectHelp =
                            (uItem == GetMenuItemCount(_hmenuHelp) - 1);
                }
                else
                {
                    _fObjectHelp = FALSE;
                }
            }
            else
            {
                // selected menu item is a command.

                _fObjectHelp = (GET_WM_MENUSELECT_HMENU(wParam, lParam)== _hmenuObjectHelp);
            }
        }

        if (_fObjectHelp)
        {
            HWND hwndDoc;

            _pInPlaceObject->GetWindow(&hwndDoc);
            return SendMessage(hwndDoc, wm, wParam, lParam);
        }
        else
        {
            return OnMenuSelect(uItem, fuFlags, GET_WM_MENUSELECT_HMENU(wParam, lParam));
        }

    case WM_COMMAND:
        {
            cmdHWND = GET_WM_COMMAND_HWND(wParam, lParam) ;
            if ( (_fObjectHelp && ( cmdHWND != _hwndToolbar) ) || ( cmdHWND == _hwndFormatToolbar) )
            {
                HWND hwndDoc;

                _pInPlaceObject->GetWindow(&hwndDoc);
                return SendMessage(hwndDoc, wm, wParam, lParam);
            }
            else
            {
                return OnCommand(GET_WM_COMMAND_CMD(wParam, lParam),
                                 GET_WM_COMMAND_ID(wParam, lParam), 
                                 GET_WM_COMMAND_HWND(wParam, lParam));
            }
        }
    case WM_HELP:
        MessageBox(NULL, _T("F1 pressed! If you got here, we bubble onhelp() OK!"), _T("Pad Help Diagnostic"), MB_ICONEXCLAMATION | MB_OK);
        break;

    case WM_DESTROY:
        OnDestroy();
        break;

    case WM_CLOSE:
        return OnClose();

    case WM_ACTIVATE:
        return OnActivate(LOWORD(wParam));

    case WM_SIZE:
        return OnSize((WORD)wParam, LOWORD(lParam), HIWORD(lParam));

#if defined(DBG_TOOLTIPS)
    case WM_NOTIFY:
        // BUGBUG (jenlc) should be replaced by QueryStatus calls.
        if ((TTN_NEEDTEXTA == ((LPNMHDR) lParam)->code) ||
                (TTN_NEEDTEXTW == ((LPNMHDR) lParam)->code))
        {
            TCHAR           szBuffer[256];
            TCHAR           szTemp[256];
            LPTOOLTIPTEXT   lpTooltipText = (LPTOOLTIPTEXT) lParam;
            int             id = lpTooltipText->hdr.idFrom;

            if (id == IDM_PAD_EDITBROWSE)
                id = _fUserMode ? IDM_PAD_EDIT : IDM_PAD_BROWSE;

            if (id == IDM_PAD_USESHDOCVW)
                id = _fUseShdocvw ? IDM_PAD_UNHOSTSHDOCVW : IDM_PAD_HOSTSHDOCVW;

            LoadString(
                g_hInstResource,
                IDS_TOOLTIP(id),
                szBuffer,
                ARRAY_SIZE(szBuffer));

            DWORD dwVersion;

            dwVersion = GetVersion();
            if (dwVersion >= 0x80000000)
            {
                _tcscpy(szTemp, szBuffer);
                WideCharToMultiByte(
                        CP_ACP,
                        0,
                        szTemp,
                        -1,
                        (char *) szBuffer,
                        sizeof(szBuffer),
                        NULL,
                        NULL);
            }
            lpTooltipText->lpszText = szBuffer;
        }
        break;
#endif

    case WM_PALETTECHANGED:
        if (_hwnd != (HWND)wParam)
            return OnPaletteChanged((HWND)wParam);
        break;
    case WM_QUERYNEWPALETTE:
        return OnQueryNewPalette();
        break;
    default:
        //if (g_msgMouseWheel == 0)
        if (g_msgMouseWheel == 0 && GetVersion() >= 0x80000000)
        {
            g_msgMouseWheel = RegisterWindowMessage(_T("MSWHEEL_ROLLMSG"));
        }
        if (wm == g_msgMouseWheel)
        {
            HWND hwndDoc;

            if (_pInPlaceObject)
            {
                _pInPlaceObject->GetWindow(&hwndDoc);
                return SendMessage(hwndDoc, wm, wParam, lParam);
            }
        }
    }

    return DefWindowProc(hwnd, wm, wParam, lParam);
}


BOOL
CPadDoc::OnTranslateAccelerator(MSG * pMsg)
{
    HRESULT hr = S_FALSE;

    if (pMsg->message >= WM_KEYFIRST &&
            pMsg->message <= WM_KEYLAST &&
            _pInPlaceActiveObject)
    {
        //  Hack (olego) we need to give a chance to Combo Zoom getting keyboard messages.
        HWND hwndFocus = ::GetFocus();
        HWND hwndParent = hwndFocus ? ::GetParent(hwndFocus) : NULL;

        if (_hwndComboZoom != hwndParent)
        {
            hr = THR(_pInPlaceActiveObject->TranslateAccelerator(pMsg));
        }
    }
    // Even if hr is not S_OK, the message may have been processed and resulted
    // in an error. In such a case, we don't want to pump the message in again !
    return (hr != S_FALSE);
}


HRESULT
CPadDoc::Open(IStream * pStream)
{
    HRESULT                 hr = S_OK;
    IUnknown *              pUnk = NULL;
    IOleObject *            pObject = NULL;
    IPersistStreamInit *    pPS = NULL;
    DWORD                   dwFlags;
    BOOL                    fInitNew = _fInitNew;

    // Shutdown the previous object.
    hr = THR(Deactivate());
    if (hr)
        goto Cleanup;

    if (!_fDisablePadEvents)
    {
        if (_pEvent)
        {
            _pEvent->Event(_T("Open"), TRUE);
        }

        FireEvent(DISPID_PadEvents_Status, _T("Open"));
    }

    hr = THR(RegisterLocalCLSIDs());
    if (hr)
        goto Cleanup;

    hr = THR(CoCreateInstance(
            CLSID_HTMLDocument,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IUnknown,
            (void **)&pUnk));
    if (hr)
        goto Cleanup;

    hr = pUnk->QueryInterface(IID_IOleObject, (void **)&pObject);
    if (hr)
        goto Cleanup;

    _fInitNew = !pStream;

    hr = THR(pObject->GetMiscStatus(DVASPECT_CONTENT, &dwFlags));
    if (hr)
        goto Cleanup;

    if (dwFlags & OLEMISC_SETCLIENTSITEFIRST)
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;
            
        if (_ulDownloadNotifyMask)
        {
            hr = THR(AttachDownloadNotify(pObject));
            if (hr)
                goto Cleanup;
        }
    }

    SendAmbientPropertyChange(DISPID_AMBIENT_USERMODE);

    hr = THR(pObject->QueryInterface(IID_IPersistStreamInit, (void **)&pPS));

    Assert(pPS);

    if(pStream)
    {
        hr = THR(pPS->Load(pStream));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pPS->InitNew());
        if (hr)
            goto Cleanup;
    }

    if (!(dwFlags & OLEMISC_SETCLIENTSITEFIRST))
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;
    }

    hr = THR(Activate(pObject));

Cleanup:
    if (hr)
    {
        _fInitNew = fInitNew;
    }

    ReleaseInterface(pObject);
    ReleaseInterface(pPS);
    ReleaseInterface(pUnk);
    RRETURN(hr);
}


HRESULT
CPadDoc::Save (IStream *pStream)
{
    HRESULT                 hr = S_OK;
    IPersistStreamInit *    pPS = NULL;

    if (!_pObject)
        goto Cleanup;

    hr = THR(_pObject->QueryInterface(IID_IPersistStreamInit, (void **)&pPS));
    if (hr)
        goto Cleanup;

    hr = THR(pPS->Save(pStream, TRUE));
    if (hr)
        goto Cleanup;

    _fInitNew = FALSE;

Cleanup:
    ReleaseInterface(pPS);
    RRETURN(hr);
}



HRESULT
CPadDoc::TimeSaveDocToDummyStream(LONG *plTimeMicros)
{
    LARGE_INTEGER liTimeStart;
    LARGE_INTEGER liTimeEnd;
    LARGE_INTEGER liFreq;
    IStream *pStreamDummy = NULL;
    IOleCommandTarget *pOCT = NULL;
    VARIANTARG varargIn;
    VARIANTARG varargOut;
    HRESULT hr;

    QueryPerformanceFrequency( &liFreq );
    if( liFreq.QuadPart == 0 )
    {
        AssertSz(0,"No high-performance timer available.");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Muck with document so that when we save it it actually comes out of the tree
    //

    hr = THR(_pObject->QueryInterface(IID_IOleCommandTarget, (void **)&pOCT));
    if (hr)
        goto Cleanup;

    // Set to design mode.
    hr = THR(pOCT->Exec(&CGID_MSHTML, 
                        IDM_EDITMODE, 
                        MSOCMDEXECOPT_DODEFAULT, 
                        &varargIn,
                        &varargOut) );
    if (hr)
        goto Cleanup;

    // Set dirty bit.
    varargIn.vt = VT_BOOL;
    varargIn.bVal = TRUE;

    hr = THR(pOCT->Exec(&CGID_MSHTML, 
                        IDM_SETDIRTY, 
                        MSOCMDEXECOPT_DODEFAULT, 
                        &varargIn,
                        &varargOut) );
    if (hr)
        goto Cleanup;

    //
    // Make the stream and time the saving.
    //

    CreateStreamOnHGlobal(NULL, TRUE, &pStreamDummy);

    QueryPerformanceCounter( &liTimeStart );
    hr = Save( pStreamDummy );
    QueryPerformanceCounter( &liTimeEnd );

    if( hr )
        goto Cleanup;

    Assert( plTimeMicros );
    *plTimeMicros = (liTimeEnd.QuadPart - liTimeStart.QuadPart) * 1000000 / liFreq.QuadPart;

Cleanup:
    ReleaseInterface( pStreamDummy );
    ReleaseInterface( pOCT );
    RRETURN(hr);
}


HRESULT
CPadDoc::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS pmgw)
{
    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT_PTR)GetSubMenu(_hmenuMain, 0),
            TEXT("&File"));
    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT_PTR)GetSubMenu(_hmenuMain, 1),
            TEXT("&Go"));
    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT_PTR)GetSubMenu(_hmenuMain, 2),
            TEXT("Favo&rite"));
    _hmenuHelp = LoadMenu(
            g_hInstResource,
            MAKEINTRESOURCE(IDR_PADHELPMENU));
    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT_PTR)_hmenuHelp,
            TEXT("&Help"));
    AppendMenu(
            hmenuShared,
            MF_BYPOSITION | MF_POPUP,
            (UINT_PTR)GetSubMenu(_hmenuMain, 4),
            TEXT(" "));

    _cMenuHelpItems = GetMenuItemCount(_hmenuHelp);

    pmgw->width[0] = 1;
    pmgw->width[2] = 0;
    pmgw->width[4] = 2;
    pmgw->width[5] = 2;

    return S_OK;
}

HRESULT
CPadDoc::UIActivateDoc(LPMSG pMsg)
{
    HRESULT hr;
    RECT rc;

    GetViewRect(&rc, TRUE);

    hr = THR(_pObject->DoVerb(
            OLEIVERB_UIACTIVATE,
            pMsg,
            &_Site,
            0,
            _hwnd,
            ENSUREOLERECT(&rc)));

    RRETURN(hr);
}

HRESULT
CPadDoc::UIDeactivateDoc()
{
    RRETURN(THR(_pInPlaceObject->UIDeactivate()));
}

void
CPadDoc::SetDocTitle(TCHAR * pchTitle)
{
    TCHAR szBuf[512];

    GetWindowText(_hwnd, szBuf, ARRAY_SIZE(szBuf));
    if (!_tcsequal(szBuf, pchTitle))
    {
        SetWindowText(_hwnd, pchTitle);
    }
    UpdateDirtyUI();    
}

HRESULT
CPadDoc::GetMoniker(LPMONIKER * ppmk)
{
    Assert (ppmk);
    if (_pMk)
    {
        * ppmk = _pMk;
        _pMk->AddRef();
        return S_OK;
    }
    else
    {
        * ppmk = NULL;
        RRETURN (E_FAIL);
    }
}

char *
DecodeMessage(UINT msg)
{
    switch (msg)
    {
        case WM_NULL:               return("WM_NULL");
        case WM_CREATE:             return("WM_CREATE");
        case WM_DESTROY:            return("WM_DESTROY");
        case WM_MOVE:               return("WM_MOVE");
        case WM_SIZE:               return("WM_SIZE");
        case WM_ACTIVATE:           return("WM_ACTIVATE");
        case WM_SETFOCUS:           return("WM_SETFOCUS");
        case WM_KILLFOCUS:          return("WM_KILLFOCUS");
        case WM_ENABLE:             return("WM_ENABLE");
        case WM_SETREDRAW:          return("WM_SETREDRAW");
        case WM_SETTEXT:            return("WM_SETTEXT");
        case WM_GETTEXT:            return("WM_GETTEXT");
        case WM_GETTEXTLENGTH:      return("WM_GETTEXTLENGTH");
        case WM_PAINT:              return("WM_PAINT");
        case WM_CLOSE:              return("WM_CLOSE");
        case WM_QUERYENDSESSION:    return("WM_QUERYENDSESSION");
        case WM_QUERYOPEN:          return("WM_QUERYOPEN");
        case WM_ENDSESSION:         return("WM_ENDSESSION");
        case WM_QUIT:               return("WM_QUIT");
        case WM_ERASEBKGND:         return("WM_ERASEBKGND");
        case WM_SYSCOLORCHANGE:     return("WM_SYSCOLORCHANGE");
        case WM_SHOWWINDOW:         return("WM_SHOWWINDOW");
        case WM_WININICHANGE:       return("WM_WININICHANGE");
        case WM_DEVMODECHANGE:      return("WM_DEVMODECHANGE");
        case WM_ACTIVATEAPP:        return("WM_ACTIVATEAPP");
        case WM_FONTCHANGE:         return("WM_FONTCHANGE");
        case WM_TIMECHANGE:         return("WM_TIMECHANGE");
        case WM_CANCELMODE:         return("WM_CANCELMODE");
        case WM_SETCURSOR:          return("WM_SETCURSOR");
        case WM_MOUSEACTIVATE:      return("WM_MOUSEACTIVATE");
        case WM_CHILDACTIVATE:      return("WM_CHILDACTIVATE");
        case WM_QUEUESYNC:          return("WM_QUEUESYNC");
        case WM_GETMINMAXINFO:      return("WM_GETMINMAXINFO");
        case WM_PAINTICON:          return("WM_PAINTICON");
        case WM_ICONERASEBKGND:     return("WM_ICONERASEBKGND");
        case WM_NEXTDLGCTL:         return("WM_NEXTDLGCTL");
        case WM_SPOOLERSTATUS:      return("WM_SPOOLERSTATUS");
        case WM_DRAWITEM:           return("WM_DRAWITEM");
        case WM_MEASUREITEM:        return("WM_MEASUREITEM");
        case WM_DELETEITEM:         return("WM_DELETEITEM");
        case WM_VKEYTOITEM:         return("WM_VKEYTOITEM");
        case WM_CHARTOITEM:         return("WM_CHARTOITEM");
        case WM_SETFONT:            return("WM_SETFONT");
        case WM_GETFONT:            return("WM_GETFONT");
        case WM_SETHOTKEY:          return("WM_SETHOTKEY");
        case WM_GETHOTKEY:          return("WM_GETHOTKEY");
        case WM_QUERYDRAGICON:      return("WM_QUERYDRAGICON");
        case WM_COMPAREITEM:        return("WM_COMPAREITEM");
        case WM_COMPACTING:         return("WM_COMPACTING");
        case WM_COMMNOTIFY:         return("WM_COMMNOTIFY");
        case WM_WINDOWPOSCHANGING:  return("WM_WINDOWPOSCHANGING");
        case WM_WINDOWPOSCHANGED:   return("WM_WINDOWPOSCHANGED");
        case WM_POWER:              return("WM_POWER");
        case WM_COPYDATA:           return("WM_COPYDATA");
        case WM_CANCELJOURNAL:      return("WM_CANCELJOURNAL");
        case WM_NOTIFY:             return("WM_NOTIFY");
        case WM_INPUTLANGCHANGEREQUEST: return("WM_INPUTLANGCHANGEREQUEST");
        case WM_INPUTLANGCHANGE:    return("WM_INPUTLANGCHANGE");
        case WM_TCARD:              return("WM_TCARD");
        case WM_HELP:               return("WM_HELP");
        case WM_USERCHANGED:        return("WM_USERCHANGED");
        case WM_NOTIFYFORMAT:       return("WM_NOTIFYFORMAT");
        case WM_CONTEXTMENU:        return("WM_CONTEXTMENU");
        case WM_STYLECHANGING:      return("WM_STYLECHANGING");
        case WM_STYLECHANGED:       return("WM_STYLECHANGED");
        case WM_DISPLAYCHANGE:      return("WM_DISPLAYCHANGE");
        case WM_GETICON:            return("WM_GETICON");
        case WM_SETICON:            return("WM_SETICON");
        case WM_NCCREATE:           return("WM_NCCREATE");
        case WM_NCDESTROY:          return("WM_NCDESTROY");
        case WM_NCCALCSIZE:         return("WM_NCCALCSIZE");
        case WM_NCHITTEST:          return("WM_NCHITTEST");
        case WM_NCPAINT:            return("WM_NCPAINT");
        case WM_NCACTIVATE:         return("WM_NCACTIVATE");
        case WM_GETDLGCODE:         return("WM_GETDLGCODE");
        case WM_SYNCPAINT:          return("WM_SYNCPAINT");
        case WM_NCMOUSEMOVE:        return("WM_NCMOUSEMOVE");
        case WM_NCLBUTTONDOWN:      return("WM_NCLBUTTONDOWN");
        case WM_NCLBUTTONUP:        return("WM_NCLBUTTONUP");
        case WM_NCLBUTTONDBLCLK:    return("WM_NCLBUTTONDBLCLK");
        case WM_NCRBUTTONDOWN:      return("WM_NCRBUTTONDOWN");
        case WM_NCRBUTTONUP:        return("WM_NCRBUTTONUP");
        case WM_NCRBUTTONDBLCLK:    return("WM_NCRBUTTONDBLCLK");
        case WM_NCMBUTTONDOWN:      return("WM_NCMBUTTONDOWN");
        case WM_NCMBUTTONUP:        return("WM_NCMBUTTONUP");
        case WM_NCMBUTTONDBLCLK:    return("WM_NCMBUTTONDBLCLK");
        case WM_KEYDOWN:            return("WM_KEYDOWN");
        case WM_KEYUP:              return("WM_KEYUP");
        case WM_CHAR:               return("WM_CHAR");
        case WM_DEADCHAR:           return("WM_DEADCHAR");
        case WM_SYSKEYDOWN:         return("WM_SYSKEYDOWN");
        case WM_SYSKEYUP:           return("WM_SYSKEYUP");
        case WM_SYSCHAR:            return("WM_SYSCHAR");
        case WM_SYSDEADCHAR:        return("WM_SYSDEADCHAR");
        case WM_IME_STARTCOMPOSITION:   return("WM_IME_STARTCOMPOSITION");
        case WM_IME_ENDCOMPOSITION: return("WM_IME_ENDCOMPOSITION");
        case WM_IME_COMPOSITION:    return("WM_IME_COMPOSITION");
        case WM_INITDIALOG:         return("WM_INITDIALOG");
        case WM_COMMAND:            return("WM_COMMAND");
        case WM_SYSCOMMAND:         return("WM_SYSCOMMAND");
        case WM_TIMER:              return("WM_TIMER");
        case WM_HSCROLL:            return("WM_HSCROLL");
        case WM_VSCROLL:            return("WM_VSCROLL");
        case WM_INITMENU:           return("WM_INITMENU");
        case WM_INITMENUPOPUP:      return("WM_INITMENUPOPUP");
        case WM_MENUSELECT:         return("WM_MENUSELECT");
        case WM_MENUCHAR:           return("WM_MENUCHAR");
        case WM_ENTERIDLE:          return("WM_ENTERIDLE");
        case WM_CTLCOLORMSGBOX:     return("WM_CTLCOLORMSGBOX");
        case WM_CTLCOLOREDIT:       return("WM_CTLCOLOREDIT");
        case WM_CTLCOLORLISTBOX:    return("WM_CTLCOLORLISTBOX");
        case WM_CTLCOLORBTN:        return("WM_CTLCOLORBTN");
        case WM_CTLCOLORDLG:        return("WM_CTLCOLORDLG");
        case WM_CTLCOLORSCROLLBAR:  return("WM_CTLCOLORSCROLLBAR");
        case WM_CTLCOLORSTATIC:     return("WM_CTLCOLORSTATIC");
        case WM_MOUSEMOVE:          return("WM_MOUSEMOVE");
        case WM_LBUTTONDOWN:        return("WM_LBUTTONDOWN");
        case WM_LBUTTONUP:          return("WM_LBUTTONUP");
        case WM_LBUTTONDBLCLK:      return("WM_LBUTTONDBLCLK");
        case WM_RBUTTONDOWN:        return("WM_RBUTTONDOWN");
        case WM_RBUTTONUP:          return("WM_RBUTTONUP");
        case WM_RBUTTONDBLCLK:      return("WM_RBUTTONDBLCLK");
        case WM_MBUTTONDOWN:        return("WM_MBUTTONDOWN");
        case WM_MBUTTONUP:          return("WM_MBUTTONUP");
        case WM_MBUTTONDBLCLK:      return("WM_MBUTTONDBLCLK");
        case WM_MOUSEWHEEL:         return("WM_MOUSEWHEEL");
        case WM_PARENTNOTIFY:       return("WM_PARENTNOTIFY");
        case WM_ENTERMENULOOP:      return("WM_ENTERMENULOOP");
        case WM_EXITMENULOOP:       return("WM_EXITMENULOOP");
        case WM_NEXTMENU:           return("WM_NEXTMENU");
        case WM_SIZING:             return("WM_SIZING");
        case WM_CAPTURECHANGED:     return("WM_CAPTURECHANGED");
        case WM_MOVING:             return("WM_MOVING");
        case WM_POWERBROADCAST:     return("WM_POWERBROADCAST");
        case WM_DEVICECHANGE:       return("WM_DEVICECHANGE");
        case WM_MDICREATE:          return("WM_MDICREATE");
        case WM_MDIDESTROY:         return("WM_MDIDESTROY");
        case WM_MDIACTIVATE:        return("WM_MDIACTIVATE");
        case WM_MDIRESTORE:         return("WM_MDIRESTORE");
        case WM_MDINEXT:            return("WM_MDINEXT");
        case WM_MDIMAXIMIZE:        return("WM_MDIMAXIMIZE");
        case WM_MDITILE:            return("WM_MDITILE");
        case WM_MDICASCADE:         return("WM_MDICASCADE");
        case WM_MDIICONARRANGE:     return("WM_MDIICONARRANGE");
        case WM_MDIGETACTIVE:       return("WM_MDIGETACTIVE");
        case WM_MDISETMENU:         return("WM_MDISETMENU");
        case WM_ENTERSIZEMOVE:      return("WM_ENTERSIZEMOVE");
        case WM_EXITSIZEMOVE:       return("WM_EXITSIZEMOVE");
        case WM_DROPFILES:          return("WM_DROPFILES");
        case WM_MDIREFRESHMENU:     return("WM_MDIREFRESHMENU");
        case WM_IME_SETCONTEXT:     return("WM_IME_SETCONTEXT");
        case WM_IME_NOTIFY:         return("WM_IME_NOTIFY");
        case WM_IME_CONTROL:        return("WM_IME_CONTROL");
        case WM_IME_COMPOSITIONFULL:    return("WM_IME_COMPOSITIONFULL");
        case WM_IME_SELECT:         return("WM_IME_SELECT");
        case WM_IME_CHAR:           return("WM_IME_CHAR");
        case WM_IME_KEYDOWN:        return("WM_IME_KEYDOWN");
        case WM_IME_KEYUP:          return("WM_IME_KEYUP");
        case WM_MOUSEHOVER:         return("WM_MOUSEHOVER");
        case WM_MOUSELEAVE:         return("WM_MOUSELEAVE");
        case WM_CUT:                return("WM_CUT");
        case WM_COPY:               return("WM_COPY");
        case WM_PASTE:              return("WM_PASTE");
        case WM_CLEAR:              return("WM_CLEAR");
        case WM_UNDO:               return("WM_UNDO");
        case WM_RENDERFORMAT:       return("WM_RENDERFORMAT");
        case WM_RENDERALLFORMATS:   return("WM_RENDERALLFORMATS");
        case WM_DESTROYCLIPBOARD:   return("WM_DESTROYCLIPBOARD");
        case WM_DRAWCLIPBOARD:      return("WM_DRAWCLIPBOARD");
        case WM_PAINTCLIPBOARD:     return("WM_PAINTCLIPBOARD");
        case WM_VSCROLLCLIPBOARD:   return("WM_VSCROLLCLIPBOARD");
        case WM_SIZECLIPBOARD:      return("WM_SIZECLIPBOARD");
        case WM_ASKCBFORMATNAME:    return("WM_ASKCBFORMATNAME");
        case WM_CHANGECBCHAIN:      return("WM_CHANGECBCHAIN");
        case WM_HSCROLLCLIPBOARD:   return("WM_HSCROLLCLIPBOARD");
        case WM_QUERYNEWPALETTE:    return("WM_QUERYNEWPALETTE");
        case WM_PALETTEISCHANGING:  return("WM_PALETTEISCHANGING");
        case WM_PALETTECHANGED:     return("WM_PALETTECHANGED");
        case WM_HOTKEY:             return("WM_HOTKEY");
        case WM_PRINT:              return("WM_PRINT");
        case WM_PRINTCLIENT:        return("WM_PRINTCLIENT");
        case WM_USER:               return("WM_USER");
        case WM_USER+1:             return("WM_USER+1");
        case WM_USER+2:             return("WM_USER+2");
        case WM_USER+3:             return("WM_USER+3");
        case WM_USER+4:             return("WM_USER+4");
    }

    return("");
}

void
CPadDoc::RunOneMessage(MSG *pmsg)
{
    PADTHREADSTATE * pts = GetThreadState();
    CPadDoc *pDoc;
    char ach[64];

    for (pDoc = pts->pDocFirst; pDoc != NULL; pDoc = pDoc->_pDocNext)
    {
        if (pmsg->hwnd == pDoc->_hwnd || IsChild(pDoc->_hwnd, pmsg->hwnd))
            break;
    }

    ach[0] = 0;

    if (pmsg->hwnd)
    {
        GetClassNameA(pmsg->hwnd, ach, sizeof(ach));
    }

    PerfLog4(tagPerfWatchPad, pDoc, "+CPadDoc::RunOneMessage msg=%04X (%s) hwnd=%lX (%s)",
        pmsg->message, DecodeMessage(pmsg->message), pmsg->hwnd, ach);

    if (pDoc)
    {
        if (WM_KEYFIRST <= pmsg->message &&
                pmsg->message <= WM_KEYLAST &&
                pDoc->OnTranslateAccelerator(pmsg))
        {
            goto leave;
        }

        if (pmsg->message == WM_PAINT && pDoc->_fPaintLocked)
        {
            PAINTSTRUCT ps;
            BeginPaint(pmsg->hwnd, &ps);
            EndPaint(pmsg->hwnd, &ps);
            pDoc->_fPaintOnUnlock = TRUE;
            goto leave;
        }
    }

    TranslateMessage(pmsg);
    DispatchMessage(pmsg);

leave:
    PerfLog(tagPerfWatchPad, pDoc, "-CPadDoc::RunOneMessage");
    return;
}

#define MAX_MESSAGES_TO_RUN 20

int
CPadDoc::Run(BOOL fStopWhenEmpty)
{
    MSG     msg;
    BOOL    fQuit;
    BOOL    fNoSuspend = IsTagEnabled(tagPadNoSuspendForMessage);
    PADTHREADSTATE * pts = GetThreadState();

    msg.wParam = 0;
    if (fStopWhenEmpty)
    {
        // Just grab all the messages that are currently in the queue, then quit.
        // Also quit if the only messages in the queue are timer messages.

        int n = 0;

        for (;;)
        {
            if (!fNoSuspend)
            {
                ::SuspendCAP();
            }

            Assert(!InSendMessage());

            fQuit = !PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

            if (!fNoSuspend)
            {
                ::ResumeCAP();
            }

            if (fQuit || msg.message == WM_QUIT)
                break;

            RunOneMessage(&msg);

            if (!fNoSuspend)
            {
                ::SuspendCAP();
            }

            if (++n > MAX_MESSAGES_TO_RUN)
            {
                fQuit = GetQueueStatus(QS_ALLEVENTS & ~QS_TIMER) == 0;
            }

            if (!fNoSuspend)
            {
                ::ResumeCAP();
            }

            if (fQuit)
                break;
        }
    }
    else
    {
        // Loop until we get a quit message.

        while (!pts->fEndEvents)
        {
            if (!fNoSuspend)
            {
                ::SuspendCAP();
            }

            //
            // When debug dialog brought up we hit this.  The
            // dialog thread was sending itself a WM_INITDIALOG
            // message and filling in the Listbox.  Why should
            // this assert be valid in this case?  davidd
            //
            // IEBUGBUG             Assert(!InSendMessage());

            fQuit = !GetMessage(&msg, NULL, 0, 0);

            if (!fNoSuspend)
            {
                ::ResumeCAP();
            }

            if (fQuit)
                break;

            RunOneMessage(&msg);
        }

    }

    return fStopWhenEmpty ? 0 : msg.wParam;
}

HRESULT
CPadDoc::GetBrowser()
{
    HRESULT hr = S_OK;

    if (!_pBrowser)
    {
        hr = THR(Open(CLSID_WebBrowser, NULL));
        if (hr)
        {
            goto Cleanup;
        }

        Assert(_pObject);
        hr = THR(_pObject->QueryInterface(IID_IWebBrowser, (void **)&_pBrowser));
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CPadDoc::SaveHistory(IStream * pStream)
{
    HRESULT                 hr = S_OK;
    IPersistHistory *       pPH = NULL;

    if (!_pObject)
        goto Cleanup;

    hr = THR(_pObject->QueryInterface(IID_IPersistHistory, (void **)&pPH));
    if (hr)
        goto Cleanup;

    hr = THR(pPH->SaveHistory(pStream));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pPH);
    RRETURN(hr);
}


HRESULT
CPadDoc::OpenHistory(IStream * pStream)
{
    HRESULT                 hr = S_OK;
    IUnknown *              pUnk = NULL;
    IOleObject *            pObject = NULL;
    IPersistHistory *       pPH = NULL;
    CLSID                   clsid;
    DWORD dwFlags;

    Assert(pStream);

    // Shutdown the previous object.
    hr = THR(Deactivate());
    if (hr)
        goto Cleanup;

    if (!_fDisablePadEvents)
    {
        if (_pEvent)
        {
            _pEvent->Event(_T("Open"), TRUE);
        }

        FireEvent(DISPID_PadEvents_Status, _T("Open"));
    }

    hr = THR(RegisterLocalCLSIDs());
    if (hr)
        goto Cleanup;

    hr = THR(CoCreateInstance(
            CLSID_HTMLDocument,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IUnknown,
            (void **)&pUnk));
    if (hr)
        goto Cleanup;

    hr = pUnk->QueryInterface(IID_IOleObject, (void **)&pObject);
    if (hr)
        goto Cleanup;

    hr = THR(pObject->GetMiscStatus(DVASPECT_CONTENT, &dwFlags));
    if (hr)
        goto Cleanup;

    if (dwFlags & OLEMISC_SETCLIENTSITEFIRST)
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;
            
        if (_ulDownloadNotifyMask)
        {
            hr = THR(AttachDownloadNotify(pObject));
            if (hr)
                goto Cleanup;
        }
    }

    SendAmbientPropertyChange(DISPID_AMBIENT_USERMODE);

    hr = THR(pObject->QueryInterface(IID_IPersistHistory, (void **)&pPH));
    if (hr)
        goto Cleanup;

    Assert(pPH);

    hr = THR(pPH->GetClassID(&clsid));
    if (hr)
        goto Cleanup;

    Assert(clsid == CLSID_HTMLDocument);

    hr = THR(pPH->LoadHistory(pStream, 0));
    if (hr)
        goto Cleanup;

    if (!(dwFlags & OLEMISC_SETCLIENTSITEFIRST))
    {
        hr = THR(pObject->SetClientSite(&_Site));
        if (hr)
            goto Cleanup;
    }

    hr = THR(Activate(pObject));

Cleanup:

    ReleaseInterface(pObject);
    ReleaseInterface(pPH);
    ReleaseInterface(pUnk);
    RRETURN(hr);
}


HRESULT
CPadDoc::DoReloadHistory()
{
    IStream *pStream = NULL;
    HGLOBAL hg = NULL;
    HRESULT hr;
    static LARGE_INTEGER i64Zero = {0, 0};

    hg = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (!hg)
        return E_OUTOFMEMORY;

    hr = THR(CreateStreamOnHGlobal(hg, TRUE, &pStream));
    if (hr)
        goto Cleanup;

    hr = THR(SaveHistory(pStream));
    if (hr)
        goto Cleanup;

    hr = THR(pStream->Seek(i64Zero, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(OpenHistory(pStream));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pStream);

    RRETURN(hr);
}

void
CPadDoc::DirtyColors()
{
    if (_palState == palChanged)
        return;
    _palState = palChanged;
    PostMessage(_hwnd, WM_QUERYNEWPALETTE, 0, 0L);
}

LRESULT
CPadDoc::OnPaletteChanged(HWND hwnd)
{
    HWND hwndDoc = 0;
    if (_pInPlaceObject && SUCCEEDED(_pInPlaceObject->GetWindow(&hwndDoc)) && hwndDoc)
        return SendMessage(hwndDoc, WM_PALETTECHANGED, (WPARAM)hwnd, 0L);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::SetupComposeFont
//
//  Synopsis:   Set the default composition font for MSHTML when its in edit mode.
//
//----------------------------------------------------------------------------
void
CPadDoc::SetupComposeFont()
{
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT hr;
    CVariant varIn;
    
    Assert(_pInPlaceObject);

    if (_fUserMode)
        goto Cleanup;
    
    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    V_VT(&varIn) = VT_BOOL;
    V_BOOL(&varIn) = TRUE;

    pCommandTarget->Exec(
                         (GUID *)&CGID_MSHTML,
                         IDM_HTMLEDITMODE,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );

    V_VT(&varIn) = VT_BSTR;

#if DBG==1
    if (!IsTagEnabled(tagSpanTag))
#endif // DBG==1
        THR(FormsAllocString(_T(",,,,,,Times New Roman"), &V_BSTR(&varIn)));
#if DBG==1
    else if (!IsTagEnabled(tagSpanTag2))
        THR(FormsAllocString(_T("1,,,2,0.128.0,,Times New Roman,MsHtmlPad"), &V_BSTR(&varIn)));
    else
        THR(FormsAllocString(_T("1,,,2,0.128.0,,Times New Roman,MsHtmlPad,1"), &V_BSTR(&varIn)));
#endif // DBG==1
    
    pCommandTarget->Exec(
                         (GUID *)&CGID_MSHTML,
                         IDM_COMPOSESETTINGS,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );
Cleanup:
    ReleaseInterface(pCommandTarget);
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::SetupDefaultBlock
//
//  Synopsis:   Set the default block tag for editting.
//
//----------------------------------------------------------------------------
void
CPadDoc::SetupDefaultBlock()
{
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT hr;
    CVariant varIn;
    
    Assert(_pInPlaceObject);

    if (_fUserMode)
        goto Cleanup;
    
    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    V_VT(&varIn) = VT_BOOL;
    V_BOOL(&varIn) = TRUE;

    pCommandTarget->Exec(
                         (GUID *)&CGID_MSHTML,
                         IDM_HTMLEDITMODE,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );

    V_VT(&varIn) = VT_BSTR;
    if (IsTagEnabled(tagDefaultDIV))
        THR(FormsAllocString(_T("DIV"), &V_BSTR(&varIn)));
    else
        THR(FormsAllocString(_T("P"), &V_BSTR(&varIn)));
    
    pCommandTarget->Exec(
                         (GUID *)&CGID_MSHTML,
                         IDM_DEFAULTBLOCK,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );
Cleanup:
    ReleaseInterface(pCommandTarget);
    return;
}

#if 0
LRESULT
CPadDoc::OnQueryNewPalette()
{
    HWND hwndDoc = 0;
    if (_pInPlaceObject && SUCCEEDED(_pInPlaceObject->GetWindow(&hwndDoc) && _hwndDoc)
        return SendMessage(hwndDoc, WM_QUERYNEWPALETTE, hwnd, 0L);
    return FALSE;
}
#else
LRESULT
CPadDoc::OnQueryNewPalette()
{
    if (!_pObject || !s_fPaletteDevice)
        return FALSE;

    switch (_palState)
    {
    case palChanged:
    case palUnknown:
    {
        HPALETTE hpal = _hpal;

        IViewObject *pVO = NULL;
        LOGPALETTE *plp = NULL;

        //
        // Try to get a color set
        //
        if (OK(_pObject->QueryInterface(IID_IViewObject, (void **)&pVO)))
        {
            pVO->GetColorSet(DVASPECT_CONTENT, -1, NULL, NULL, NULL, &plp);
            pVO->Release();
        }

        if (plp)
        {
            PALETTEENTRY ape[256];

            if ( _hpal ) 
            {
                unsigned cpe = GetPaletteEntries(_hpal, 0, 0, NULL);

                if (cpe > 256 ||
                    cpe != plp->palNumEntries ||
                    (GetPaletteEntries(_hpal, 0, cpe, ape) != cpe) ||
                    memcmp(plp->palPalEntry, ape, cpe * sizeof(PALETTEENTRY)))
                {
                    DeleteObject(_hpal);
                    _hpal = CreatePalette(plp);
                }
            } 
            else 
            {
                _hpal = CreatePalette(plp);
            }

            CoTaskMemFree(plp);
        }

        if (hpal != _hpal)
            SendAmbientPropertyChange(DISPID_AMBIENT_PALETTE);

        _palState = palNormal;
    }
    // Fall through
    case palNormal:
        HDC hdc = GetDC(_hwnd);
        SelectPalette(hdc, _hpal ? _hpal : (HPALETTE)GetStockObject(DEFAULT_PALETTE), FALSE);
        RealizePalette(hdc);
        ReleaseDC(_hwnd, hdc);
    }
    return TRUE;
}
#endif


class CDebugDownloadNotify : public IDownloadNotify
{
public:
    static HRESULT Create(IDownloadNotify **ppdn, ULONG ulMask)
    {
        CDebugDownloadNotify *pddn;
        HRESULT hr = E_OUTOFMEMORY;

        pddn = new CDebugDownloadNotify(ulMask);
        if (!pddn)
            goto Cleanup;

        hr = THR(pddn->QueryInterface(IID_IDownloadNotify, (void**)ppdn));
        if (hr)
            ClearInterface(ppdn);

        pddn->Release();
    Cleanup:
        RRETURN(hr);
    }
    
private:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDebugDownloadNotify))
    CDebugDownloadNotify(ULONG ulMask) { _ulMask = ulMask; _ulRefs = 1; }

    // IUnknown methods
    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj)
    {
        if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDownloadNotify))
        {
            *ppvObj = (IDownloadNotify *)this;
        }
        else
        {
            *ppvObj = NULL;
            return E_NOINTERFACE;
        }
        
        AddRef();
        return S_OK;
    }
    
    STDMETHOD_(ULONG,AddRef)()
    {
        return((ULONG)InterlockedIncrement((LONG *)&_ulRefs));
    }
    
    STDMETHOD_(ULONG,Release)()
    {
        ULONG ulRefs = (ULONG)InterlockedDecrement((LONG *)&_ulRefs);

        if (ulRefs == 0)
        {
            delete this;
        }

        return(ulRefs);
    }

    // IDownloadNotify methods
    virtual HRESULT STDMETHODCALLTYPE DownloadStart( 
        /* [in] */ LPCWSTR pchUrl,
        /* [in] */ DWORD dwDownloadId,
        /* [in] */ DWORD dwType,
        /* [in] */ DWORD dwReserved)
    {
        TraceTag((0, "DownloadStart    %lx (tid%3x): id#%d type %d %ls", this, GetCurrentThreadId(), dwDownloadId, dwType, pchUrl));
        if (!((1 << dwType) & _ulMask))
            return E_ABORT;

        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE DownloadComplete( 
        /* [in] */ DWORD dwDownloadId,
        /* [in] */ HRESULT hrNotify,
        /* [in] */ DWORD dwReserved)
    {
        TraceTag((0, "DownloadComplete %lx (tid%3x): id#%d hr %8lx", this, GetCurrentThreadId(), dwDownloadId, hrNotify));
        return S_OK;
    }

    ULONG _ulRefs;
    ULONG _ulMask;
};


//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::AttachDownloadNotify
//
//  Synopsis:   Attach a new debug download notify object to the doc
//
//----------------------------------------------------------------------------
HRESULT
CPadDoc::AttachDownloadNotify(IOleObject *pObject)
{
    IDownloadNotify *pdn = NULL;
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT hr;
    VARIANT varIn;

    hr = THR_NOTRACE(pObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    hr = CDebugDownloadNotify::Create(&pdn, _ulDownloadNotifyMask);
    if (hr)
        goto Cleanup;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = (IUnknown*)pdn;

    pCommandTarget->Exec(
                         (GUID *)&CGID_DownloadHost,
                         DWNHCMDID_SETDOWNLOADNOTIFY,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );

Cleanup:
    ReleaseInterface(pCommandTarget);
    ReleaseInterface(pdn);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPadDoc::DetachDownloadNotify
//
//  Synopsis:   Detach the debug download notify object from the doc
//
//----------------------------------------------------------------------------
HRESULT
CPadDoc::DetachDownloadNotify(IOleObject *pObject)
{
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT hr;
    VARIANT varIn;

    hr = THR_NOTRACE(pObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = (IUnknown*)NULL;

    pCommandTarget->Exec(
                         (GUID *)&CGID_DownloadHost,
                         DWNHCMDID_SETDOWNLOADNOTIFY,
                         MSOCMDEXECOPT_DONTPROMPTUSER,
                         &varIn,
                         NULL
                        );

Cleanup:
    ReleaseInterface(pCommandTarget);

    RRETURN(hr);
}

//----------------------------------------------------------------
//
//  Zoom Combo UI Stuff
//
//
//
//----------------------------------------------------------------

//+-------------------------------------------------------------------
//
//  Window Procedure: EditZoomWndProc
//
//  Synopsis:
//
//--------------------------------------------------------------------
LRESULT CALLBACK
EditZoomWndProc(HWND hWnd,
             UINT uMessage,
             WPARAM wParam,
             LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_KEYDOWN:
        {
            switch ((int)wParam)
            {
            case VK_RETURN:
                ::SendMessage(GetParent(hWnd), WM_ZOOMRETURN, 0, 0);
                return 0;
            case VK_ESCAPE:
                ::SendMessage(GetParent(hWnd), WM_ZOOMESCAPE, 0, 0);
                return 0;
            case VK_UP:
                ::SendMessage(GetParent(hWnd), WM_ZOOMUP, 0, 0);
                return 0;
            case VK_DOWN:
                ::SendMessage(GetParent(hWnd), WM_ZOOMDOWN, 0, 0);
                return 0;
            default:
                break;
            }
        }
        break;

    case WM_KEYUP:
        {
            switch ((int)wParam)
            {
            case VK_RETURN:
            case VK_ESCAPE:
            case VK_UP:
            case VK_DOWN:
                return 0;
            default:
                break;
            }
        }
    default:
        break;
    }
    //  Edit has address of default wnd proc in its GWLP_USERDATA
    return CallWindowProc((WNDPROC)GetWindowLongPtr(hWnd, GWLP_USERDATA), 
        hWnd, uMessage, wParam, lParam);
}

//+-------------------------------------------------------------------
//
//  Window Procedure: ZoomComboWndProc
//
//  Synopsis:
//
//--------------------------------------------------------------------
LRESULT CALLBACK
ZoomComboWndProc(HWND hWnd,
             UINT uMessage,
             WPARAM wParam,
             LPARAM lParam)
{
    MSG        msg;
    HWND       hwndTooltip;

    switch (uMessage)
    {
    case WM_MOUSEMOVE :
    case WM_LBUTTONDOWN :
    case WM_LBUTTONUP :
        msg.lParam  = lParam;
        msg.wParam  = wParam;
        msg.message = uMessage;
        msg.hwnd    = hWnd;
        hwndTooltip = (HWND) ::SendMessage(GetParent(hWnd), TB_GETTOOLTIPS, 0, 0);
        ::SendMessage(hwndTooltip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
        break;

    case WM_ZOOMRETURN:
        ::SendMessage(::GetParent(hWnd), 
            WM_COMMAND, 
            (WPARAM)MAKELONG(IDM_ZOOMPERCENT, CBN_SELENDOK), 
            (LPARAM)hWnd);
        return 0;

    case WM_ZOOMESCAPE:
        if ((WPARAM)::SendMessage(hWnd, CB_GETCURSEL, 0, 0) == CB_ERR)
        {
            ::SendMessage(hWnd, CB_SETCURSEL, (WPARAM)-1, 0);
        }
        else
        {
            ::SendMessage(::GetParent(hWnd), 
                WM_COMMAND, 
                (WPARAM)MAKELONG(IDM_ZOOMPERCENT, CBN_SELENDOK), 
                (LPARAM)hWnd);
        }
        return 0;

    case WM_ZOOMUP:
    case WM_ZOOMDOWN:
        {
            WPARAM wCurSel;

            wCurSel = (WPARAM)::SendMessage(hWnd, CB_GETCURSEL, 0, 0);
            if (wCurSel == CB_ERR)
            {
                wCurSel = 0;
            }

            if (uMessage == WM_ZOOMUP)
            {
                if (wCurSel > 0)
                {
                    wCurSel--;
                }
            }
            else
            {
                if ((wCurSel + 1) < (WPARAM)::SendMessage(hWnd, CB_GETCOUNT, 0, 0))
                {
                    wCurSel++;
                }
            }

            ::SendMessage(hWnd, CB_SETCURSEL, wCurSel, 0);
        }
        return 0;

    default:
        break;
    }
    return CallWindowProc(lpfnDefCombo, hWnd, uMessage, wParam, lParam);
}

//----------------------------------------------------------------
//
//  Editing Toolbar UI Stuff
//
//
//
//----------------------------------------------------------------

//+-------------------------------------------------------------------
//
//  Window Procedure: ComboWndProc
//
//  Synopsis:
//
//--------------------------------------------------------------------
#if defined(DBG_TOOLTIPS)
LRESULT CALLBACK
ComboWndProc(HWND hWnd,
             UINT uMessage,
             WPARAM wParam,
             LPARAM lParam)
{
    MSG        msg;
    HWND       hwndTooltip;

    switch (uMessage)
    {
    case WM_MOUSEMOVE :
    case WM_LBUTTONDOWN :
    case WM_LBUTTONUP :
        msg.lParam  = lParam;
        msg.wParam  = wParam;
        msg.message = uMessage;
        msg.hwnd    = hWnd;
        hwndTooltip = (HWND) SendMessage(GetParent(hWnd), TB_GETTOOLTIPS, 0, 0);
        SendMessage(hwndTooltip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
        break;

    case WM_CHAR:
        // Don't allow the user to type anything into the combo
        return 0;
    }
    return CallWindowProc(lpfnDefCombo, hWnd, uMessage, wParam, lParam);
}
#endif

//+-------------------------------------------------------------------
//
// Local Helper Function: InstallComboboxTooltip
//
//--------------------------------------------------------------------

void 
CPadDoc::InstallComboboxTooltip(HWND hwndCombo, UINT IDMmessage)
{
#if defined(DBG_TOOLTIPS)
    HWND     hwndTooltip;
    TOOLINFO tbToolInfo;

    Assert(hwndCombo);
    Assert(GetParent(hwndCombo));

    hwndTooltip = (HWND) SendMessage(
            GetParent(hwndCombo),
            TB_GETTOOLTIPS,
            0,
            0);

    SetWindowLongPtr(hwndCombo, GWLP_WNDPROC, (LONG)ComboWndProc);

    tbToolInfo.cbSize   = sizeof(TOOLINFO);
    tbToolInfo.uFlags   = TTF_IDISHWND;
    tbToolInfo.hwnd     = GetParent(hwndCombo);
    tbToolInfo.uId      = (UINT) hwndCombo;
    tbToolInfo.hinst    = 0;

#ifndef WINCE
    DWORD dwVersion = GetVersion();
#else
    DWORD dwVersion = 0;
#endif

    tbToolInfo.lpszText = (dwVersion < 0x80000000) ?
            ((LPTSTR) LPSTR_TEXTCALLBACKW) : ((LPTSTR) LPSTR_TEXTCALLBACKA);

    Assert(hwndTooltip);
    SendMessage(
            hwndTooltip,
            (dwVersion < 0x80000000) ? (TTM_ADDTOOLW) : (TTM_ADDTOOLA),
            0,
            (LPARAM)(LPTOOLINFO)&tbToolInfo);
#endif
}

//+-------------------------------------------------------------------
//
//  Callback:   FillFontProc
//
//  This procedure is called by the EnumFontFamilies call.
//  It fills the combobox with the font facename.
//
//--------------------------------------------------------------------

int CALLBACK
FillFontProc(const LOGFONT *    lplf,
             const TEXTMETRIC * lptm,
             DWORD            iFontType,
             LPARAM           lParam)
{
    int  fontStyle[3];
    char szFontName[128];

    // We don't want to list the vertical fonts.
    // These by convention start with an @ symbol.
    if (lplf->lfFaceName[0] == L'@')
        return TRUE;

    fontStyle[0] = (lplf->lfWeight == FW_BOLD) ? (1) : (0);
    fontStyle[1] = (lplf->lfItalic == TRUE)    ? (1) : (0);
    fontStyle[2] = (lplf->lfUnderline == TRUE) ? (1) : (0);
    WideCharToMultiByte(
            CP_ACP,
            0,
            (const WCHAR *) lplf->lfFaceName,
            -1,
            szFontName,
            ARRAY_SIZE(szFontName),
            NULL,
            NULL);
    if (CB_ERR == (WPARAM) SendMessage((HWND) lParam,CB_FINDSTRING,
                  (WPARAM) -1,(LPARAM)(lplf->lfFaceName)))
    {
        SendMessage((HWND)lParam,CB_ADDSTRING,
                    (WPARAM) 0,(LPARAM)(lplf->lfFaceName));
    }
    return TRUE;
}

//+-------------------------------------------------------------------
//
// Local Helper Functions: AddComboboxItems, ConvColorrefToString
//
//--------------------------------------------------------------------




void 
CPadDoc::ConvColorrefToString(COLORREF crColor, LPTSTR szName, int cchName )
{
    int     i;
    BOOL fFound = FALSE;

    if(crColor == (COLORREF)-1)
    {
        szName[0] = 0;
        return;
    }

    // Reset the upper 8 bits because palettergb type color values have them
    // set to 0x20 and the compare will fail
    crColor &= 0xFFFFFF;

    for(i = 0; ComboColorItems[i].iIdm != 0; i++)
    {
        if(ComboColorItems[i].lData == (LONG)crColor)  {
            fFound = TRUE;
            break;
        }
    }

    if(fFound)
        Format(0, szName, cchName, MAKEINTRESOURCE(ComboColorItems[i].iIdm));
    else
        szName[0] = 0;
}


DWORD 
CPadDoc::AddComboboxItems(HWND hwndCombo,
                       BOOL fItemData,
                       const ComboItem * pComboItems)
{
    DWORD   dwIndex = 0;
    TCHAR   achColor[128];

    while(pComboItems->iIdm)
    {
        Format(0, achColor, ARRAY_SIZE(achColor),
                  MAKEINTRESOURCE(pComboItems->iIdm));

        dwIndex = SendMessage(
                hwndCombo,
                CB_ADDSTRING,
                0,
                (LPARAM) achColor);
        if (fItemData)
        {
            SendMessage(
                    hwndCombo,
                    CB_SETITEMDATA,
                    dwIndex,
                    (LPARAM) pComboItems->lData);
        }
        pComboItems ++;
    }
    return dwIndex;
}
///--------------------------------------------------------------
//
// Member:    CPadDoc::IsEdit
//
// Synopsis: Return TRUE if we are in Edit Mode
//           False if in BROWSE Mode
//
//---------------------------------------------------------------

BOOL
CPadDoc::IsEdit()
{
    return ! _fUserMode; 
}

//+=========================================================
//
// Update the Standard Toolbar UI.
//
//
//
//+=========================================================

LRESULT
CPadDoc::UpdateStandardToolbar()
{
   static const MsoCmdInfo TBBtnInfo[] = {
            { IDM_PAD_BACK,    IDM_UNKNOWN,    NULL },
            { IDM_PAD_FORWARD, IDM_UNKNOWN,    NULL },
            { IDM_PAD_HOME,    IDM_UNKNOWN,    NULL },
            { IDM_PAD_FIND,    IDM_UNKNOWN,    NULL },
            { IDM_PAD_STOP,    OLECMDID_STOP,  NULL },
            { IDM_PAD_REFRESH, OLECMDID_REFRESH,NULL },
            { IDM_PAD_PRINT,   OLECMDID_PRINT, NULL },
            { IDM_PAD_CUT,     OLECMDID_CUT,   NULL },
            { IDM_PAD_COPY,    OLECMDID_COPY,  NULL },
            { IDM_PAD_PASTE,   OLECMDID_PASTE, NULL },
            { 0, 0, NULL }
    };
    int cButton;

    HRESULT hr = S_OK;
    MSOCMD msocmd;
    
    IOleCommandTarget* pCommandTarget = NULL;
    
    if (!_fStandardInit)
    {
        InitStandardToolbar();
        _fStandardInit = TRUE;
    }
#ifdef IE5_ZOOM
    EnableWindow(_hwndComboZoom, !!_pInPlaceObject);
#endif
    
    if (hr)
        goto Cleanup;
        
    if (!_hwndToolbar)
        goto Cleanup;

    if ( !_pInPlaceObject )
        goto Cleanup;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if (hr)
        goto Cleanup;
    //
    // Make sure we are in sync with the doc user mode.
    //
    msocmd.cmdID = IDM_EDITMODE;
    msocmd.cmdf  = 0;
    if (OK(THR(pCommandTarget->QueryStatus(&CGID_MSHTML, 1, &msocmd, NULL))))
        _fUserMode = msocmd.cmdf != MSOCMDSTATE_DOWN;

    UpdateFontSizeBtns(pCommandTarget);

    for (cButton = 0; TBBtnInfo[cButton].localIDM; cButton ++)
    {
        msocmd.cmdID = TBBtnInfo[cButton].MsoCmdIDM;
        msocmd.cmdf  = 0;

        hr = pCommandTarget->QueryStatus(
            TBBtnInfo[cButton].MsoCmdGUID,
            1,
            &msocmd,
            NULL);

        if (_pBrowser && (IDM_UNKNOWN == TBBtnInfo[cButton].MsoCmdIDM))
        {
            SendMessage(
                    _hwndToolbar,
                    TB_ENABLEBUTTON,
                    (WPARAM) TBBtnInfo[cButton].localIDM,
                    (LPARAM) MAKELONG(TRUE, 0));
        }
        else
        {
            SendMessage(
                    _hwndToolbar,
                    TB_ENABLEBUTTON,
                    (WPARAM) TBBtnInfo[cButton].localIDM,
                    (LPARAM) MAKELONG(msocmd.cmdf & MSOCMDF_ENABLED ? TRUE : FALSE, 0));
        }

        switch (msocmd.cmdf)
        {
        case MSOCMDSTATE_UP:
        case MSOCMDSTATE_DOWN:
        case MSOCMDSTATE_NINCHED:
            SendMessage(
                    _hwndToolbar,
                    TB_CHECKBUTTON,
                    (WPARAM) TBBtnInfo[cButton].localIDM,
                    (LPARAM) MAKELONG(
                            (msocmd.cmdf == MSOCMDSTATE_DOWN) ?
                                    (TRUE) : (FALSE),
                            0));
            break;
        }
        SendMessage(
                _hwndToolbar,
                TB_PRESSBUTTON,
                TBBtnInfo[cButton].localIDM,
                (LPARAM) MAKELONG(FALSE, 0));
    }



    SendMessage(
            _hwndToolbar,
            TB_CHANGEBITMAP,
            IDM_PAD_EDITBROWSE,
            MAKELPARAM(_fUserMode ? 13 : 12, 0));

    SendMessage(_hwndToolbar, TB_ENABLEBUTTON, (WPARAM)IDM_PAD_EDITBROWSE, !_pBrowser);

    pCommandTarget->Release();

Cleanup:
    return 0;
}

void
CPadDoc::UpdateFormatButtonStatus()
{
    UINT cButtons;
    CVariant var;
    int j, iIndex, iCurrentIndex;
    TCHAR szBuf[128];
    HWND  hwndCombobox = NULL;  
    MSOCMD  msocmd;
 
        // update zoom combobox status
    static const UINT ComboSet[] = {
            IDM_FONTNAME,
            IDM_FONTSIZE,
            IDM_BLOCKFMT,
            IDM_FORECOLOR,
            0 };
            
            
    struct ButtonInfo {
        UINT ButtonIDM;
        UINT ToolbarIDR;
    };


    static const ButtonInfo tbButtons[] =
    {
        { IDM_BOLD,          IDR_HTMLFORM_TBFORMAT },
        { IDM_ITALIC,        IDR_HTMLFORM_TBFORMAT },
        { IDM_UNDERLINE,     IDR_HTMLFORM_TBFORMAT },
        { IDM_ORDERLIST,     IDR_HTMLFORM_TBFORMAT },
        { IDM_UNORDERLIST,   IDR_HTMLFORM_TBFORMAT },
        { IDM_INDENT,        IDR_HTMLFORM_TBFORMAT },
        { IDM_OUTDENT,       IDR_HTMLFORM_TBFORMAT },
        { IDM_JUSTIFYLEFT,   IDR_HTMLFORM_TBFORMAT },
        { IDM_JUSTIFYCENTER, IDR_HTMLFORM_TBFORMAT },
        { IDM_JUSTIFYRIGHT,  IDR_HTMLFORM_TBFORMAT },
        { IDM_BLOCKDIRLTR,   IDR_HTMLFORM_TBFORMAT },
        { IDM_BLOCKDIRRTL,   IDR_HTMLFORM_TBFORMAT },
        { 0, 0}

    };

    HRESULT hr;

    
    IOleCommandTarget * pCommandTarget = NULL;
            

    
    if ( !_pInPlaceObject )
        goto Cleanup;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
            IID_IOleCommandTarget,
            (void **)&pCommandTarget));
    if ( hr )
        goto Cleanup;
       




    
    for (j = 0; ComboSet[j]; j ++)
    {
        switch (ComboSet[j])
        {
        case IDM_FONTSIZE:
            hwndCombobox = _hwndComboSize;
            var.vt   = VT_I4;
            var.lVal = 0;
            break;
        case IDM_BLOCKFMT:
            hwndCombobox = _hwndComboTag;
            break;
        case IDM_FONTNAME:
            hwndCombobox = _hwndComboFont;
            break;
        case IDM_FORECOLOR:
            hwndCombobox = _hwndComboColor;
            break;
        }

        msocmd.cmdID = ComboSet[j];
        msocmd.cmdf  = 0;
        hr = THR( pCommandTarget->QueryStatus((GUID *)&CGID_MSHTML, 1, &msocmd, NULL));
        switch (msocmd.cmdf)
        {
        case MSOCMDSTATE_UP:
        case MSOCMDSTATE_DOWN:
        case MSOCMDSTATE_NINCHED:
            EnableWindow(hwndCombobox, TRUE);
            break;

        case MSOCMDSTATE_DISABLED:
        default:
            EnableWindow(hwndCombobox, FALSE);
            break;
        }

        hr = THR_NOTRACE( pCommandTarget->Exec((GUID *)&CGID_MSHTML, ComboSet[j],
                MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &var));
        if (FAILED(hr))
            continue;

        switch (ComboSet[j])
        {
        case IDM_BLOCKFMT:
        case IDM_FONTNAME:
            // It is legal for the returned bstr to be NULL.
            wcscpy(szBuf, (var.vt == VT_BSTR && var.bstrVal) ? var.bstrVal : TEXT(""));
            break;

        case IDM_FORECOLOR:
            if(V_VT(&var) == VT_NULL)
                szBuf[0] = 0;
            else
                ConvColorrefToString(V_I4(&var), szBuf, ARRAY_SIZE(szBuf));
            break;

        case IDM_FONTSIZE:
            // If the font size is changing in the selection VT_NULL is returned
            if(V_VT(&var) == VT_NULL)
            {
                szBuf[0] = 0;
            }
            else
            {
                Format(0, szBuf, ARRAY_SIZE(szBuf), TEXT("<0d>"), var.lVal);
            }
            break;
        }

        iIndex = SendMessage(
                hwndCombobox,
                CB_FINDSTRINGEXACT,
                (WPARAM) -1,
                (LPARAM)(LPTSTR) szBuf);

        if (iIndex == CB_ERR)
        {
            // CB_FINDSTRINGEXACT cannot find the string in the combobox.
            //
            switch (ComboSet[j])
            {
            case IDM_BLOCKFMT:
                // GetBlockFormat returns something not in the BlockFormat
                // combobox, display empty string.
                //
                iIndex = -1;
                break;

            case IDM_FONTSIZE:
            case IDM_FONTNAME:
            case IDM_FORECOLOR:
                // Nothing is selected
                iIndex = -1;
                break;
            }
        }

        iCurrentIndex = SendMessage(hwndCombobox, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
        if ( iCurrentIndex != iIndex )
            SendMessage(hwndCombobox, CB_SETCURSEL, (WPARAM) iIndex, (LPARAM) 0);

        VariantClear(&var);
    }

    // update buttons status
    BOOL fEnabled;
    BOOL fChecked;
    for (cButtons = 0; tbButtons[cButtons].ButtonIDM != 0; cButtons ++)
    {
        if (_hwndFormatToolbar)
        {
            msocmd.cmdID = tbButtons[cButtons].ButtonIDM;
            msocmd.cmdf  = 0;
            hr = pCommandTarget->QueryStatus((GUID *)&CGID_MSHTML, 1, &msocmd, NULL);

            switch (msocmd.cmdf)
            {
            case MSOCMDSTATE_UP:
            case MSOCMDSTATE_DOWN:
            case MSOCMDSTATE_NINCHED:
                fEnabled = TRUE;
                fChecked = (msocmd.cmdf == MSOCMDSTATE_DOWN) ? TRUE : FALSE;
                break;

            case MSOCMDSTATE_DISABLED:
            default:
                fEnabled = FALSE;
                fChecked = FALSE;
                break;
            }
            SendMessage(
                    _hwndFormatToolbar,
                    TB_ENABLEBUTTON,
                    (WPARAM) tbButtons[cButtons].ButtonIDM,
                    (LPARAM) MAKELONG(fEnabled, 0));
            SendMessage(
                    _hwndFormatToolbar,
                    TB_CHECKBUTTON,
                    (WPARAM) tbButtons[cButtons].ButtonIDM,
                    (LPARAM) MAKELONG(fChecked, 0));
            SendMessage(
                    _hwndFormatToolbar,
                    TB_PRESSBUTTON,
                    (WPARAM) tbButtons[cButtons].ButtonIDM,
                    (LPARAM) MAKELONG(FALSE, 0));
        }

    }
Cleanup:

    if ( pCommandTarget) 
        pCommandTarget->Release();

}

LRESULT
CPadDoc::UpdateFormatToolbar()
{
    if ( !_pInPlaceObject )
        goto Cleanup;


    if ( IsEdit())
    {
        if ( ! _fFormatInit )
        {   
            InitFormatToolbar(); // Load the pull-downs and other fun stuff.
            _fFormatInit = TRUE;
        }
        UpdateFormatButtonStatus();
        if ( _fToolbarhidden )
            ShowFormatToolbar();
    }
    else if ( ! _fToolbarhidden )
    {
        HideFormatToolbar();
    }

Cleanup:
    return 0;
 
}

void
CPadDoc::ShowFormatToolbar()
{
    ::ShowWindow(_hwndFormatToolbar, SW_SHOW);
    _fToolbarhidden = FALSE;
    //
    //Resize
    //
    //
    ::SetMenu( _hwnd, _hmenuEdit );
    Resize();
}

void
CPadDoc::HideFormatToolbar()
{
    ::ShowWindow(_hwndFormatToolbar, SW_HIDE);
    _fToolbarhidden = TRUE;
    //
    // Resize
    //
    ::SetMenu( _hwnd, _hmenuMain );
    Resize();
}

LRESULT
CPadDoc::UpdateToolbarUI()
{
    UpdateStandardToolbar();
    UpdateFormatToolbar();
    UpdateDirtyUI();
    
    return 0;
}

///--------------------------------------------------------------
//
// Member:    CPadDoc::CreateStandardToolbar
//
// Synopsis: Creates the Standard toolbar UI.
//
//
//---------------------------------------------------------------
HRESULT 
CPadDoc::CreateStandardToolbar()
{
    HRESULT hr = S_OK;
    HWND   hwndToolbar = NULL;
    HFONT  hFont;
    DWORD  cTBCombos;
    TEXTMETRIC tm;
    HDC    hdc;
    HWND * pHwndCombo = NULL;

    struct ComboInfo {
        UINT ComboIDM;
        UINT ToolbarIDR;
        LONG cx;
        LONG dx;
        LONG cElements;
        UINT ComboStyle;
    };
    static const ComboInfo tbCombos[] =
    {
        { IDM_ZOOMPERCENT, IDR_HTMLFORM_TBSTANDARD, 402, 60, 12, 
                CBS_DROPDOWN},
        { 0, 0, 0, 0, 0}
    };

   //
    // Standard Toolbar table
    //
    static const TBBUTTON tbButton[] =
    {
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        { 0, IDM_PAD_BACK, 0, TBSTYLE_BUTTON, 0L, 0},
        { 1, IDM_PAD_FORWARD, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        { 2, IDM_PAD_HOME, 0, TBSTYLE_BUTTON, 0L, 0},
        { 3, IDM_PAD_FIND, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        { 4, IDM_PAD_STOP, 0, TBSTYLE_BUTTON, 0L, 0},
        { 5, IDM_PAD_REFRESH, 0, TBSTYLE_BUTTON, 0L, 0},
        { 6, IDM_PAD_PRINT, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        { 7, IDM_PAD_FONTINC, 0, TBSTYLE_BUTTON, 0L, 0},
        { 8, IDM_PAD_FONTDEC, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        { 9, IDM_PAD_CUT, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, IDM_PAD_COPY, 0, TBSTYLE_BUTTON, 0L, 0},
        {11, IDM_PAD_PASTE, 0, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        {13, IDM_PAD_EDITBROWSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {10, 0, 0, TBSTYLE_SEP, 0L, 0},

        {14, IDM_PAD_USESHDOCVW, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},

        // Reserve space for zoom combo.
        {70, 0, 0, TBSTYLE_SEP, 0L, 0}

        
    };

 

    
    //
    // Create the "standard" toolbar
    //
    
    _hwndToolbar = CreateToolbarEx(
            _hwnd,
            WS_CHILD | WS_VISIBLE /* TBSTYLE_TOOLTIPS*/,
            IDR_HTMLPAD_TOOLBAR,
            15,                             // number of bitmaps
            g_hInstResource,
            IDB_HTMLPAD_TOOLBAR,
            (LPCTBBUTTON) &tbButton,
            ARRAY_SIZE(tbButton),
            16,
            16,
            16,
            16,
            sizeof(TBBUTTON));
            
    if (!_hwndToolbar)
    {
        hr = E_FAIL;
    }

    for (cTBCombos = 0; tbCombos[cTBCombos].ComboIDM; cTBCombos ++)
    {
        switch (tbCombos[cTBCombos].ComboIDM)
        {
        case IDM_ZOOMPERCENT:
            pHwndCombo = &(_hwndComboZoom);
            break;
        }
        switch (tbCombos[cTBCombos].ToolbarIDR)
        {
        case IDR_HTMLFORM_TBSTANDARD:
            hwndToolbar = _hwndToolbar;
            break;
        case IDR_HTMLFORM_TBFORMAT:
            hwndToolbar = _hwndFormatToolbar;
            break;
        }

        Assert(hwndToolbar);

        *pHwndCombo = CreateWindow(
                TEXT("COMBOBOX"),
                TEXT(""),
                WS_CHILD | WS_VSCROLL | tbCombos[cTBCombos].ComboStyle,
                0,
                0,
                tbCombos[cTBCombos].dx,
                0,
                hwndToolbar,
                (HMENU)tbCombos[cTBCombos].ComboIDM,
                GetResourceHInst(),
                NULL);
        if (!(*pHwndCombo))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(*pHwndCombo, WM_SETFONT, (WPARAM)hFont, FALSE);
        hdc   = ::GetDC(*pHwndCombo);
        GetTextMetrics(hdc, &tm);
        ::ReleaseDC(*pHwndCombo, hdc);
        DeleteObject(hFont);

        EnableWindow(*pHwndCombo, FALSE);
        ::ShowWindow(*pHwndCombo, SW_SHOW);
        ::MoveWindow(
                *pHwndCombo,
                tbCombos[cTBCombos].cx,
                2,
                tbCombos[cTBCombos].dx,
                tm.tmHeight * min((long) MAX_COMBO_VISUAL_ITEMS,
                        (long) tbCombos[cTBCombos].cElements),
                FALSE);
        lpfnDefCombo = (WNDPROC) GetWindowLongPtr(*pHwndCombo, GWLP_WNDPROC);
        InstallComboboxTooltip(*pHwndCombo, tbCombos[cTBCombos].ComboIDM);

        if (*pHwndCombo == _hwndComboZoom)
        {
            WNDPROC lpfnDefEdit;
            HWND    hWndEdit;
            POINT   pt;

            //  Set window proc for zoom combo
            lpfnDefEdit = (WNDPROC)GetWindowLongPtr(*pHwndCombo, GWLP_WNDPROC);
            SetWindowLongPtr(*pHwndCombo, GWLP_WNDPROC, (LONG_PTR)ZoomComboWndProc);
            SetWindowLongPtr(*pHwndCombo, GWLP_USERDATA, (LONG_PTR)lpfnDefEdit);

            //  Set window proc for zoom combo edit contol
            pt.x = 3; 
            pt.y = 3; 
            hWndEdit = ChildWindowFromPoint(*pHwndCombo, pt);
            lpfnDefEdit = (WNDPROC)GetWindowLongPtr(hWndEdit, GWLP_WNDPROC);
            SetWindowLongPtr(hWndEdit, GWLP_WNDPROC, (LONG_PTR)EditZoomWndProc);
            SetWindowLongPtr(hWndEdit, GWLP_USERDATA, (LONG_PTR)lpfnDefEdit);
        }
    }
Cleanup:
RRETURN ( hr );

}
///--------------------------------------------------------------
//
// Member:    CPadDoc::CreateFormatToolbar
//
// Synopsis: Creates the Formatting Toolbar UI.
//
//
//---------------------------------------------------------------
HRESULT 
CPadDoc::CreateFormatToolbar()
{

    HRESULT hr = S_OK;
    int newTop = 0;
    
    
    HWND   hwndToolbar = NULL;
    HFONT  hFont;
    DWORD  cTBCombos;
    TEXTMETRIC tm;
    HDC    hdc;
    HWND * pHwndCombo = NULL;

    struct ComboInfo {
        UINT ComboIDM;
        UINT ToolbarIDR;
        LONG cx;
        LONG dx;
        LONG cElements;
        UINT ComboStyle;
    };
    static const ComboInfo tbCombos[] =
    {
        { IDM_BLOCKFMT,  IDR_HTMLFORM_TBFORMAT,       5, 100, 16,
                CBS_DROPDOWNLIST},
        { IDM_FONTNAME,    IDR_HTMLFORM_TBFORMAT,   110, 150, 43,
                CBS_SORT | CBS_DROPDOWNLIST},
        { IDM_FONTSIZE,    IDR_HTMLFORM_TBFORMAT,   265,  40,  8,
                CBS_DROPDOWNLIST},
        { IDM_FORECOLOR,   IDR_HTMLFORM_TBFORMAT,   384,  55, 12,
                CBS_DROPDOWNLIST},
        { 0, 0, 0, 0, 0}
    };


    static const TBBUTTON tbFmtButton[] =
    {
        // reserved space for HTML Markup Tag,FontName, and FontSize Comboboxes
        {110, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        {155, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},
        { 45, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {0, IDM_BOLD,      TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},
        {1, IDM_ITALIC,    TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},
        {2, IDM_UNDERLINE, TBSTATE_ENABLED, TBSTYLE_CHECK,  0L, 0},

        // reserved space for BackGroundColor Combobox.
        { 65, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {3, IDM_JUSTIFYLEFT,   TBSTATE_CHECKED, TBSTYLE_CHECK, 0L, 0},
        {4, IDM_JUSTIFYCENTER, TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
        {5, IDM_JUSTIFYRIGHT,  TBSTATE_ENABLED, TBSTYLE_CHECK, 0L, 0},
        {5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {6, IDM_ORDERLIST,  TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0L, 0},
        {7, IDM_UNORDERLIST, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {8, IDM_OUTDENT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {9, IDM_INDENT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {5, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0L, 0},

        {10, IDM_BLOCKDIRLTR, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0},
        {11, IDM_BLOCKDIRRTL, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0L, 0}
    };


    _hwndFormatToolbar = CreateToolbarEx(
            _hwnd,
            WS_CHILD | WS_VISIBLE | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS | CCS_NODIVIDER ,
            IDR_HTMLPAD_TOOLBAR,
            15,                             // number of bitmaps
            g_hInstResource,
            IDB_HTMLPAD_TBFORMAT ,
            (LPCTBBUTTON) &tbFmtButton,
            ARRAY_SIZE(tbFmtButton),
            16,
            16,
            16,
            16,
            sizeof(TBBUTTON));
            
    if (!(_hwndFormatToolbar))
    {
        hr = E_FAIL;
        goto Cleanup;
    }
        RECT rcClient;
        GetClientRect( _hwnd, &rcClient);

        RECT rcHost;
        GetWindowRect( _hwnd, & rcHost);
        newTop = 26 ;
        RECT rcTopToolbar;
        GetWindowRect( _hwndToolbar, &rcTopToolbar );
        newTop = rcTopToolbar.bottom - rcTopToolbar.top;
        RECT rcToolbar;
        GetWindowRect( _hwnd, &rcToolbar);
        
        ::MoveWindow(
                _hwndFormatToolbar,
                0, 
                newTop  ,
                3000 /*rcHost.right - rcHost.left*/ ,
                rcToolbar.bottom - rcToolbar.top ,
                TRUE);

         
    for (cTBCombos = 0; tbCombos[cTBCombos].ComboIDM; cTBCombos ++)
    {
        switch (tbCombos[cTBCombos].ComboIDM)
        {
        case IDM_BLOCKFMT:
            pHwndCombo = &(_hwndComboTag);
            break;
        case IDM_FONTNAME:
            pHwndCombo = &(_hwndComboFont);
            break;
        case IDM_FONTSIZE:
            pHwndCombo = &(_hwndComboSize);
            break;
        case IDM_FORECOLOR:
            pHwndCombo = &(_hwndComboColor);
            break;
        }
        switch (tbCombos[cTBCombos].ToolbarIDR)
        {
        case IDR_HTMLFORM_TBSTANDARD:
            hwndToolbar = _hwndToolbar;
            break;
        case IDR_HTMLFORM_TBFORMAT:
            hwndToolbar = _hwndFormatToolbar;
            break;
        }

        Assert(hwndToolbar);

        *pHwndCombo = CreateWindow(
                TEXT("COMBOBOX"),
                TEXT(""),
                WS_CHILD | WS_VSCROLL | tbCombos[cTBCombos].ComboStyle,
                0,
                0,
                tbCombos[cTBCombos].dx,
                0,
                hwndToolbar,
                (HMENU)tbCombos[cTBCombos].ComboIDM,
                GetResourceHInst(),
                NULL);
        if (!(*pHwndCombo))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(*pHwndCombo, WM_SETFONT, (WPARAM) hFont, FALSE);
        hdc   = ::GetDC(*pHwndCombo);
        GetTextMetrics(hdc, &tm);
        ::ReleaseDC(*pHwndCombo, hdc);
        DeleteObject(hFont);

        EnableWindow(*pHwndCombo, TRUE);
        ::ShowWindow(*pHwndCombo, SW_SHOW);
        ::MoveWindow(
                *pHwndCombo,
                tbCombos[cTBCombos].cx,
                2,
                tbCombos[cTBCombos].dx,
                tm.tmHeight * min((long) MAX_COMBO_VISUAL_ITEMS,
                        (long) tbCombos[cTBCombos].cElements),
                FALSE);
        lpfnDefCombo = (WNDPROC) GetWindowLongPtr(*pHwndCombo, GWLP_WNDPROC);
        InstallComboboxTooltip(*pHwndCombo, tbCombos[cTBCombos].ComboIDM);

        }

Cleanup:
    RRETURN ( hr );

}

//+-------------------------------------------------------------------
//
// Member:  CPadDoc::ResetZoomPercent
//
//--------------------------------------------------------------------
void 
CPadDoc::ResetZoomPercent()
{
    ::SendMessage(_hwndComboZoom, CB_SETCURSEL, INDEX_ZOOM_100, 0);
}

//+-------------------------------------------------------------------
//
// Member:  CPadDoc::ApplyZoomPercent
//
//--------------------------------------------------------------------
void 
CPadDoc::ApplyZoomPercent()
{
    ::SendMessage(
        _hwnd, 
        WM_COMMAND, 
        (WPARAM)MAKELONG(IDM_ZOOMPERCENT, CBN_SELENDOK), 
        (LPARAM)_hwndComboZoom);
}

//+-------------------------------------------------------------------
//
// Member:  CPadDoc::InitStandardToolbar
//
//--------------------------------------------------------------------
void
CPadDoc::InitStandardToolbar()
{
    AddComboboxItems(_hwndComboZoom, TRUE, ComboZoomItems);
    ResetZoomPercent();
}


//+-------------------------------------------------------------------
//
// Member:  CDoc::InitFormatToolbar
//
//--------------------------------------------------------------------

void
CPadDoc::InitFormatToolbar()
{

    HDC      hdc;
    HRESULT  hr = S_OK ;

    struct ComboStruct {
        UINT ComboIDM;
        BOOL fLoaded;
    };

    // load items into ComboTag, ComboZoom and ComboSize comboboxes
    static ComboStruct ComboLoad[] = {
            { IDM_FONTSIZE, FALSE },
            { IDM_GETBLOCKFMTS, FALSE },
            { 0, FALSE },
    };
    int i;

    if ( ! _fComboLoaded)
    {
        //
        // Load Font Names
        //
        hdc = ::GetDC(_hwnd);
        EnumFontFamilies(
                hdc,
                NULL,
                FillFontProc,
                (LPARAM)_hwndComboFont);
        ::ReleaseDC(_hwnd, hdc);


        //
        // Load Colors
        //
        AddComboboxItems(_hwndComboColor, TRUE, ComboColorItems);
        SendMessage(_hwndComboColor, CB_SETCURSEL, 0, 0);
        _fComboLoaded = TRUE;
   

        VARIANTARG varRange;
        LONG lLBound, lUBound, lIndex, lValue;
        BSTR  bstrBuf;
        TCHAR szBuf[64];
        SAFEARRAY * psa = NULL;
        HWND hwndCombobox = NULL;

        //
        // Load Sizes and Default Block Formats.
        //
        for (i = 0; ComboLoad[i].ComboIDM; i ++)
        {
            if (ComboLoad[i].fLoaded)
                continue;

            switch (ComboLoad[i].ComboIDM)
            {
            case IDM_FONTSIZE:
                hwndCombobox = _hwndComboSize;
                break;
            case IDM_GETBLOCKFMTS:
                hwndCombobox = _hwndComboTag;
                break;
            }
            varRange.vt = VT_ARRAY;
            varRange.parray = psa;
            
            IOleCommandTarget* pCommandTarget = NULL;

            if ( _pInPlaceObject )
                hr = THR( _pInPlaceObject->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget));

            if ( ! hr )
                hr = THR( pCommandTarget->Exec(
                        (GUID *)&CGID_MSHTML,
                        ComboLoad[i].ComboIDM,
                        MSOCMDEXECOPT_DONTPROMPTUSER,
                        NULL,
                        &varRange));

            pCommandTarget->Release();

            // NULL pointer will sometimes return huge values and go into
            // an infinite loop.
            if (OK(hr) && V_ARRAY(&varRange) != NULL)
            {
                psa = V_ARRAY(&varRange);
                SafeArrayGetLBound(psa, 1, &lLBound);
                SafeArrayGetUBound(psa, 1, &lUBound);
                for (lIndex = lLBound; lIndex <= lUBound; lIndex ++)
                {
                    switch (ComboLoad[i].ComboIDM)
                    {
                    case IDM_GETBLOCKFMTS:
                        SafeArrayGetElement(psa, &lIndex, &bstrBuf);
                        SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM) bstrBuf);
                        SysFreeString(bstrBuf);
                        break;

                    case IDM_FONTSIZE:
                        SafeArrayGetElement(psa, &lIndex, &lValue);
                        Format(0, szBuf, ARRAY_SIZE(szBuf), TEXT("<0d>"), lValue);
                        SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM) szBuf);
                        break;
                    }
                }

                // OLEAUT32 crashes if psa is NULL
                if (psa)
                {
                    SafeArrayDestroyData(psa);
                }
                SafeArrayDestroy(psa);

                ComboLoad[i].fLoaded = TRUE;
            }
        }

        _fComboLoaded = TRUE;
    }
    
}

///--------------------------------------------------------------
//
// Member:    CPadDoc::CreateToolBarUI
//
// Synopsis: Creates the PAD UI. We always create the "standard" toolbar
// and the Editing UI.
//
//
//---------------------------------------------------------------
HRESULT 
CPadDoc::CreateToolBarUI()
{
    HRESULT hr = S_OK;

    CreateStandardToolbar();
    CreateFormatToolbar();

    if ( IsEdit())
        ShowFormatToolbar();
    else
        HideFormatToolbar();

    RRETURN ( hr );
}



void
CPadDoc::DestroyToolbars()
{
    if (_hwndToolbar)
    {
        if (_hwndComboZoom) DestroyWindow(_hwndComboZoom);
        DestroyWindow(_hwndToolbar);
        _hwndComboZoom = NULL;
        _hwndToolbar = NULL;
    }
    if (_hwndFormatToolbar)
    {
        if ( _hwndComboTag) DestroyWindow(_hwndComboTag);
        if ( _hwndComboFont) DestroyWindow(_hwndComboFont);
        if ( _hwndComboSize) DestroyWindow(_hwndComboSize);
        if ( _hwndComboColor) DestroyWindow(_hwndComboColor);
        DestroyWindow(_hwndFormatToolbar);

        _hwndComboTag   = NULL;
        _hwndComboFont  = NULL;
        _hwndComboSize  = NULL;
        _hwndComboColor = NULL;
        _hwndFormatToolbar   = NULL;
    }
}

//+======================
// Move it to the right place
//
//-----------------------
HRESULT
CPadDoc::MoveFormatToolbar()
{
    HRESULT hr = S_OK;
    
    if (_hwndToolbar || _hwndFormatToolbar)
    {
        RECT         rcToolbar;
        RECT         rcFormatToolbar;

        if ( _hwndToolbar)
        {
            GetWindowRect(_hwndToolbar, &rcToolbar);
        }
        else
        {
            SetRectEmpty(&rcToolbar);
        }
        if ( _hwndFormatToolbar)
        {
            GetWindowRect(_hwndFormatToolbar, &rcFormatToolbar);
        }
        else
        {
            SetRectEmpty(&rcFormatToolbar);
        }

        if (hr)
            goto Error;

        if ( _hwndFormatToolbar)
        {
            RECT rcRect;
            GetWindowRect( _hwnd, &rcRect );

            ::MoveWindow(
                    _hwndFormatToolbar,
                    0, 
                    50 ,
                    rcFormatToolbar.right - rcFormatToolbar.left,
                    rcFormatToolbar.bottom - rcFormatToolbar.top,
                    TRUE);
        }
    }
Error:

    RRETURN_NOTRACE(hr);
}

BOOL CPadDoc::IsEditCommand(WORD idm)
{
    switch ( idm )
    {
        case IDM_IMAGE:
        case IDM_INSERTOBJECT:    	
        case IDM_HORIZONTALLINE:
        case IDM_LINEBREAKNORMAL:
        case IDM_LINEBREAKLEFT:
        case IDM_LINEBREAKRIGHT:
        case IDM_LINEBREAKBOTH:
        case IDM_NONBREAK:
        case IDM_PAGEBREAK:
        case IDM_SPECIALCHAR:
        case IDM_MARQUEE:
        case IDM_1D:
        case IDM_TEXTBOX:
#ifdef NEVER        
        case IDM_HTMLAREA:
#endif        
        case IDM_TEXTAREA:
        case IDM_CHECKBOX:
        case IDM_RADIOBUTTON:
        case IDM_DROPDOWNBOX:
        case IDM_LISTBOX:
        case IDM_BUTTON:
        case IDM_IFRAME:
            return TRUE;

        default:
            return FALSE;
        }
}

//+-------------------------------------------------------------------
//
//  Member:     CPadDoc::UpdateDirtyUI
//
//--------------------------------------------------------------------

LRESULT
CPadDoc::UpdateDirtyUI()
{
    BOOL  fDirty = GetDirtyState();
    BOOL  fDirtyUI;
    TCHAR szBuf[512];
    INT   iTitleLen;

    GetWindowText(_hwnd, szBuf, ARRAY_SIZE(szBuf));
    iTitleLen = _tcslen(szBuf);
    Assert(iTitleLen > 0);
    
    fDirtyUI = (szBuf[iTitleLen-1] == TCHAR('*'));

    if (fDirty != fDirtyUI)
    {
        if (fDirty)
        {
            Assert(iTitleLen < 511);
            szBuf[iTitleLen] = TCHAR('*');
            szBuf[iTitleLen+1] = 0;
        }
        else
        {
            szBuf[iTitleLen-1]= 0;            
        }
        SetWindowText(_hwnd, szBuf);
    }
    
    return 0;
}

