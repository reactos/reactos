//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padauto.cxx
//
//  Contents:   CPadDoc class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_PADDEBUG_HXX_
#define X_PADDEBUG_HXX_
#include "paddebug.hxx"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#include "platform.h"
#endif

#ifndef X_PRIVCID_H_
#define X_PRIVCID_H_
#include "privcid.h"
#endif

#ifndef X_UNICWRAP_HXX
#define X_UNICWRAP_HXX
#include "unicwrap.hxx"
#endif

#ifndef X_MSHTMSVR_H_
#define X_MSHTMSVR_H_
#include "mshtmsvr.h"
#endif

#ifndef X_OLEACC_H_
#define X_OLEACC_H_
#include "oleacc.h"
#endif

#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif

#ifndef X_CDUTIL_HXX_
#define X_CDUTIL_HXX_
#include "cdutil.hxx"
#endif


#undef ASSERT

DeclareTag(tagScriptLog, "ScriptLog", "script log");

extern DYNLIB g_dynlibMSHTML;

#define FILEBUFFERSIZE 2048

#ifndef UNIX
#define PRINTDRT_FILE _T("c:\\printdrt.ps")  // CONSIDER: use GetTempFileName instead.
#define PRINTDRT_FILEA "c:\\printdrt.ps"
#else
#define PRINTDRT_FILE _T("printdrt.ps")  // CONSIDER: use GetTempFileName instead.
#define PRINTDRT_FILEA "printdrt.ps"
#endif
#define WM_UPDATEDEFAULTPRINTER WM_USER+338

TCHAR achScriptPath[MAX_PATH] = { 0 };
TCHAR achScriptStartDir[MAX_PATH] = { 0 };
TCHAR achDRTPath[MAX_PATH] = { 0 };

#ifdef UNIX
void SanitizePath( TCHAR *pszPath )
{
    if ( pszPath ) {
        TCHAR *pch = pszPath;

        while (*pch) {
            if (( *pch == L'/') ||
                ( *pch == L'\\')) {
                *pch = _T(FILENAME_SEPARATOR);
            }

            pch++;
        }
    }
}
#endif

void PrimeDRTPath()
{
    if (!achDRTPath[0])
    {
        char achDRTPathA[MAX_PATH] = "";

        GetPrivateProfileStringA("PadDirs","DRTScript","",achDRTPathA,MAX_PATH, "mshtmdbg.ini");

        if (achDRTPathA[0])
        {
            MultiByteToWideChar(CP_ACP, 0, achDRTPathA, MAX_PATH, achDRTPath, MAX_PATH);
        }

    }

    if (!achDRTPath[0])
    {
        TCHAR  *pch;

        // Prime file name with drt file directory.

        // Start with the directory containing this file.

        _tcscpy(achDRTPath, _T(__FILE__));

        // Chop off the name of this file and three directories.
        // This will leave the root of our SLM tree in achScriptPath.

        for (int i = 0; i < 4; i++)
        {
            pch = _tcsrchr(achDRTPath, _T(FILENAME_SEPARATOR));
            if (pch)
                *pch = 0;
        }

        // Glue on the name of the directory containing the test cases.

        _tcscat(achDRTPath, _T(FILENAME_SEPARATOR_STR)
                               _T("src")
                               _T(FILENAME_SEPARATOR_STR)
                               _T("f3")
                               _T(FILENAME_SEPARATOR_STR)
                               _T("drt")
                               _T(FILENAME_SEPARATOR_STR)
                               _T("0drt.js"));
    }
}

void PrimeScriptStartDir()
{
    // look in mshtmdbg.ini first
    if(!achScriptStartDir[0])
    {
        char achScriptPathA[MAX_PATH] = "";

        GetPrivateProfileStringA("PadDirs","ScriptDir","",achScriptPathA,MAX_PATH, "mshtmdbg.ini");

        if (achScriptPathA[0])
        {
            MultiByteToWideChar(CP_ACP, 0, achScriptPathA, MAX_PATH, achScriptStartDir, MAX_PATH);
        }
    }

    // cobble something up form where we were compiled
    if (!achScriptStartDir[0])
    {
        TCHAR  *pch;

        // Prime file name with drt file directory.

        // Start with the directory containing this file.

        _tcscpy(achScriptStartDir, _T(__FILE__));

        // Chop off the name of this file and three directories.
        // This will leave the root of our SLM tree in achScriptPath.

        for (int i = 0; i < 4; i++)
        {
            pch = _tcsrchr(achScriptStartDir, _T(FILENAME_SEPARATOR));
            if (pch)
                *pch = 0;
        }

        // Glue on the name of the directory containing the test cases.

        _tcscat(achScriptStartDir,  _T(FILENAME_SEPARATOR_STR)
                                    _T("src")
                                    _T(FILENAME_SEPARATOR_STR)
                                    _T("f3")
                                    _T(FILENAME_SEPARATOR_STR)
                                    _T("drt"));
    }
}

HRESULT
CPadDoc::ExecuteDRT()
{
    HRESULT hr = S_OK;

    PrimeDRTPath();

    hr = THR(ExecuteTopLevelScript(achDRTPath));

    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
        hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        TCHAR achMsg[400];

        _tcscpy(achMsg, _T("Could not find the DRT script at "));
        _tcscat(achMsg, achDRTPath);
        _tcscat(achMsg, _T(".\nDo you wish to try and find the script yourself?"));

        if (MessageBox(_hwnd, achMsg, _T("Could not find DRT."), MB_YESNO) == IDYES)
        {
            hr = THR(PromptExecuteScript(TRUE));
        }
        else
        {
            hr = S_OK;
        }
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CPadDoc::PromptExecuteScript(BOOL fDRT)
{
    HRESULT         hr;
    OPENFILENAME    ofn;
    BOOL            f;
    TCHAR *         pchPath;
    TCHAR *         pchStartDir;

    if (fDRT)
    {
        // This should only be called from ExecuteDRT which
        // will already have called PrimeScriptPath.  We didn't
        // find the DRT start file so we open the dialog at
        // where we looked for it.
        Assert( *achDRTPath );
        pchPath = achDRTPath;
        pchStartDir = NULL;
    }
    else
    {
        if (!*achScriptStartDir)
        {
            PrimeScriptStartDir();
        }

        pchPath = achScriptPath;
        pchStartDir = *pchPath ? NULL : achScriptStartDir;
    }

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = _hwnd;
    ofn.lpstrFilter = TEXT("All Scripts (*.js;*.vbs)\0*.js;*.vbs\0JavaScript (*.js)\0*.js\0VBScript (*.vbs)\0*.vbs\0All Files (*.*)\0*.*\0");
    ofn.lpstrFile = pchPath;
    ofn.lpstrDefExt = _T("js");
    ofn.lpstrInitialDir = pchStartDir;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    DbgMemoryTrackDisable(TRUE);
    f = GetOpenFileName(&ofn);
    DbgMemoryTrackDisable(FALSE);

    if (!f)
        return S_FALSE;

    hr = THR(ExecuteTopLevelScript(pchPath));

    RRETURN(hr);
}

HRESULT
CPadDoc::LoadTypeLibrary()
{
    HRESULT hr = S_OK;
    TCHAR achExe[MAX_PATH];

    if (!_pTypeLibPad)
    {
        GetModuleFileName(g_hInstCore, achExe, MAX_PATH);

        //BUGBUG (carled) again the crt library shutdown causes oleaut32 to leak memory
        // which is allocated on the very first call to GetTypeLib. Remove this
        // block once the crt libraries are no longer linked in.

        DbgMemoryTrackDisable(TRUE);

        hr = THR(LoadTypeLib(achExe, &_pTypeLibPad));

        DbgMemoryTrackDisable(FALSE);

        if (hr)
            goto Cleanup;

        hr = THR(RegisterTypeLib(_pTypeLibPad, achExe, NULL));
        if (hr)
            goto Cleanup;
    }

    if (!_apTypeComp[0])
    {
        hr = THR(_pTypeLibPad->GetTypeComp(&_apTypeComp[0]));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoCPad)
    {
        hr = THR(_pTypeLibPad->GetTypeInfoOfGuid(CLSID_Pad, &_pTypeInfoCPad));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoIPad)
    {
        hr = THR(_pTypeLibPad->GetTypeInfoOfGuid(IID_IPad, &_pTypeInfoIPad));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoILine)
    {
        hr = THR(_pTypeLibPad->GetTypeInfoOfGuid(IID_ILine, &_pTypeInfoILine));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoICascaded)
    {
        hr = THR(_pTypeLibPad->GetTypeInfoOfGuid(IID_ICascaded, &_pTypeInfoICascaded));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeLibDLL)
    {
        // If specified, load MSHTML from the system[32] directory
        // instead of the from the exe directory.
        if (g_fLoadSystemMSHTML)
        {
            UINT uRet = GetSystemDirectory(achExe, sizeof(achExe));
            Assert(uRet);
            achExe[uRet] = FILENAME_SEPARATOR;

            achExe[uRet + 1] = 0;
        }

        TCHAR * pchName = _tcsrchr(achExe, FILENAME_SEPARATOR);
        if (!pchName)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

#ifdef UNIX
        _tcscpy(achExe, _T("mshtml.tlb"));
#else
        _tcscpy(pchName + 1, _T("mshtml.tlb"));
#endif
        hr = THR(LoadTypeLib(achExe, &_pTypeLibDLL));

        if (hr)
        {
#ifdef UNIX
            _tcscpy(achExe, _T("mshtml.dll"));
#else
            _tcscpy(pchName + 1, _T("mshtml.dll"));
#endif

            hr = THR(LoadTypeLib(achExe, &_pTypeLibDLL));
            if (hr)
                goto Cleanup;
        }
    }

    if (!_apTypeComp[1])
    {
        hr = THR(_pTypeLibDLL->GetTypeComp(&_apTypeComp[1]));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::PushScript
//
//  Create a new script site/engine and push it on the script stack
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::PushScript(TCHAR *pchType)
{
    HRESULT hr;
    CPadScriptSite * pScriptSite;

    hr = LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    pScriptSite = new CPadScriptSite(this);
    if(!pScriptSite)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pScriptSite->Init(pchType);
    if (hr)
    {
        delete pScriptSite;
        pScriptSite = NULL;
        goto Cleanup;
    }

    pScriptSite->_pScriptSitePrev = _pScriptSite;
    _pScriptSite = pScriptSite;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::PopScript
//
//  Pop last script site/engine off the script stack
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::PopScript()
{
    CPadScriptSite * pScriptSite = _pScriptSite;

    if(!_pScriptSite)
        return S_FALSE;

    // Script about to unload fire unload.
    FireEvent(DISPID_PadEvents_Unload, 0, NULL);

    _pScriptSite = _pScriptSite->_pScriptSitePrev;

    pScriptSite->Close();
    delete pScriptSite;

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CloseScripts
//
//  Clear the stack of script engines
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::CloseScripts()
{
    while(PopScript() == S_OK)
        ;

    Assert(_pScriptSite == NULL);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ExecuteTopLevelScript
//
//  Close previous top level script engine then load and execute script
//  in a new top level script engine
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ExecuteTopLevelScript(TCHAR * pchPath)
{
    HRESULT hr;

    // Stablize reference count during script execution.
    // Script can hide window which decrements reference count.

    AddRef();

    // Getting read to close the scripts fire unload event.
    CloseScripts();

    hr = THR(PushScript(_tcsrchr(pchPath, _T('.'))));
    if(hr)
        goto Cleanup;

    hr = THR(_pScriptSite->ExecuteScriptFile(pchPath));
    if(hr)
        goto Cleanup;

    hr = THR(_pScriptSite->SetScriptState(SCRIPTSTATE_CONNECTED));
    if (hr)
        goto Cleanup;

    FireEvent(DISPID_PadEvents_Load, 0, NULL);

Cleanup:
    Release();
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ExecuteScriptlet
//
//  Add a scriptlet to the current top level script engine and execute it
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ExecuteTopLevelScriptlet(TCHAR * pchScript)
{
    HRESULT hr;

    // Stablize reference count during script execution.
    // Script can hide window which decrements reference count.

    AddRef();

    if(!_pScriptSite)
    {
        hr = THR(PushScript(NULL));
        if(hr)
            goto Cleanup;
    }
    else
    {
        Assert(_pScriptSite->_pScriptSitePrev == NULL);
    }

    hr = THR(_pScriptSite->ExecuteScriptStr(pchScript));
    if (hr)
        goto Cleanup;

    hr = THR(_pScriptSite->SetScriptState(SCRIPTSTATE_CONNECTED));

Cleanup:
    Release();
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::FireEvent
//
//---------------------------------------------------------------------------

void
CPadDoc::FireEvent(DISPID dispid, UINT carg, VARIANTARG *pvararg)
{
    DISPPARAMS  dp;
    EXCEPINFO   ei;
    UINT        uArgErr = 0;

    if (_pScriptSite && _pScriptSite->_pDispSink)
    {
        dp.rgvarg            = pvararg;
        dp.rgdispidNamedArgs = NULL;
        dp.cArgs             = carg;
        dp.cNamedArgs        = 0;

        _pScriptSite->_pDispSink->Invoke(
                dispid,
                IID_NULL,
                0,
                DISPATCH_METHOD,
                &dp,
                NULL,
                &ei,
                &uArgErr);
    }
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::FireEvent
//
//---------------------------------------------------------------------------

void
CPadDoc::FireEvent(DISPID dispid, LPCTSTR pch)
{
    VARIANT var;

    V_BSTR(&var) = SysAllocString(pch);
    V_VT(&var) = VT_BSTR;
    FireEvent(dispid, 1, &var);
    SysFreeString(V_BSTR(&var));
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::FireEvent
//
//---------------------------------------------------------------------------

void
CPadDoc::FireEvent(DISPID dispid, BOOL fArg)
{
    VARIANT var;

    V_BOOL(&var) = fArg ? VARIANT_TRUE : VARIANT_FALSE;
    V_VT(&var) = VT_BOOL;
    FireEvent(dispid, 1, &var);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    HRESULT hr;

    hr = THR(LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    *pptinfo = _pTypeInfoIPad;
    (*pptinfo)->AddRef();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    HRESULT hr;

    hr = THR(LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(_pTypeInfoIPad->GetIDsOfNames(rgszNames, cNames, rgdispid));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,DISPPARAMS * pdispparams, VARIANT * pvarResult,EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr;

    hr = THR(LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(_pTypeInfoIPad->Invoke((IPad *)this, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr));

Cleanup:
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::DoEvents, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::DoEvents(VARIANT_BOOL fWait)
{
    PADTHREADSTATE * pts = GetThreadState();

    PerfLog1(tagPerfWatchPad, this, "+CPadDoc::DoEvents(fWait=%d)", fWait);

    pts->fEndEvents = FALSE;
    pts->cDoEvents += !!fWait;
    Run(!fWait);

    if (fWait && !pts->fEndEvents)
    {
        MessageBox(
            _hwnd,
            TEXT("Warning: DoEvents TRUE was not balanced by a call to EndEvents."),
            TEXT("EndEvents pad script error"),
            MB_APPLMODAL | MB_ICONERROR | MB_OK);
    }

    if (pts->fEndEvents && !fWait)
    {
        MessageBox(
            _hwnd,
            TEXT("Warning: EndEvents called inside a DoEvents FALSE."),
            TEXT("EndEvents pad script error"),
            MB_APPLMODAL | MB_ICONERROR | MB_OK);
    }

    pts->fEndEvents = FALSE;
    pts->cDoEvents -= !!fWait;

    PerfLog(tagPerfWatchPad, this, "-CPadDoc::DoEvents()");

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::EndEvents, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::EndEvents()
{
    PADTHREADSTATE * pts = GetThreadState();

    if (!pts->cDoEvents)
    {
        return S_OK;
    }

    pts->fEndEvents = TRUE;

    PerfLog(tagPerfWatchPad, this, "CPadDoc::EndEvents()");

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::OpenFileStream, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::OpenFileStream(BSTR bstrPath)
{
    HRESULT hr = S_OK;
    IStream *pStream = NULL;

    if (_fUseShdocvw)
        return E_FAIL;

    if (bstrPath && !*bstrPath)
        bstrPath = NULL;

    if (!bstrPath)
    {
        hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pStream));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(CreateStreamOnFile(bstrPath, STGM_READ|STGM_SHARE_DENY_NONE, &pStream));
        if (hr)
            goto Cleanup;
    }

    hr = THR(Open(pStream));

Cleanup:
    ReleaseInterface(pStream);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::OpenFile, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::OpenFile(BSTR bstrPath, BSTR bstrProgID)
{
    HRESULT hr = S_OK;
    VARIANT vEmpty;
    BOOL fUseBindToObject = FALSE;

    PerfLog1(tagPerfWatchPad, this, "+CPadDoc::OpenFile(\"%ls\")", bstrPath);

    if (bstrPath && !*bstrPath)
        bstrPath = NULL;

    if (bstrPath && (*bstrPath == '*'))
    {
        fUseBindToObject = TRUE;
        bstrPath += 1;
    }

#ifdef UNIX
    SanitizePath( bstrPath );
#endif

    if (bstrProgID && *bstrProgID)
    {
        CLSID clsid;

        hr = THR(CLSIDFromProgID(bstrProgID, &clsid));
        if(hr)
            goto Cleanup;

        hr = THR(Open(clsid, bstrPath));
    }
    else if (bstrPath)
    {
        if (_fUseShdocvw)
        {
            PerfLog(tagPerfWatchPad, this, "+CPadDoc::OpenFile GetBrowser");
            hr = THR(GetBrowser());
            PerfLog(tagPerfWatchPad, this, "-CPadDoc::OpenFile GetBrowser");
            if (hr)
            {
                goto Cleanup;
            }

            VariantInit(&vEmpty);

            PerfLog(tagPerfWatchPad, this, "+CPadDoc::OpenFile _pBrowser::Navigate()");
            hr = THR(_pBrowser->Navigate(bstrPath, &vEmpty, &vEmpty, &vEmpty, &vEmpty));
            PerfLog(tagPerfWatchPad, this, "-CPadDoc::OpenFile _pBrowser::Navigate()");

            PerfLog(tagPerfWatchPad, this, "+CPadDoc::OpenFile UpdateToolbarUI");
            UpdateToolbarUI();
            PerfLog(tagPerfWatchPad, this, "-CPadDoc::OpenFile UpdateToolbarUI");
        }
        else if (fUseBindToObject)
        {
            TCHAR ach[MAX_PATH];
            BOOL IsUrlPrefix(const TCHAR * pchPath);

            if (!IsUrlPrefix(bstrPath))
            {
                _tcscpy(ach, _T("file://"));
                _tcscat(ach, bstrPath);
                bstrPath = ach;
            }

            hr = THR(Open(bstrPath));
        }
        else
        {
            hr = THR(Open(CLSID_HTMLDocument, bstrPath));
        }
    }
    else
    {
        hr = THR(Open(CLSID_HTMLDocument, NULL));
    }

Cleanup:
    PerfLog(tagPerfWatchPad, this, "-CPadDoc::OpenFile()");
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::SaveFile, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::SaveFile(BSTR bstrPath)
{
    if(!bstrPath)
        return E_INVALIDARG;

    return Save(bstrPath);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CloseFile, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::CloseFile()
{
    HRESULT hr;

    hr = THR(Deactivate());
    if (hr)
        goto Cleanup;

    Welcome();

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ExecuteCommand, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ExecuteCommand(LONG lCmdID, VARIANT * pvarParam)
{
    HRESULT hr = S_OK;;
    IOleCommandTarget * pCommandTarget = NULL;

    if (!_pInPlaceObject)
        goto Cleanup;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
                            IID_IOleCommandTarget,
                            (void **)&pCommandTarget));
    if(hr)
        goto Cleanup;

    if(!pvarParam || pvarParam->vt != VT_ERROR)
    {
        hr = pCommandTarget->Exec(
                (GUID *)&CGID_MSHTML,
                lCmdID,
                MSOCMDEXECOPT_DONTPROMPTUSER,
                pvarParam,
                NULL);
    }
    else
    {
        hr = pCommandTarget->Exec(
                (GUID *)&CGID_MSHTML,
                lCmdID,
                0,
                NULL,
                NULL);
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetCommandState, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::QueryCommandStatus(LONG lCmdID, VARIANT * pvarState)
{
    HRESULT hr;
    IOleCommandTarget * pCommandTarget = NULL;

    if(!pvarState)
        return E_INVALIDARG;

    hr = THR_NOTRACE(_pInPlaceObject->QueryInterface(
                            IID_IOleCommandTarget,
                            (void **)&pCommandTarget));
    if(hr)
        goto Cleanup;

    pvarState->vt = VT_NULL;

    switch(lCmdID)
    {
    case IDM_FONTSIZE:
        pvarState->vt   = VT_I4;
        pvarState->lVal = 0;
        goto QueryWithExec;
    case IDM_BLOCKFMT:
        pvarState->vt      = VT_BSTR;
        pvarState->bstrVal = NULL;
        goto QueryWithExec;
    case IDM_FONTNAME:
        pvarState->vt      = VT_BSTR;
        pvarState->bstrVal = NULL;
        goto QueryWithExec;
    case IDM_FORECOLOR:
        pvarState->vt      = VT_I4;
        pvarState->lVal    = 0;
        goto QueryWithExec;

QueryWithExec:
        hr = THR(pCommandTarget->Exec(
                    (GUID *)&CGID_MSHTML,
                    lCmdID,
                    MSOCMDEXECOPT_DONTPROMPTUSER,
                    NULL,
                    pvarState));
        break;


    default:
        MSOCMD rgCmds[1];

        rgCmds[0].cmdID = lCmdID;

        hr = pCommandTarget->QueryStatus(
                (GUID *)&CGID_MSHTML,
                1,
                rgCmds,
                NULL);
        if(hr)
            goto Cleanup;

        pvarState->vt = VT_I4;
        pvarState->lVal = rgCmds[0].cmdf;
        break;
    }

Cleanup:
    ReleaseInterface(pCommandTarget);
    RRETURN(hr);
}

//+--------------------------------------------------------------------------
//
//  Member : MoveMouseTo
//
//  Synopsis : this exposes mouse control through the pad's OM so that
//      (like send keys) script files, and drt tests can verify mouse
//      events and behavior.  the mosue api's in t3ctrl.dll is used.
//      the X and Y parameters are assumed to be in client coordiantes.
//
//+--------------------------------------------------------------------------

HRESULT
CPadDoc::MoveMouseTo(int x, int y, VARIANT_BOOL fLeftButton, int keyState)
{
    HRESULT hr = S_OK;
    POINT   pt;
    HWND    hwnd;
    WPARAM  wParam=0;
    LPARAM  lParam=0;

    // get the appropriate HWND for this event
    if ((hwnd = GetFocus()) != NULL)
    {
        // use focus window if we can get one
    }
    else if (_pInPlaceActiveObject &&
            OK(_pInPlaceActiveObject->GetWindow(&hwnd)))
    {
        // use inplace active object hwnd
    }
    else if (_pInPlaceObject &&
            OK(_pInPlaceObject->GetWindow(&hwnd)))
    {
        // use inplace object window
    }
    else
    {
        // use our window
        hwnd = _hwnd;
    }

    pt.x = x;
    pt.y = y;

    if (!ClientToScreen(hwnd, &pt))
    {
        hr = GetLastError();
        goto Cleanup;
    }

    // first move the mosue, but because the accompaning
    // mosemove msg is flaky, we want to send another
    // immediately
    if(!SetCursorPos(pt.x, pt.y))
    {
        hr = GetLastError();
        goto Cleanup;
    }

    // set up the LParam
    lParam = pt.y;
    lParam = lParam << 16;  // make pt.y the HIWORD
    lParam += pt.x;         // set the LOWORD

    //set up the key part of the lParam
    if (keyState & 0x0001)
        wParam = MK_CONTROL;

    if (keyState & 0x0002)
        wParam |= MK_SHIFT;

    wParam |= (fLeftButton==VB_TRUE) ? MK_LBUTTON : 0;

    // SendMessage to make this happen now
    if (!SendMessage(hwnd, WM_MOUSEMOVE, wParam, lParam))
    {
        hr = GetLastError();
        goto Cleanup;
    }


Cleanup:
    RRETURN( hr );
}


//---------------------------------------------------------------------------
//
//  Member : DoMouseButtonAt
//
//  Synopsis: iniate a mouse button event
//
//---------------------------------------------------------------------------

//
// BUGBUG - duplication between DoMouseButton and DoMouseButtonAt.
//
HRESULT
CPadDoc::DoMouseButtonAt(int x, int y, VARIANT_BOOL fLeftButton, BSTR bstrAction, int keyState)
{
    HRESULT hr = S_OK;
    POINT   pt;
    HWND    hwnd;
    UINT    uMsg=0;
    WPARAM  wParam=0;
    LPARAM  lParam=0;
    // get the appropriate HWND for this event
    if ((hwnd = GetFocus()) != NULL)
    {
        // use focus window if we can get one
    }
    else if (_pInPlaceActiveObject &&
            OK(_pInPlaceActiveObject->GetWindow(&hwnd)))
    {
        // use inplace active object hwnd
    }
    else if (_pInPlaceObject &&
            OK(_pInPlaceObject->GetWindow(&hwnd)))
    {
        // use inplace object window
    }
    else
    {
        // use our window
        hwnd = _hwnd;
    }
    pt.x = x;
    pt.y = y;

    // set up the LParam
    lParam = pt.y;
    lParam = lParam << 16;  // make pt.y the HIWORD
    lParam += pt.x;         // set the LOWORD

    //set up the key part of the lParam
    if (keyState & 0x0001)
        wParam = MK_CONTROL;

    if (keyState & 0x0002)
        wParam |= MK_SHIFT;

    // set up the Msg and the wParam
    if (_tcsicmp(bstrAction, _T("down"))==0)
    {
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONDOWN;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONDOWN;
            wParam |= MK_RBUTTON;
        }
    }
    else if (_tcsicmp(bstrAction, _T("up"))==0)
    {
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONUP;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONUP;
            wParam |= MK_RBUTTON;
        }
    }
    else if (_tcsicmp(bstrAction, _T("click"))==0)
    {
        // down
        hr = THR_NOTRACE(DoMouseButtonAt(x, y, fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        // up
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONUP;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONUP;
            wParam |= MK_RBUTTON;
        }
    }
    else if ((_tcsicmp(bstrAction, _T("doubleclick"))==0) ||
            (_tcsicmp(bstrAction, _T("double"))==0))
    {
        // oh oh gotta do some work, simulate by sending down, up, down, dbl
        hr = THR_NOTRACE(DoMouseButtonAt(x,y,fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(DoMouseButtonAt(x,y, fLeftButton, _T("up"), keyState));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(DoMouseButtonAt(x,y, fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONDBLCLK;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONDBLCLK;
            wParam |= MK_RBUTTON;
        }
    }

    if (!PostMessage(hwnd, uMsg, wParam, lParam))
        hr = GetLastError();

Cleanup:
    RRETURN ( hr );
}

//---------------------------------------------------------------------------
//
//  Member : DoMouseButton
//
//  Synopsis: iniate a mouse button event
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::DoMouseButton(VARIANT_BOOL fLeftButton, BSTR bstrAction, int keyState)
{
    HRESULT hr = S_OK;
    POINT   pt;
    HWND    hwnd;
    UINT    uMsg=0;
    WPARAM  wParam=0;
    LPARAM  lParam=0;

    // get the cursor's position
    if (!GetCursorPos(&pt))
    {
        hr = GetLastError();
        goto Cleanup;
    }

    // and the window that is going to get the message

    hwnd = WindowFromPoint(pt);
    if (!hwnd)
        goto Cleanup;

    // adjust the point
    if (!ScreenToClient(hwnd, &pt))
    {
        hr = GetLastError();
        goto Cleanup;
    }

    // set up the LParam
    lParam = pt.y;
    lParam = lParam << 16;  // make pt.y the HIWORD
    lParam += pt.x;         // set the LOWORD

    //set up the key part of the lParam
    if (keyState & 0x0001)
        wParam = MK_CONTROL;

    if (keyState & 0x0002)
        wParam |= MK_SHIFT;

    // set up the Msg and the wParam
    if (_tcsicmp(bstrAction, _T("down"))==0)
    {
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONDOWN;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONDOWN;
            wParam |= MK_RBUTTON;
        }
    }
    else if (_tcsicmp(bstrAction, _T("up"))==0)
    {
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONUP;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONUP;
            wParam |= MK_RBUTTON;
        }
    }
    else if (_tcsicmp(bstrAction, _T("click"))==0)
    {
        // down
        hr = THR_NOTRACE(DoMouseButton(fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        // up
        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONUP;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONUP;
            wParam |= MK_RBUTTON;
        }
    }
    else if ((_tcsicmp(bstrAction, _T("doubleclick"))==0) ||
            (_tcsicmp(bstrAction, _T("double"))==0))
    {
        // oh oh gotta do some work, simulate by sending down, up, down, dbl
        hr = THR_NOTRACE(DoMouseButton(fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(DoMouseButton(fLeftButton, _T("up"), keyState));
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(DoMouseButton(fLeftButton, _T("down"), keyState));
        if (hr)
            goto Cleanup;

        if (fLeftButton==VB_TRUE)
        {
            uMsg = WM_LBUTTONDBLCLK;
            wParam |= MK_LBUTTON;
        }
        else
        {
            uMsg = WM_RBUTTONDBLCLK;
            wParam |= MK_RBUTTON;
        }
    }

    if (!PostMessage(hwnd, uMsg, wParam, lParam))
        hr = GetLastError();

Cleanup:
    RRETURN ( hr );
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::PrintStatus, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::PrintStatus(BSTR bstrMessage)
{
    SetStatusText(bstrMessage);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ScriptParam, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_ScriptParam(VARIANT *pvarScriptParam)
{
    HRESULT hr;

    if (_pScriptSite)
    {
        hr = THR(VariantCopy(pvarScriptParam, &_pScriptSite->_varParam));
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN(hr);
}



//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ScriptObject, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_ScriptObject(IDispatch **ppDisp)
{
    HRESULT hr;

    if (_pScriptSite)
    {
        hr = THR(_pScriptSite->_pScript->GetScriptDispatch(_T("Pad"), ppDisp));
    }
    else
    {
        *ppDisp = NULL;
        hr = E_FAIL;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ExecuteScript, IPad
//
//  Load and execute script file in a brand new script engine
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ExecuteScript(BSTR bstrPath, VARIANT *pvarScriptParam, VARIANT_BOOL fAsync)
{
    HRESULT hr;

    if(!bstrPath)
        return E_INVALIDARG;

    hr = THR(PushScript(_tcsrchr(bstrPath, _T('.'))));
    if(hr)
        goto Cleanup;

    SetWindowText(_hwnd, bstrPath);

    if (pvarScriptParam && pvarScriptParam->vt != VT_ERROR)
    {
        hr = THR(VariantCopy(&_pScriptSite->_varParam, pvarScriptParam));
        if (hr)
            goto Cleanup;
    }

    hr = THR(_pScriptSite->ExecuteScriptFile(bstrPath));

    hr = THR(_pScriptSite->SetScriptState(SCRIPTSTATE_CONNECTED));
    if (hr)
        goto Cleanup;

    if (fAsync)
    {
        PostMessage(_hwnd, WM_RUNSCRIPT, 0, 0);
    }
    else
    {
        FireEvent(DISPID_PadEvents_Load, 0, NULL);
        PopScript();
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::RegisterControl, IPad
//
//  Synopsis:   Register the control whose fully qualified name is passed
//              in Path.
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::RegisterControl(BSTR Path)
{
#ifdef UNIX
    // Fix path in case we succeed but silently fail

    SanitizePath( Path );
    RegisterDLL( Path );

    return S_OK;
#else
    RRETURN(RegisterDLL(Path));
#endif
}



//---------------------------------------------------------------------------
//
//  Member: CPadDoc::IncludeScript, IPad
//
//  Load and execute script file in current script engine
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::IncludeScript(BSTR bstrPath)
{
    HRESULT hr;

    if(!bstrPath)
        return E_INVALIDARG;

    hr = THR(_pScriptSite->ExecuteScriptFile(bstrPath));

    RRETURN(hr);
}



//---------------------------------------------------------------------------
//
//  Member: CPadDoc::SetProperty, IPad
//
//  Sets a property for an object, given property name and value
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::SetProperty(IDispatch * pDisp, BSTR bstrProperty, VARIANT * pvar)
 {
    HRESULT     hr;
    DISPPARAMS  dp;
//    EXCEPINFO   ei;
    UINT        uArgErr = 0;
    DISPID      dispidProp;
    DISPID      dispidPut = DISPID_PROPERTYPUT;

    if (!pvar)
    {
        return E_INVALIDARG;
    }

    hr = pDisp->GetIDsOfNames(IID_NULL, &bstrProperty, 1, 0, &dispidProp);
    if (hr)
        goto Cleanup;

    dp.rgvarg            = pvar;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cArgs             = 1;
    dp.cNamedArgs        = 1;

    hr = pDisp->Invoke(
                dispidProp,
                IID_NULL,
                0,
                DISPATCH_PROPERTYPUT,
                &dp,
                NULL,
                NULL,   // use excepinfo?
                &uArgErr);

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetProperty, IPad
//
//  Gets a property value for an object, given property name
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetProperty(IDispatch * pDisp, BSTR bstrProperty, VARIANT * pvar)
{
    HRESULT     hr;
    DISPPARAMS  dp;
//    EXCEPINFO   ei;
    UINT        uArgErr = 0;
    DISPID      dispidProp;

    if (!pvar)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pDisp->GetIDsOfNames(IID_NULL, &bstrProperty, 1, 0, &dispidProp);
    if (hr)
        goto Cleanup;

    dp.rgvarg            = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs             = 0;
    dp.cNamedArgs        = 0;

    hr = pDisp->Invoke(
                dispidProp,
                IID_NULL,
                0,
                DISPATCH_PROPERTYGET,
                &dp,
                pvar,
                NULL,       // use excepinfo?
                &uArgErr);

Cleanup:
    if (hr)
    {
        pvar->vt = VT_NULL;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_Document, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_Document(IDispatch **ppDocDisp)
{
    HRESULT hr = S_OK;
    IHTMLDocument2 *pOmDoc;

    if(!ppDocDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDocDisp = NULL;

    if (_pBrowser)
    {
        hr = THR(_pBrowser->get_Document(ppDocDisp));
    }
    else if (_pObject)
    {
        hr = THR(GetOmDocumentFromDoc (_pObject, &pOmDoc));
        if (OK(hr))
        {
            *ppDocDisp = pOmDoc;
        }
    }

Cleanup:
    RRETURN (hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_TempPath, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_TempPath(BSTR * pbstrPath)
{
    TCHAR achTempPath[MAX_PATH];
    int cch;

    if(!pbstrPath)
        return E_INVALIDARG;

    cch = GetTempPath(ARRAY_SIZE(achTempPath), achTempPath);

    if(cch == 0 || cch > ARRAY_SIZE(achTempPath))
        return E_FAIL;

    *pbstrPath = SysAllocString(achTempPath);

    if(!*pbstrPath)
        return E_OUTOFMEMORY;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_TempFile, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetTempFileName(BSTR * pbstrName)
{
    HRESULT hr;
    BSTR bstrPath;
    TCHAR achTempFile[MAX_PATH];

    if(!pbstrName)
        return E_INVALIDARG;

    hr = get_TempPath(&bstrPath);
    if(hr)
        goto Cleanup;

    if(::GetTempFileName(bstrPath, _T("PAD"), 0, achTempFile) == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *pbstrName = SysAllocString(achTempFile);

    if(!*pbstrName)
        hr = E_OUTOFMEMORY;

Cleanup:
    SysFreeString(bstrPath);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CleanupTempFiles, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::CleanupTempFiles()
{
    HRESULT          hr;
    BSTR             bstrPath;
    CHAR             achTempFile[MAX_PATH];
    HANDLE           hFiles;
    WIN32_FIND_DATAA fd;
    CHAR          *  pch;
    int              c;

    //
    // This is all done in ANSI because we don't have unicode wrappers for
    // FindFirstFile and etc.
    //
    hr = get_TempPath(&bstrPath);
    if (hr)
        goto Cleanup;

    c = WideCharToMultiByte(CP_ACP,
                            0,
                            bstrPath,
                            SysStringLen(bstrPath),
                            achTempFile,
                            MAX_PATH,
                            NULL,
                            NULL);

    pch = achTempFile + c;

    Assert(*(pch-1) == FILENAME_SEPARATOR);

    strcpy(pch, "PAD*.TMP");

    hFiles = FindFirstFileA(achTempFile, &fd);

    if (hFiles == INVALID_HANDLE_VALUE)
    {
        hr = S_OK;
        goto Cleanup;
    }

    do
    {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            strcpy(pch, fd.cFileName);

            (void) DeleteFileA(achTempFile); // Ignore errors
        }
    } while (FindNextFileA(hFiles, &fd));

    FindClose(hFiles);

Cleanup:
    SysFreeString(bstrPath);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::PrintLog, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::PrintLog(BSTR bstrLine)
{
    DWORD  cchLen = SysStringLen(bstrLine);
    char * pchBuf = new char[cchLen+3];

    WideCharToMultiByte(CP_ACP, 0, bstrLine, cchLen, pchBuf, cchLen+1, NULL, NULL);
    pchBuf[cchLen] = '\0';

    TraceTagEx((tagScriptLog, TAG_NONAME, "%s", pchBuf));

    DWORD  cbWrite;

    //
    // Write to STDOUT. Used by the DRTDaemon process to relay information
    // back to the build machine. Ignore any errors.
    //
    strcat(pchBuf, "\r\n");

    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
              pchBuf,
              cchLen+2,
              &cbWrite,
              NULL);

    delete pchBuf;

    RRETURN(S_OK);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::PrintDebug, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::PrintDebug(BSTR bstrValue)
{
    if(!_pDebugWindow)
        ToggleDebugWindowVisibility();

    Assert(_pDebugWindow);

    RRETURN(THR(_pDebugWindow->Print(bstrValue)));
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::DRTPrint, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::DRTPrint(long Flags, VARIANT_BOOL * Success)
{
    HRESULT hr = E_FAIL;
    VARIANT varPrintFlags;
    VARIANT_BOOL fOutputFileExists;

    // 1. Print the current doc.

    V_VT(&varPrintFlags) = VT_I2;
    V_I2(&varPrintFlags) = (short) Flags;

    hr = THR( ExecuteCommand(IDM_PRINT, &varPrintFlags) );
    if (hr)
        goto Cleanup;

    // 2. Wait until the DRT output file exists.
    do
    {
        hr = THR( FileExists(PRINTDRT_FILE, &fOutputFileExists) );

        if (hr)
            goto Cleanup;
    }
    while (fOutputFileExists == VB_FALSE && MsgWaitForMultipleObjects(0, NULL, FALSE, 250, 0) == WAIT_TIMEOUT);

    #if (defined(_X86_))
    // 3. Fire up a postscript viewer.
    if (!(Flags & 128))
    {
        HINSTANCE hInstance = 0;
        HWND hwndDesktop = GetDesktopWindow();

        if (g_fUnicodePlatform)
        {
            hInstance = ShellExecuteW(hwndDesktop, NULL, _T("gs.bat"), PRINTDRT_FILE, NULL, SW_HIDE);
        }
        else
        {
            hInstance = ShellExecuteA(hwndDesktop, NULL, "gs.bat", PRINTDRT_FILEA, NULL, SW_HIDE);
        }

        Assert((LONG) hInstance > 32 && "Error bringing up a postscript viewer.  Verify C:\\PRINTDRT.PS manually by printing it.");
    }
    #endif

Cleanup:

    *Success = (hr == S_OK) ? VB_TRUE : VB_FALSE;

    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::SetDefaultPrinter, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::SetDefaultPrinter(BSTR bstrNewDefaultPrinter, VARIANT_BOOL * Success)
{
    HRESULT hr= E_FAIL;
    *Success = VB_FALSE;;

#if (defined(_X86_))
    HWND hwndDesktopWindow = GetDesktopWindow();
    HWND hwndInternetExplorer = 0;

    if (g_fUnicodePlatform)
    {
       hr = WriteProfileStringW(_T("Windows"), _T("Device"), (const TCHAR *)bstrNewDefaultPrinter)
           ? S_OK : E_FAIL;


        if (!hr && hwndDesktopWindow)
        {
            // make sure the spooler knows about the new default printer
            // Cause MSHMTL.DLL to re-read registry settings
            do
            {
                hwndInternetExplorer = FindWindowExW(hwndDesktopWindow, hwndInternetExplorer, _T("Internet Explorer_Hidden"), NULL);


                if (hwndInternetExplorer)
                {
                    SendMessage(hwndInternetExplorer, WM_UPDATEDEFAULTPRINTER, 0, 0);
                }
            }
            while (hwndInternetExplorer);
        }
    }
    else
    {
        char strNewDefaultPrinter[100];
        DWORD cbNewDefaultPrinter = WideCharToMultiByte(
                CP_ACP, 0, bstrNewDefaultPrinter, -1, strNewDefaultPrinter, 100, NULL, NULL);

        hr = (cbNewDefaultPrinter && WriteProfileStringA("Windows", "Device", (const char *)strNewDefaultPrinter))
            ? S_OK : E_FAIL;

        if (!hr && hwndDesktopWindow)
        {
            // make sure the spooler knows about the new default printer
            // Cause MSHMTL.DLL to re-read registry settings
            do
            {
                hwndInternetExplorer = FindWindowExA(hwndDesktopWindow, hwndInternetExplorer, "Internet Explorer_Hidden", NULL);

                if (hwndInternetExplorer)
                {
                    SendMessage(hwndInternetExplorer, WM_UPDATEDEFAULTPRINTER, 0, 0);
                }
            }
            while (hwndInternetExplorer);
        }
    }

    *Success = hr ? VB_FALSE : VB_TRUE;

#endif // _X86_

    RRETURN1(hr, E_FAIL);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CompareFiles, IPad
//          and AreFilesSame helper fn()
//          and IsPostscript helper fn()
//          and ComparePostscriptFiles helper fn()
//
//---------------------------------------------------------------------------

BOOL
AreFilesSame(HANDLE hFile1, HANDLE hFile2)
{
    DWORD dwFile1Size, dwFile2Size, dwCurSize, dwActualRead;

    Assert(hFile1 != INVALID_HANDLE_VALUE && hFile2 != INVALID_HANDLE_VALUE);

    // get and compare the files
    dwFile1Size = GetFileSize(hFile1, NULL);
    dwFile2Size = GetFileSize(hFile2, NULL);

    if (dwFile1Size != dwFile2Size)
        return FALSE;

    dwCurSize = 0;

    while (dwCurSize < dwFile1Size)
    {
        DWORD dwRead;
        char Buf1[FILEBUFFERSIZE], Buf2[FILEBUFFERSIZE];
        dwCurSize += FILEBUFFERSIZE;

        // determine number of bytes to read
        if (dwCurSize <= dwFile1Size)
            dwRead = FILEBUFFERSIZE;
        else
            dwRead = dwFile1Size - (dwCurSize - FILEBUFFERSIZE);

        // read a portion of the first file
        if (!ReadFile(hFile1, Buf1, dwRead, &dwActualRead, NULL) ||
            !ReadFile(hFile2, Buf2, dwRead, &dwActualRead, NULL))
        {
            TraceTag((tagError, "ERROR reading one of the htm files."));
            return FALSE;
        }

        if (memcmp(Buf1, Buf2, dwActualRead) != 0)
            return FALSE;
    }
    return TRUE;
}

BOOL IsPostscript(BSTR bstrFile)
{
    if (!bstrFile)
        return FALSE;

    if (_tcslen(bstrFile) < 3)
        return FALSE;

    return (StrCmpIC(_T(".ps"), bstrFile+_tcslen(bstrFile)-3) == 0);
}

HRESULT
ReadNextPostscriptLine(HANDLE hFile, char *lpBuffer, LPDWORD pdwLineLength)
{
    char  * lpFirstNewLine;
    char  * lpFirstRandomIdentifier;
    int     cch;

    Assert(lpBuffer);

    do
    {
        // read a portion of the file
        if (!ReadFile(hFile, lpBuffer, FILEBUFFERSIZE, pdwLineLength, NULL))
        {
            TraceTag((tagError, "ERROR reading one of the postscript files."));
            return E_FAIL;
        }

        // end of file?
        if (*pdwLineLength == 0)
            return S_OK;

        // find the first new line in the buffer
        for (cch = *pdwLineLength, lpFirstNewLine = lpBuffer; cch > 0; --cch, ++lpFirstNewLine)
            if (*lpFirstNewLine == 10)
                break;
        if (*lpFirstNewLine != 10)
            lpFirstNewLine = NULL;

        // if we found a new line, reposition the file pointer
        if (lpFirstNewLine)
        {
            SetFilePointer(hFile, (lpFirstNewLine - lpBuffer + 1) - *pdwLineLength, NULL, FILE_CURRENT);

            *pdwLineLength = lpFirstNewLine - lpBuffer;
        }
    }
    while (lpBuffer[0] == '%' && lpBuffer[1] == '%');

    // before we return successfully, we have to NULL out all font identifiers starting with
    // MSTT followed by a 8-digit hexcode.

    lpBuffer[*pdwLineLength] = 0;
    lpFirstRandomIdentifier = lpBuffer;

    do
    {
        // Find first occurence of "MSTT"
        lpFirstRandomIdentifier = StrStrA(lpFirstRandomIdentifier, "MSTT");

        // If found, NULL out the random 8-digit hexcode following it.
        if (lpFirstRandomIdentifier)
        {
            int nPadding = *pdwLineLength - (lpFirstRandomIdentifier+4-lpBuffer); // chars left in line

            // Zero out at most 8 characters.
            if (nPadding > 8)
                nPadding = 8;

            memset( lpFirstRandomIdentifier+4, '0', nPadding );

            // prevent finding the same occurence again.
            lpFirstRandomIdentifier++;
        }
    }
    while (lpFirstRandomIdentifier);

    return S_OK;
}

BOOL
ComparePostscriptFiles(HANDLE hFile1, HANDLE hFile2)
{
    DWORD dwActualRead1, dwActualRead2;

    Assert(hFile1 != INVALID_HANDLE_VALUE && hFile2 != INVALID_HANDLE_VALUE);

    do
    {
        char Buf1[FILEBUFFERSIZE], Buf2[FILEBUFFERSIZE];
        HRESULT hr;

        hr = ReadNextPostscriptLine(hFile1, Buf1, &dwActualRead1);
        if (hr)
            return FALSE;

        hr = ReadNextPostscriptLine(hFile2, Buf2, &dwActualRead2);
        if (hr)
            return FALSE;

        if (dwActualRead1 && dwActualRead2)
        {
            // Compare lengths of lines.
            if (dwActualRead1 != dwActualRead2)
                return FALSE;

            // Compare lines.
            if (memcmp(Buf1, Buf2, dwActualRead1) != 0)
                return FALSE;
        }
    }
    // We compare until one of the files reaches its end.
    while (dwActualRead1 && dwActualRead2);

    return TRUE;
}

HRESULT
CPadDoc::CompareFiles(BSTR bstrFile1, BSTR bstrFile2, VARIANT_BOOL *pfMatch)
{
    HANDLE hFile1;
    HANDLE hFile2;
    BOOL fMatch = TRUE;

    hFile1 = CreateFile(bstrFile1, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile1 == INVALID_HANDLE_VALUE)
        fMatch = FALSE;

    hFile2 = CreateFile(bstrFile2, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile2 == INVALID_HANDLE_VALUE)
        fMatch = FALSE;

    if (fMatch)
    {
        fMatch =
        // Are we comparing postscript files?
        (IsPostscript(bstrFile1) && IsPostscript(bstrFile2)) ?
            // Comparing postscript files requires dropping postscript
            // comments (%%) that contain irrelevant, but distinct information
            // such as the creation date, time, and environment.
            ComparePostscriptFiles(hFile1,hFile2) :
            // else do normal compare
            AreFilesSame(hFile1, hFile2);
    }
    if (hFile1 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile1);
    if (hFile2 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile2);

    *pfMatch = fMatch ? VB_TRUE : VB_FALSE;
    RRETURN(S_OK);
}

HRESULT
CPadDoc::CopyThisFile(BSTR bstrFile1, BSTR bstrFile2, VARIANT_BOOL *pfSuccess)
{
    *pfSuccess = CopyFile(bstrFile1, bstrFile2, FALSE) ? VB_TRUE : VB_FALSE;
    RRETURN(S_OK);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::FileExists, IPad
//
//  Synopsis:  Return TRUE if file exists.
//---------------------------------------------------------------------------

HRESULT
CPadDoc::FileExists(BSTR bstrFile, VARIANT_BOOL *pfFileExists)
{
    DWORD dwReturn;
    LPTSTR pDummy;

    dwReturn = SearchPath(NULL, bstrFile, NULL, 0, NULL, &pDummy);
    *pfFileExists = (dwReturn > 0) ? VB_TRUE : VB_FALSE;

    if (*pfFileExists == VB_TRUE)
    {
        HANDLE hFile = CreateFile(bstrFile, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            *pfFileExists = VB_FALSE;
        else
            CloseHandle(hFile);
    }

    RRETURN(S_OK);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::DisableDialogs, IPad
//
//  Note: UnhandledExceptionHandler is the global exception handler that gets
//        called whenever an unhandled exception occurs in Pad. This is
//        installed when SetUnhandledExceptionFilter is called from
//        DisableDialogs.
//
//---------------------------------------------------------------------------

LONG UnhandledExceptionHandler(LPEXCEPTION_POINTERS lpexpExceptionInfo)
{
    HANDLE hProcess;

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());
    if (hProcess)
    {
        char * pszExType = NULL;
        char   achBuf[200];

        switch (lpexpExceptionInfo->ExceptionRecord->ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            pszExType = "Access Violation";
            break;

        case EXCEPTION_BREAKPOINT:
            pszExType = "Breakpoint Hit";
            break;

        case EXCEPTION_STACK_OVERFLOW:
            pszExType = "Stack Overflow";
            break;

        default:
            pszExType = "Unhandled Exception";
            break;
        }

        wsprintfA(achBuf, "*** FATAL ERROR: %s at", pszExType);

        DbgExGetSymbolFromAddress(lpexpExceptionInfo->ExceptionRecord->ExceptionAddress,
                                  &achBuf[strlen(achBuf)], ARRAY_SIZE(achBuf) - strlen(achBuf) - 2);

        //
        // We explicitly turn off the assert stacktrace here because it only
        // gives the stacktrace of this UnhandledExceptionHandler, which means
        // nothing and just confuses people.
        //
        BOOL fStacktrace = DbgExEnableTag(tagAssertStacks, FALSE);

        AssertSz(FALSE, achBuf);

        DbgExEnableTag(tagAssertStacks, fStacktrace);

        TerminateProcess(hProcess, 1);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

HRESULT
CPadDoc::DisableDialogs()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);
    EnableTag(tagAssertExit, TRUE);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_DialogsDisabled, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_DialogsEnabled(VARIANT_BOOL *pfEnabled)
{
    *pfEnabled = IsTagEnabled(tagAssertExit) ? VB_FALSE : VB_TRUE;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CPadDoc::get_ScriptBase, IPad
//
//  Arguments:  i  - Number of levels back up the script path to return.
//                   0 returns directory containing script.
//                   1 returns parent of directory containing script
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_ScriptPath(long i, BSTR * pbstrPath)
{
    TCHAR achPath[MAX_PATH];
    TCHAR *pch = NULL;

    *pbstrPath = NULL;
    if (_pScriptSite)
    {
        memcpy(achPath, _pScriptSite->_achPath, sizeof(achPath));
        for (; i >= 0; i--)
        {
            pch = _tcsrchr(achPath, _T(FILENAME_SEPARATOR));
            if (pch)
            {
                *pch = 0;
            }
            else
            {
                return E_FAIL;
            }
        }
        if (pch)
        {
            pch[0] = _T(FILENAME_SEPARATOR);
            pch[1] = 0;
        }
        *pbstrPath = SysAllocString(achPath);
    }

    return *pbstrPath ? S_OK : E_FAIL;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::put_TimerInterval, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::put_TimerInterval(long lInterval)
{
    KillTimer(_hwnd, 0);
    _lTimerInterval = lInterval;
    if (_lTimerInterval)
    {
        SetTimer(_hwnd, 0, _lTimerInterval, NULL);
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_TimerInterval, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_TimerInterval(long *plInterval)
{
    *plInterval = _lTimerInterval;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ResumeCAP, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ResumeCAP()
{
    ::ResumeCAPAll();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::SuspendCAP, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::SuspendCAP()
{
    ::SuspendCAPAll();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::StartCAP, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::StartCAP()
{
    ::StartCAPAll();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::StopCAP, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::StopCAP()
{
    ::StopCAPAll();
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::TicStartAll, IPad
//  Member: CPadDoc::TicStopAll, IPad
//
//---------------------------------------------------------------------------

DYNLIB g_dynlibTimeLib = { NULL, NULL, "TimeLib.DLL" };
DYNPROC g_dynprocTicStartAll = { NULL, &g_dynlibTimeLib, "TicStartAll" };
DYNPROC g_dynprocTicStopAll = { NULL, &g_dynlibTimeLib, "TicStopAll" };

HRESULT
CPadDoc::TicStartAll()
{
    if (    g_dynprocTicStartAll.pfn != NULL
        ||  LoadProcedure(&g_dynprocTicStartAll) == S_OK)
    {
        ((HRESULT (STDAPICALLTYPE *)())g_dynprocTicStartAll.pfn)();
    }

    return(S_OK);
}

HRESULT
CPadDoc::TicStopAll()
{
    if (    g_dynprocTicStopAll.pfn != NULL
        ||  LoadProcedure(&g_dynprocTicStopAll) == S_OK)
    {
        ((HRESULT (STDAPICALLTYPE *)())g_dynprocTicStopAll.pfn)();
    }

    return(S_OK);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::DumpMeterLog, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::DumpMeterLog(BSTR bstrFileName)
{
    char ach[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, bstrFileName, -1, ach, sizeof(ach), NULL, NULL);
    DbgExMtLogDump(ach);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetSwitchTimers, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetSwitchTimers(VARIANT * pValue)
{
    TCHAR ach[256];

    ach[0] = 0;
    pValue->vt = VT_BSTR;
    pValue->bstrVal = NULL;

    IOleCommandTarget * pCommandTarget = NULL;

    _pInPlaceObject->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget);

    if (pCommandTarget)
    {
        VARIANT v;
        v.vt = VT_BSTR;
        v.bstrVal = (BSTR)ach;
        pCommandTarget->Exec((GUID *)&CGID_MSHTML, IDM_GETSWITCHTIMERS, 0, NULL, &v);
        pCommandTarget->Release();
    }

    pValue->bstrVal = SysAllocString(ach);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::TestNile
//
//  This is a hack because I can't figure out how else to do this.  -cfranks
//
//---------------------------------------------------------------------------

#if DBG==1

DYNPROC s_dynprocTestNileSTD = { NULL, &g_dynlibMSHTML, "TestNileSTD" };

HRESULT
CPadDoc::TestNile()
{

    if (s_dynprocTestNileSTD.pfn != NULL
        ||  LoadProcedure(&s_dynprocTestNileSTD) == S_OK)
    {
        return ((HRESULT (STDAPICALLTYPE *)())s_dynprocTestNileSTD.pfn)();
    }
    return ERROR_DLL_INIT_FAILED;
}

#endif

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GetObject, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GetObject(BSTR bstrFile, BSTR bstrProgID, IDispatch **ppDisp)
{
    HRESULT hr;
    CLSID clsid;
    IPersistFile *pPF = NULL;
    IUnknown *pUnk = NULL;
    IBindCtx *pBCtx = NULL;
    IMoniker *pMk = NULL;

    *ppDisp = NULL;

    if (bstrFile && !*bstrFile)
        bstrFile = NULL;

    if (bstrProgID && !*bstrProgID)
        bstrProgID = NULL;

    if (bstrProgID)
    {
        hr = THR(CLSIDFromProgID(bstrProgID, &clsid));
        if (hr)
            goto Cleanup;
    }

    if (bstrFile && bstrProgID)
    {
        hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_SERVER,
                IID_IPersistFile, (void **)&pPF));
        if (hr)
            goto Cleanup;

        hr = THR(pPF->Load(bstrFile, 0));
        if (hr)
            goto Cleanup;

        hr = THR(pPF->QueryInterface(IID_IDispatch, (void **)ppDisp));
        if (hr)
            goto Cleanup;
    }
    else if (bstrFile)
    {
        ULONG cEaten;

        hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBCtx, 0));
        if (hr)
            goto Cleanup;

        hr = THR(MkParseDisplayName(pBCtx, bstrFile, &cEaten, &pMk));
        if (hr)
            goto Cleanup;

        hr = THR(pMk->BindToObject(pBCtx, NULL, IID_IDispatch, (void **)ppDisp));
        if (hr)
            goto Cleanup;

    }
    else if (bstrProgID)
    {
        hr = THR(GetActiveObject(clsid, NULL, &pUnk));
        if (hr)
            goto Cleanup;

        hr = THR(pUnk->QueryInterface(IID_IDispatch, (void **)ppDisp));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = E_INVALIDARG;
    }


Cleanup:
    ReleaseInterface(pPF);
    ReleaseInterface(pBCtx);
    ReleaseInterface(pMk);
    ReleaseInterface(pUnk);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CreateObject, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::CreateObject(BSTR bstrProgID, IDispatch **ppDisp)
{
    extern CPadFactory PadDocFactory;
    HRESULT hr;
    CLSID clsid;
    IUnknown *pUnk = NULL;

    if (StrCmpIC(bstrProgID, _T("TridentPad")) == 0)
    {

        // Short circuit our own object so we do not depend
        // on user registering this version of HTMLPad.

        CThreadProcParam tpp(FALSE, ACTION_NONE);
        hr = THR(CreatePadDoc(&tpp, &pUnk));
        if (hr)
            goto Cleanup;

        hr = THR(pUnk->QueryInterface(IID_IDispatch, (void **)ppDisp));
    }
    else
    {
        hr = THR(CLSIDFromProgID(bstrProgID, &clsid));
        if (hr)
            goto Cleanup;

        hr = THR(CoCreateInstance(clsid, NULL, CLSCTX_SERVER,
                     IID_IDispatch, (void **)ppDisp));
    }

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_CurrentTime, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_CurrentTime(long *plTime)
{
    LONGLONG f;
    LONGLONG t;

    QueryPerformanceFrequency((LARGE_INTEGER *)&f);
    QueryPerformanceCounter((LARGE_INTEGER *)&t);

    *plTime = (long)((t * 1000) / f);

    PerfLog1(tagPerfWatchPad, this, "CPadDoc::CurrentTime(*plTime=%ld)", *plTime);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::Show, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ShowWindow(long lCmdShow)
{
    if (_hwnd)
    {
        ::ShowWindow(_hwnd, lCmdShow);
    }

    if (lCmdShow == SW_HIDE && _fVisible)
    {
        _fVisible = FALSE;
        Release();
    }
    else if (lCmdShow != SW_HIDE && !_fVisible)
    {
        _fVisible = TRUE;
        AddRef();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::MoveWindow, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::MoveWindow(long x, long y, long cx, long cy)
{
    if (_hwnd)
    {
        ::MoveWindow(_hwnd, x, y, cx, cy, TRUE);
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::Left/Top/Width/Height, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_WindowLeft(long *px)
{
    RECT rc;

    if (_hwnd)
    {
        GetWindowRect(_hwnd,  &rc);
        *px = rc.left;
    }
    else
    {
        *px = 0;
    }
    return S_OK;
}

HRESULT
CPadDoc::get_WindowTop(long *py)
{
    RECT rc;

    if (_hwnd)
    {
        GetWindowRect(_hwnd,  &rc);
        *py = rc.top;
    }
    else
    {
        *py = 0;
    }
    return S_OK;
}

HRESULT
CPadDoc::get_WindowWidth(long *pcx)
{
    RECT rc;

    if (_hwnd)
    {
        GetWindowRect(_hwnd,  &rc);
        *pcx = rc.right - rc.left;
    }
    else
    {
        *pcx = 0;
    }
    return S_OK;
}

HRESULT
CPadDoc::get_WindowHeight(long *pcy)
{
    RECT rc;

    if (_hwnd)
    {
        GetWindowRect(_hwnd,  &rc);
        *pcy = rc.bottom - rc.top;
    }
    else
    {
        *pcy = 0;
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::Assert, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ASSERT(VARIANT_BOOL fAssert, BSTR bstrMessage)
{
    if (!fAssert)
    {
        char ach[1024];

        // Add name of currently executing script to the assert message.

        if (!_pScriptSite || !_pScriptSite->_achPath[0])
        {
            ach[0] = 0;
        }
        else
        {
            // Try to chop of directory name.

            TCHAR * pchName = wcsrchr(_pScriptSite->_achPath, _T(FILENAME_SEPARATOR));
            if (pchName)
                pchName += 1;
            else
                pchName = _pScriptSite->_achPath;

            WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pchName,
                    -1,
                    ach,
                    MAX_PATH,
                    NULL,
                    NULL);

            strcat(ach, ": ");
        }

        // Add message to the assert.

        if (!bstrMessage || !*bstrMessage)
        {
            strcat(ach, "HTMLPad Script Assert");
        }
        else
        {
            WideCharToMultiByte(
                    CP_ACP,
                    0,
                    bstrMessage,
                    -1,
                    &ach[strlen(ach)],
                    ARRAY_SIZE(ach) - MAX_PATH - 3,
                    NULL,
                    NULL);
        }

        PrintLog(bstrMessage);
        AssertSz(0, ach);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::WaitForRecalc, IPad
//
//  Synopsis: Wait for the document to finish recalcing
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::WaitForRecalc()
{
    IOleCommandTarget * pCommandTarget = NULL;

    _pInPlaceObject->QueryInterface(IID_IOleCommandTarget, (void **)&pCommandTarget);

    if (pCommandTarget)
    {
        pCommandTarget->Exec((GUID *)&CGID_MSHTML, IDM_WAITFORRECALC, 0, NULL, NULL);
        pCommandTarget->Release();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::SetPerfCtl, IPad
//
//  Synopsis: Setup the HtmPerfCtl block
//
//---------------------------------------------------------------------------

HANDLE      g_hMapHtmPerfCtl = NULL;
HTMPERFCTL *g_pHtmPerfCtl = NULL;

void WINAPI PadDocCallback(DWORD dwArg1, void * pvArg2)
{
    ((CPadDoc *)g_pHtmPerfCtl->pvHost)->PerfCtlCallback(dwArg1, pvArg2);
}

void
DeletePerfCtl()
{
    if (g_pHtmPerfCtl)
        Verify(UnmapViewOfFile(g_pHtmPerfCtl));
    if (g_hMapHtmPerfCtl)
        Verify(CloseHandle(g_hMapHtmPerfCtl));
    g_pHtmPerfCtl = NULL;
    g_hMapHtmPerfCtl = NULL;
}

void
CreatePerfCtl(DWORD dwFlags, void * pvHost)
{
    char achName[sizeof(HTMPERFCTL_NAME) + 8 + 1];
    wsprintfA(achName, "%s%08lX", HTMPERFCTL_NAME, GetCurrentProcessId());

    if (g_hMapHtmPerfCtl == NULL)
        g_hMapHtmPerfCtl = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, achName);
    if (g_hMapHtmPerfCtl == NULL)
    {
        TraceTag((tagError, "CreateFileMappingA(\"%s\") failed (%ld)", achName, GetLastError()));
        return;
    }
#ifndef UNIX
    if (g_pHtmPerfCtl == NULL)
        g_pHtmPerfCtl = (HTMPERFCTL *)MapViewOfFile(g_hMapHtmPerfCtl, FILE_MAP_WRITE, 0, 0, 0);
#endif
    if (g_pHtmPerfCtl == NULL)
    {
        TraceTag((tagError, "MapViewOfFile() failed (%ld)", GetLastError()));
        return;
    }

    g_pHtmPerfCtl->dwSize  = sizeof(HTMPERFCTL);
    g_pHtmPerfCtl->dwFlags = dwFlags;
    g_pHtmPerfCtl->pfnCall = PadDocCallback;
    g_pHtmPerfCtl->pvHost  = pvHost;
}

void
CPadDoc::PerfCtlCallback(DWORD dwArg1, void * pvArg2)
{
    if (_fDisablePadEvents)
    {
        EndEvents();
    }
    else
    {
        VARIANT var;

        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = dwArg1;
        FireEvent(DISPID_PadEvents_PerfCtl, 1, &var);
    }
}

HRESULT
CPadDoc::SetPerfCtl(DWORD dwFlags)
{
    CreatePerfCtl(dwFlags, this);
    _fDisablePadEvents = !!(dwFlags & HTMPF_DISABLE_PADEVENTS);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::ClearDownloadCache, IPad
//
//  Synopsis: Flush WININET disk cache
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::ClearDownloadCache()
{
    HANDLE  hEnumCache;
    DWORD   dw;

    struct
    {
        INTERNET_CACHE_ENTRY_INFOA info;
        BYTE ab[MAX_CACHE_ENTRY_INFO_SIZE];
    } info;

    dw = sizeof(info);
    hEnumCache = FindFirstUrlCacheEntryA("*.*", &info.info, &dw);
    if (hEnumCache == 0)
        return S_OK;

    do
    {
        DeleteUrlCacheEntryA(info.info.lpszSourceUrlName);
        dw = sizeof(info);
    } while (FindNextUrlCacheEntryA(hEnumCache, &info.info, &dw));

    FindCloseUrlCache(hEnumCache);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::EnablePainting, IPad
//
//  Synopsis: Enable painting.
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::LockWindowUpdate(VARIANT_BOOL fLock)
{
    _fPaintLocked = fLock != 0;

    if (!fLock && _fPaintOnUnlock && _hwnd)
    {
        RedrawWindow(_hwnd, (GDIRECT *)NULL, NULL, RDW_INVALIDATE|RDW_ALLCHILDREN);
    }

    return S_OK;
}

#ifdef NEVER    // (srinib) - commenting this out, since it is not being used
                // causing maintenance problems and also forces text.lib to be linked
                // to pad.
extern HRESULT Lines(IDispatch * pObject, long *pl);
extern HRESULT Line(IDispatch * pObject, long l, IDispatch **ppLine, CPadDoc * pPadDoc);
extern HRESULT Cascaded(IDispatch * pObject, IDispatch **ppCascaded, CPadDoc * pPadDoc);
#endif

HRESULT CPadDoc::get_Lines(IDispatch * pObject, long *pl)
{
    return 0;
#ifdef NEVER
    return ::Lines(pObject, pl);
#endif
}

HRESULT CPadDoc::get_Line(IDispatch * pObject, long l, IDispatch **ppLine)
{
    return 0;
#ifdef NEVER
    return ::Line(pObject, l, ppLine, this);
#endif
}

HRESULT CPadDoc::get_Cascaded(IDispatch * pObject, IDispatch **ppCascaded)
{
    return 0;
#ifdef NEVER
    return ::Cascaded(pObject, ppCascaded, this);
#endif
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::EnableTraceTag, IPad
//
//---------------------------------------------------------------------------


HRESULT
CPadDoc::EnableTraceTag(BSTR bstrTag, BOOL fEnable)
{
    if (!bstrTag || !_tcslen(bstrTag))
        return S_FALSE;

    int     tag;
    int     ret;
    char    str[256];

    ret = WideCharToMultiByte(CP_ACP, 0, bstrTag, _tcslen(bstrTag), str, 256, NULL, NULL);
    if (!ret)
        return S_FALSE;

    str[ret] = '\0';

    tag = FindTag (str);
    if (tag)
    {
        EnableTag(tag, fEnable);
        return S_OK;
    }

    return S_FALSE;
}

HRESULT
CPadDoc::GetRegValue(long hKeyIn, BSTR bstrSubkey, BSTR bstrValueName, VARIANT *pValue)
{
    HKEY hKey = (HKEY)(LONG_PTR)hKeyIn;
    HKEY hOpenedKey = 0;
    HRESULT hr;
    WCHAR buffer[128];                  // Buffer for RegQueryValueEx
    DWORD type;
    DWORD lType = sizeof(buffer);

    // Default key to HKEY_CURRENT_USER
    if (hKey==0)
    {
        hKey = HKEY_CURRENT_USER;
    }

    (*pValue).vt = VT_BSTR;                  // Initialize answer to empty BSTR
    (*pValue).bstrVal = NULL;

    hr = RegOpenKeyEx(hKey, bstrSubkey, 0, KEY_QUERY_VALUE, &hOpenedKey);
    if (hr)
        goto Cleanup;
    hr = RegQueryValueEx(hOpenedKey, bstrValueName, NULL, &type, (UCHAR *)buffer, &lType);
    if (hr)
        goto Cleanup;

    VariantClear(pValue);
    switch (type)
    {
        case REG_SZ:
            (*pValue).bstrVal = SysAllocString(buffer);
            if (!(*pValue).bstrVal)       // Alloc failed?
                goto Cleanup;
            (*pValue).vt = VT_BSTR;
            break;

        case REG_DWORD:
            (*pValue).vt = VT_I4;
            (*pValue).lVal = *(DWORD*)buffer;
            break;

        default:
            hr = E_NOTIMPL;
            goto Cleanup;
    }

Cleanup:
    if (hOpenedKey)
        (void) RegCloseKey(hOpenedKey);
    return S_OK;
}

HRESULT
CPadDoc::DeleteRegValue(long hkeyIn, BSTR bstrSubKey, BSTR bstrValueName)
{
    HKEY hkey = (HKEY)(LONG_PTR)hkeyIn;
    HKEY hOpenedKey;
    HRESULT hr = S_OK;

    // Default key to HKEY_CURRENT_USER
    if (hkey==0)
    {
        hkey = HKEY_CURRENT_USER;
    }

    hr = RegOpenKeyEx(hkey, bstrSubKey, 0, KEY_SET_VALUE, &hOpenedKey);

    if (ERROR_SUCCESS == hr)
    {
        RegDeleteValue(hOpenedKey, bstrValueName);
        (void)RegCloseKey(hOpenedKey);
    }

    return hr;
}

HRESULT
CPadDoc::SetRegValue(long hkeyIn, BSTR bstrSubKey, BSTR bstrValueName, VARIANT value)
{
    HKEY hkey = (HKEY)(LONG_PTR)hkeyIn;
    HKEY hOpenedKey;
    HRESULT hr = S_OK;
    DWORD dwDisposition;

    // Default key to HKEY_CURRENT_USER
    if (hkey==0)
    {
        hkey = HKEY_CURRENT_USER;
    }

    if (!RegCreateKeyEx(hkey, bstrSubKey, 0, 0, 0, KEY_SET_VALUE, 0,
                            &hOpenedKey, &dwDisposition))
    {
        long lval;
        switch V_VT(&value)
        {
        case VT_I4:
            lval = V_I4(&value);

            if (RegSetValueEx(hOpenedKey, bstrValueName, 0, REG_DWORD, (const BYTE *)(&lval), sizeof(DWORD)))
            {
                hr = E_FAIL;
            }
            break;

        case VT_I2:
            lval = V_I2(&value);

            if (RegSetValueEx(hOpenedKey, bstrValueName, 0, REG_DWORD, (const BYTE *)(&lval), sizeof(DWORD)))
            {
              hr = E_FAIL;
            }
            break;

        case VT_BSTR:
            if (RegSetValueEx(hOpenedKey, bstrValueName, 0, REG_SZ, (const BYTE *)(V_BSTR(&value)), SysStringLen(V_BSTR(&value))*2 + 2))
            {
              hr = E_FAIL;
            }
            break;

        default: hr = E_FAIL;
        }

        Verify(!RegCloseKey(hOpenedKey));

        // Cause MSHMTL.DLL to re-read registry settings
        if (_pInPlaceObject)
        {
            HWND hwndDoc;
            _pInPlaceObject->GetWindow(&hwndDoc);
            (void)SendMessage(hwndDoc, WM_WININICHANGE, 0, 0);
        }

    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT
CPadDoc::TrustProvider(BSTR bstrKeyname, BSTR bstrProvidername, VARIANT *pOldKey)
{
    HRESULT hr;
    VARIANT varProvider;

    VariantInit(pOldKey);            // assume there was no previous key

    VariantInit(&varProvider);          // GetRegValue wants a varaint

    // Get old key value
    hr = GetRegValue(0,
                     _T("Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0"),
                     bstrKeyname,
                     &varProvider);

    // Was there an existing key?
    if (VT_BSTR!=varProvider.vt || NULL==varProvider.bstrVal)
    {
        varProvider.vt = VT_BSTR;
        varProvider.bstrVal = SysAllocString(bstrProvidername);

        // No, then we set one
        hr = SetRegValue(0,
                         _T("Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0"),
                         bstrKeyname,
                         varProvider);

        // and return a copy of the keyname to indicate we set it
        // (this will be used by RevertTrustProvider to delete it).
        pOldKey->vt = VT_BSTR;
        pOldKey->bstrVal = SysAllocString(bstrKeyname);
    }

    VariantClear(&varProvider);

    return hr;
}

HRESULT
CPadDoc::RevertTrustProvider(BSTR keyname)
{
    HRESULT hr = S_OK;

    if (keyname && *keyname)
    {
        hr = DeleteRegValue(0,
                            _T("Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust\\Trust Providers\\Software Publishing\\Trust Database\\0"),
                            keyname);
    }

    return hr;
}

HRESULT GetPadLineTypeInfo(CPadDoc * pPadDoc, ITypeInfo ** pptinfo)
{
    HRESULT hr;

    hr = THR(pPadDoc->LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    *pptinfo = pPadDoc->_pTypeInfoILine;

Cleanup:
    RRETURN(hr);
}

HRESULT GetPadCascadedTypeInfo(CPadDoc * pPadDoc, ITypeInfo ** pptinfo)
{
    HRESULT hr;

    hr = THR(pPadDoc->LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    *pptinfo = pPadDoc->_pTypeInfoICascaded;

Cleanup:
    RRETURN(hr);
}

HRESULT
CPadDoc::ComputeCRC(BSTR bstrText, VARIANT * pCRC)
{
    const TCHAR* pch = bstrText;
    WORD         wHash = 0;

    if (pch)
    {
        for (;*pch; pch++)
        {
            wHash = wHash << 7 ^ wHash >> (16-7) ^ (*pch);
        }
    }

    V_VT(pCRC) = VT_I2;
    V_I2(pCRC) = wHash;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_Dbg, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_Dbg(long *plDbg)
{
#ifdef DBG
    *plDbg = DBG;
#else
    *plDbg = 0;
#endif
    RRETURN(S_OK);
}


//---------------------------------------------------------------------------
//
//  Member:     CPadDoc::get_ProcessorArchitecture, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_ProcessorArchitecture(BSTR * pbstrMachineType)
{
    SYSTEM_INFO si;
    TCHAR *     pch;

    GetSystemInfo(&si);

    switch (si.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        pch = _T("x86");
        break;
    case PROCESSOR_ARCHITECTURE_MIPS:
        pch = _T("mips");
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        pch = _T("alpha");
        break;
    case PROCESSOR_ARCHITECTURE_PPC:
        pch = _T("ppc");
        break;
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
        pch = _T("Unknown");
        break;
    }

    *pbstrMachineType = SysAllocString(pch);

    RRETURN(*pbstrMachineType ? S_OK : E_OUTOFMEMORY);
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::CoMemoryTrackDisable, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::CoMemoryTrackDisable(VARIANT_BOOL fDisable)
{
    DbgCoMemoryTrackDisable(fDisable);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_UseShdocvw, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_UseShdocvw(VARIANT_BOOL *pfHosted)
{
    *pfHosted = !!_fUseShdocvw;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::put_UseShdocvw, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::put_UseShdocvw(VARIANT_BOOL fHosted)
{
    _fUseShdocvw = fHosted;
    SendMessage(_hwndToolbar, TB_CHECKBUTTON, (WPARAM)IDM_PAD_USESHDOCVW, (LPARAM)MAKELONG(_fUseShdocvw, 0));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::get_DownloadNotifyMask, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::get_DownloadNotifyMask(ULONG *pulMask)
{
    *pulMask = _ulDownloadNotifyMask;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::put_DownloadNotifyMask, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::put_DownloadNotifyMask(ULONG ulMask)
{
    _ulDownloadNotifyMask = ulMask;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GoBack, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GoBack(VARIANT_BOOL *pfWentBack)
{
    HRESULT hr;
    *pfWentBack = FALSE;

    if (_pBrowser)
    {
        hr = THR(_pBrowser->GoBack());
        *pfWentBack = (hr == S_OK);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::GoForward, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::GoForward(VARIANT_BOOL *pfWentForward)
{
    HRESULT hr;
    *pfWentForward = FALSE;

    if (_pBrowser)
    {
        hr = THR(_pBrowser->GoForward());
        *pfWentForward = (hr == S_OK);
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::TestExternal, IPad
//
//---------------------------------------------------------------------------

typedef HRESULT (TestExternal_Func)(VARIANT *pParam, long *plRetVal);

HRESULT
CPadDoc::TestExternal(
    BSTR bstrDLLName,
    BSTR bstrFunctionName,
    VARIANT *pParam,
    long *plRetVal)
{
    HRESULT hr = S_OK;
    HINSTANCE hInstDLL = NULL;
    TestExternal_Func *pfn = NULL;

    if (!plRetVal || !bstrDLLName || !bstrFunctionName)
        return E_POINTER;

    *plRetVal = -1;

    int cchLen = SysStringLen(bstrDLLName);
    char * pchBuf = new char[cchLen+1];

    if (pchBuf)
    {
        WideCharToMultiByte(CP_ACP, 0, bstrDLLName, cchLen, pchBuf, cchLen+1, NULL, NULL);
        pchBuf[cchLen] = '\0';

        hInstDLL = LoadLibraryA(pchBuf);

        delete pchBuf;
    }

    if (NULL == hInstDLL)
    {
        TraceTagEx((tagScriptLog, TAG_NONAME, "TestExternal can't load external dll"));
        return S_FALSE;     // Can't return error codes or the script will abort
    }

    cchLen = SysStringLen(bstrFunctionName);
    pchBuf = new char[cchLen+1];

    if (pchBuf)
    {
        WideCharToMultiByte(CP_ACP, 0, bstrFunctionName, cchLen, pchBuf, cchLen+1, NULL, NULL);
        pchBuf[cchLen] = '\0';

        pfn = (TestExternal_Func *)GetProcAddress(hInstDLL, pchBuf);

        delete pchBuf;
    }

    if (NULL == pfn)
    {
        TraceTagEx((tagScriptLog, TAG_NONAME, "TestExternal can't find external function"));
        hr = S_FALSE;
    }
    else
    {
        *plRetVal = 0;
        hr = (*pfn)(pParam, plRetVal);
    }

    FreeLibrary(hInstDLL);

    return hr;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::UnLoadDLL, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::UnLoadDLL()
{
    HRESULT hr;

    hr = THR(CloseFile());
    if (hr)
        goto Cleanup;

    UnregisterLocalCLSIDs();
    CoFreeUnusedLibraries();

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member: CPadDoc::DeinitDynamicLibrary, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::DeinitDynamicLibrary(BSTR bstrDLLName)
{
    static DYNPROC s_dynprocDeinitDynamicLibrary = { NULL, &g_dynlibMSHTML, "DeinitDynamicLibrary" };

    if (    s_dynprocDeinitDynamicLibrary.pfn != NULL
        ||  LoadProcedure(&s_dynprocDeinitDynamicLibrary) == S_OK)
    {
        ((void (STDAPICALLTYPE *)(LPCTSTR))s_dynprocDeinitDynamicLibrary.pfn)(bstrDLLName);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::IsDynamicLibraryLoaded, IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::IsDynamicLibraryLoaded(BSTR bstrDLLName, VARIANT_BOOL *pfLoaded)
{
    static DYNPROC s_dynprocIsDynamicLibraryLoaded = { NULL, &g_dynlibMSHTML, "IsDynamicLibraryLoaded" };
    if (    s_dynprocIsDynamicLibraryLoaded.pfn != NULL
        ||  LoadProcedure(&s_dynprocIsDynamicLibraryLoaded) == S_OK)
    {
        *pfLoaded = ((BOOL (STDAPICALLTYPE *)(LPCTSTR))s_dynprocIsDynamicLibraryLoaded.pfn)(bstrDLLName) ?
            VB_TRUE : VB_FALSE;
    }

    return S_OK;
}

HRESULT CPadDoc::get_ViewChangesFired(long *plCount)
{
    *plCount = _lViewChangesFired;
    _lViewChangesFired = 0; // reset count
    return S_OK;
}

HRESULT CPadDoc::get_DataChangesFired(long *plCount)
{
    *plCount = _lDataChangesFired;
    _lDataChangesFired = 0; // reset count
    return S_OK;
}



class CPadServerOM : public IDispatch
{
public:
    CPadServerOM(DWORD dwUA, TCHAR *pchUA, TCHAR *pchFile, TCHAR *pchQS);

    DECLARE_FORMS_STANDARD_IUNKNOWN(CPadServerOM)
    STDMETHOD(GetTypeInfo)(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
        { return E_NOTIMPL; }
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo)
        { return E_NOTIMPL; }
    STDMETHOD(GetIDsOfNames)(REFIID riid,
                             LPOLESTR * rgszNames,
                             UINT cNames,
                             LCID lcid,
                             DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember,
                      REFIID riid,
                      LCID lcid,
                      WORD wFlags,
                      DISPPARAMS * pdispparams,
                      VARIANT * pvarResult,
                      EXCEPINFO * pexcepinfo,
                      UINT * puArgErr);

    //
    // Data
    //

    CStr    _cstrUA;
    CStr    _cstrFile;
    CStr    _cstrQS;
    DWORD   _dwUA;
};


CPadServerOM::CPadServerOM(DWORD dwUA, TCHAR *pchUA, TCHAR *pchFile, TCHAR *pchQS)
{
    _dwUA = dwUA;
    _cstrUA.Set(pchUA);
    _cstrFile.Set(pchFile);
    _cstrQS.Set(pchQS);
    _ulRefs = 0;
}


HRESULT
CPadServerOM::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown || riid == IID_IDispatch)
    {
        *ppv = (IDispatch *)this;
        AddRef();
        return S_OK;
    }

    return E_NOTIMPL;
}


HRESULT
CPadServerOM::GetIDsOfNames(
    REFIID riid,
    LPOLESTR * rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID * rgdispid)
{
    DISPID  dispid = DISPID_UNKNOWN;

    if (_tcsequal(*rgszNames, _T("UserAgent")))
    {
        dispid = 1;
    }
    else if (_tcsequal(*rgszNames, _T("NormalizedUA")))
    {
        dispid = 2;
    }
    else if (_tcsequal(*rgszNames, _T("URL")))
    {
        dispid = 3;
    }
    else if (_tcsequal(*rgszNames, _T("Path")))
    {
        dispid = 4;
    }
    else if (_tcsequal(*rgszNames, _T("QueryString")))
    {
        dispid = 5;
    }

    *rgdispid = dispid;
    return dispid == DISPID_UNKNOWN ? DISP_E_UNKNOWNNAME : S_OK;
}


HRESULT
CPadServerOM::Invoke(
    DISPID dispidMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS * pdispparams,
    VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo,
    UINT * puArgErr)
{
    if (!pvarResult)
        return S_OK;

    switch (dispidMember)
    {
    case 1:
        V_VT(pvarResult) = VT_BSTR;
        _cstrUA.AllocBSTR(&V_BSTR(pvarResult));
        break;

    case 2:
        V_VT(pvarResult) = VT_I4;
        V_I4(pvarResult) = _dwUA;
        break;

    case 3:
    case 4:
        V_VT(pvarResult) = VT_BSTR;
        _cstrFile.AllocBSTR(&V_BSTR(pvarResult));
        break;

    case 5:
        if (_cstrQS)
        {
            V_VT(pvarResult) = VT_BSTR;
            _cstrQS.AllocBSTR(&V_BSTR(pvarResult));
        }
        else
        {
            V_VT(pvarResult) = VT_NULL;
        }
        break;

    default:
        return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::Sleep, per IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::Sleep (int nTimeout)
{
    ::Sleep (nTimeout);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadDoc::IsWin95, per IPad
//
//---------------------------------------------------------------------------

HRESULT
CPadDoc::IsWin95(long * pfWin95)
{
    OSVERSIONINFOA ovi;

    if (!pfWin95)
        return E_POINTER;

    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&ovi);

    *pfWin95 = (ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Member: CPadDoc::GetAccWindow, per IPad
//
//----------------------------------------------------------------------------
HRESULT
CPadDoc::GetAccWindow( IDispatch ** ppAccWindow )
{
    HWND                hwnd = NULL;
    HRESULT             hr = S_OK;
    HINSTANCE           hInstOleacc = 0;
    IDispatch *         pHTMLDoc = NULL;
    IServiceProvider *  pServProv = NULL;

    if ( !ppAccWindow )
        return E_POINTER;

    *ppAccWindow = NULL;

    if (_pInPlaceActiveObject )
            _pInPlaceActiveObject->GetWindow(&hwnd);

    if ( !hwnd && _pInPlaceObject )
            _pInPlaceObject->GetWindow(&hwnd);

    hInstOleacc = LoadLibraryA( "OLEACC.DLL" );

    if ( hwnd && hInstOleacc )
    {
        // get the document
        hr = get_Document( &pHTMLDoc );
        if ( hr || !pHTMLDoc )
            goto Cleanup;

        // get the service provider interface
        hr = pHTMLDoc->QueryInterface( IID_IServiceProvider, (void **)&pServProv);
        if ( hr || !pServProv )
            goto Cleanup;


        // get the IAccessible interface
        hr = pServProv->QueryService( IID_IAccessible, IID_IAccessible, (void **)ppAccWindow);
        if ( hr || !ppAccWindow )
            goto Cleanup;

    }


//FerhanE:
//  Even if we can not find the OLEACC.DLL, return NULL as the out value and a success code.
//  This way, the script can skip over the code that requires this information.

Cleanup:
    if ( hInstOleacc)
        FreeLibrary( hInstOleacc );

    if ( pHTMLDoc )
        pHTMLDoc->Release();

    if ( pServProv )
        pServProv->Release();

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member: CPadDoc::GetAccObjAtPoint, per IPad
//
//----------------------------------------------------------------------------
HRESULT
CPadDoc::GetAccObjAtPoint( long x, long y, IDispatch ** ppAccObject )
{
    if ( !ppAccObject )
        return E_POINTER;

    *ppAccObject = NULL;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member : SetKeyboard
//
//  Synopsis: Change the keyboard layout for IME testing.
//
//---------------------------------------------------------------------------

HRESULT
   CPadDoc::SetKeyboard( BSTR bstrKeyboard)
{
    if (bstrKeyboard)
    {
        HKL hkblyt;
        char szBufA[KL_NAMELENGTH];
        WideCharToMultiByte(CP_ACP, 0,
                            bstrKeyboard, SysStringLen(bstrKeyboard),
                            szBufA, sizeof(szBufA), NULL, NULL);
        szBufA[KL_NAMELENGTH-1] = (char)0;
        hkblyt = LoadKeyboardLayoutA(szBufA, 0);
        if (hkblyt)
        {
#if 1
            hkblyt = ActivateKeyboardLayout(hkblyt, KLF_SETFORPROCESS);
            if (hkblyt)
                return (S_OK);
#else
            PostMessage(_hwnd, WM_INPUTLANGCHANGEREQUEST,
                        (WPARAM)0, (LPARAM)hkblyt );
            return (S_OK);
#endif
        }
    }

    return E_POINTER;
}

//---------------------------------------------------------------------------
//
//  Member : GetKeyboard
//
//  Synopsis: Change the keyboard layout for IME testing.
//
//---------------------------------------------------------------------------

HRESULT
   CPadDoc::GetKeyboard( VARIANT *pKeyboard)
{
    if (pKeyboard)
    {
        char szBufA[KL_NAMELENGTH];
        if (GetKeyboardLayoutNameA(szBufA))
        {
            TCHAR szBufW[KL_NAMELENGTH];
            MultiByteToWideChar(CP_ACP, 0, szBufA, sizeof(szBufA),
                                szBufW, sizeof(szBufW)/sizeof(TCHAR));
            szBufW[KL_NAMELENGTH-1] = (char)0;
            V_VT(pKeyboard) = VT_BSTR;
            V_BSTR(pKeyboard) = SysAllocString(szBufW);
            if (V_BSTR(pKeyboard))
                return (S_OK);
        }
    }

    return E_POINTER;
}
//---------------------------------------------------------------------------
//
//  Member : ToggleIMEMode
//
//  Synopsis: Change the IME mode.
//
//---------------------------------------------------------------------------

HRESULT
   CPadDoc::ToggleIMEMode( BSTR bstrIME)
{
    if (StrCmpIC(bstrIME, _T("Japanese")) == 0)
    {
        keybd_event(VK_MENU, 0, 0, 0);
        keybd_event(0x29, 0x29, 0, KF_ALTDOWN << 16);
        keybd_event(0x29, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
        return S_OK;
    }
    else if (StrCmpIC(bstrIME, _T("Chinese")) == 0)
    {
        keybd_event(VK_SHIFT, 0, 0, 0);
        keybd_event(VK_SPACE, 0, 0, 0);
        keybd_event(VK_SPACE, 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
        return S_OK;
    }
    else if (StrCmpIC(bstrIME, _T("Korean")) == 0)
    {
        // The VK_RMENU keycode is not supported by keybd_event().
        // 0x15 is a magic, reserved for kanji, code that I discovered
        // using Spy. -- AryeG.
        keybd_event(0x15, 0, 0, 0);
        keybd_event(0x15, 0, KEYEVENTF_KEYUP, 0);
        return S_OK;
    }


    return E_POINTER;
}
//---------------------------------------------------------------------------
//
//  Member : SendIMEKeys
//
//  Synopsis: Send keyboard events from the given string
//            so that the IME will be able to trap them.
//
//---------------------------------------------------------------------------

// For just one key at a time.
TCHAR *
   CPadDoc::SendIMEKey(TCHAR *pch, DWORD dwFlags)
{
    BYTE ch;
    switch (*pch)
    {
        default:
            ch = (BYTE)*pch;
            if (ch >= 'a' && ch <= 'z')
                ch -= 'a' - 'A';
            keybd_event(ch, (BYTE)MapVirtualKey(ch, 0), 0, 0);
            keybd_event(ch, (BYTE)MapVirtualKey(ch, 0), KEYEVENTF_KEYUP, 0);
            pch++;
            break;

        case _T('+'):
            keybd_event(VK_SHIFT, 0, 0, 0);
            pch++;
            pch = SendIMEKey(pch, dwFlags);
            keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
            break;

        case _T('^'):
            keybd_event(VK_CONTROL, 0, 0, 0);
            pch++;
            pch = SendIMEKey(pch, dwFlags);
            keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
            break;

        case _T('%'):
            keybd_event(VK_MENU, 0, 0, 0);
            pch++;
            pch = SendIMEKey(pch, dwFlags | (KF_ALTDOWN << 16));
            keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            break;

        case _T('{'):
            pch++;
            pch = SendIMESpecial(pch, 0);
            break;
    }
    return pch;
}
HRESULT
   CPadDoc::SendIMEKeys( BSTR bstrKeys )
{
    if (bstrKeys)
    {
        TCHAR *pch = bstrKeys;

        // Put in all the keystrokes.
        while (*pch != 0)
        {
            pch = SendIMEKey(pch, 0);
        }
        return (S_OK);
    }

    return E_POINTER;
}

//----------------------------------------------------------------------------
//
//  Member: CPadDoc::RemoveElement
//
//----------------------------------------------------------------------------
HRESULT
CPadDoc::RemoveElement ( IDispatch * pIDispatch )
{
    HRESULT            hr = S_OK;
    IDispatch *        pDocDisp = NULL;
    IMarkupServices *  pMarkupServices = NULL;
    IHTMLElement *     pIElement = NULL;

    if (!pIDispatch)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR( pIDispatch->QueryInterface( IID_IHTMLElement, (void **) & pIElement ) );

    if (hr)
        goto Cleanup;

    hr = THR( get_Document( & pDocDisp ) );

    if (hr)
        goto Cleanup;

    hr = THR( pDocDisp->QueryInterface( IID_IMarkupServices, (void **) & pMarkupServices ) );

    if (hr)
        goto Cleanup;

    hr = THR( pMarkupServices->RemoveElement( pIElement ) );

    if (hr)
        goto Cleanup;

Cleanup:

    if (pDocDisp)
        pDocDisp->Release();

    if (pIElement)
        pIElement->Release();

    if (pMarkupServices)
        pMarkupServices->Release();

    RRETURN( hr );
}

//----------------------------------------------------------------------------
//
//  Member: CPadDoc::Markup
//
//----------------------------------------------------------------------------

static IMarkupServices *  g_MS = NULL;
static IMarkupContainer * g_PM = NULL;

static HRESULT
FindElementByTagId ( ELEMENT_TAG_ID tagIdFindThis, IHTMLElement * * ppIElement )
{
    HRESULT          hr = S_OK;
    IMarkupPointer * pIPointer = NULL;

    Assert( ppIElement );

    *ppIElement = NULL;

    hr = THR( g_MS->CreateMarkupPointer( & pIPointer ) );

    if (hr)
        goto Cleanup;

    hr = THR( pIPointer->MoveToContainer( g_PM, TRUE ) );

    if (hr)
        goto Cleanup;

    for ( ; ; )
    {
        MARKUP_CONTEXT_TYPE ct;

        hr = THR( pIPointer->Right( TRUE, & ct, ppIElement, NULL, NULL ) );

        if (ct == CONTEXT_TYPE_None)
            break;

        if (ct == CONTEXT_TYPE_EnterScope)
        {
            ELEMENT_TAG_ID tagId;

            hr = THR( g_MS->GetElementTagId( *ppIElement, & tagId ) );

            if (hr)
                goto Cleanup;

            if (tagId == tagIdFindThis)
                break;

        }
        if (*ppIElement)
            (*ppIElement)->Release();
    }

Cleanup:

    if (pIPointer)
        pIPointer->Release();

    RRETURN( hr );
}

static HRESULT
MangleRemoveBody ( )
{
    HRESULT        hr = S_OK;
    IHTMLElement * pIElement = NULL;

    hr = THR( FindElementByTagId( TAGID_BODY, & pIElement ) );

    if (hr)
        goto Cleanup;

    if (!pIElement)
        goto Cleanup;

    hr = THR( g_MS->RemoveElement( pIElement ) );

    if (hr)
        goto Cleanup;

Cleanup:

    if (pIElement)
        pIElement->Release();

    RRETURN( hr );
}

static HRESULT
MangleRemoveTables ( )
{
    HRESULT        hr = S_OK;
    IHTMLElement * pIElement = NULL;

    for ( ; ; )
    {
        hr = THR( FindElementByTagId( TAGID_TABLE, & pIElement ) );

        if (hr)
            goto Cleanup;

        if (!pIElement)
            goto Cleanup;

        hr = THR( g_MS->RemoveElement( pIElement ) );

        if (hr)
            goto Cleanup;

        pIElement->Release();
        pIElement = NULL;
    }

Cleanup:

    if (pIElement)
        pIElement->Release();

    RRETURN( hr );
}

IMarkupPointer *
CPadDoc::FindPadPointer ( long id )
{
    for ( int i = 0 ; i < _aryPadPointers.Size() ; i++ )
        if (_aryPadPointers[i]._id == id)
            return _aryPadPointers[i]._pPointer;

    return NULL;
}

IMarkupPointer *
CPadDoc::FindPadPointer ( VARIANT * pvar )
{
    if (!pvar || V_VT( pvar ) != VT_I4)
        return NULL;

    return FindPadPointer( V_I4( pvar ) );
}

IMarkupContainer *
CPadDoc::FindPadContainer ( long id )
{
    for ( int i = 0 ; i < _aryPadContainers.Size() ; i++ )
        if (_aryPadContainers[i]._id == id)
            return _aryPadContainers[i]._pContainer;

    return NULL;
}

IMarkupContainer *
CPadDoc::FindPadContainer ( VARIANT * pvar )
{
    if (!pvar || V_VT( pvar ) != VT_I4)
        return NULL;

    return FindPadContainer( V_I4( pvar ) );
}

IHTMLElement *
CPadDoc::GetElement ( VARIANT * pvar )
{
    IHTMLElement * pIElement;

    if (!pvar || V_VT( pvar ) != VT_DISPATCH || !V_DISPATCH( pvar ))
        return NULL;

    if (V_DISPATCH( pvar )->QueryInterface( IID_IHTMLElement, (void **) & pIElement ) != S_OK)
        return NULL;

    pIElement->Release(); // I don't really want a ref (the dispatch will keep it around)

    return pIElement;
}

HRESULT
CPadDoc::Markup (
    VARIANT * pvarParam1, VARIANT * pvarParam2, VARIANT * pvarParam3,
    VARIANT * pvarParam4, VARIANT * pvarParam5, VARIANT * pvarParam6,
    VARIANT * pvarRet )
{
    HRESULT            hr = S_OK;
    IDispatch *        pDocDisp = NULL;

    VariantClear( pvarRet );

    hr = THR( get_Document( & pDocDisp ) );

    if (hr)
        goto Cleanup;

    Assert( ! g_MS && ! g_PM );

    hr = THR( pDocDisp->QueryInterface( IID_IMarkupServices, (void **) & g_MS ) );

    if (hr)
        goto Cleanup;

    hr = THR( pDocDisp->QueryInterface( IID_IMarkupContainer, (void **) & g_PM ) );

    if (hr)
        goto Cleanup;

    if (!pvarParam1 || V_VT( pvarParam1 ) != VT_BSTR)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "RemoveElement" )))  // element
    {
        if (!pvarParam2)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (GetElement( pvarParam2 ))
        {
            hr = THR( RemoveElement( V_DISPATCH( pvarParam2 ) ) );

            if (hr)
                goto Cleanup;
        }
        else if (V_VT( pvarParam2 ) == VT_BSTR)
        {
            ELEMENT_TAG_ID tagId;
            IHTMLElement * pIElement = NULL;

            hr = THR( g_MS->GetTagIDForName ( V_BSTR( pvarParam2 ), & tagId ) );

            if (hr)
                goto Cleanup;

            hr = THR( FindElementByTagId( tagId, & pIElement ) );

            if (hr)
                goto Cleanup;

            if (pIElement)
            {
                hr = THR( RemoveElement( pIElement ) );

                if (hr)
                    goto Cleanup;
                
                pIElement->Release();
            }

        }
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "TagIdToString" )))  // str/element
    {
        if (!pvarParam2 || V_VT( pvarParam2 ) != VT_I4)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->GetNameForTagID( ELEMENT_TAG_ID( V_I4( pvarParam2 ) ), & V_BSTR( pvarRet ) ) );

        if (hr)
            goto Cleanup;
        
        pvarRet->vt = VT_BSTR;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "GetTagId" )))  // str/element
    {
        ELEMENT_TAG_ID tagID = TAGID_NULL;
                
        if (!pvarParam2)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (GetElement( pvarParam2 ))
        {
            hr = THR( g_MS->GetElementTagId( GetElement( pvarParam2 ), & tagID ) );

            if (hr)
                goto Cleanup;
        }
        else if (V_VT( pvarParam2 ) == VT_BSTR)
        {
            hr = THR( g_MS->GetTagIDForName ( V_BSTR( pvarParam2 ), & tagID ) );

            if (hr)
                goto Cleanup;
        }

        pvarRet->vt = VT_I4;
        pvarRet->lVal = tagID;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "InsertElement" )))  // pStart, pFin, element
    {
        IMarkupPointer * pIPointerStart = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerFinish = FindPadPointer( pvarParam3 );
        IHTMLElement *   pIElement = GetElement( pvarParam4 );

        if (!pIPointerStart || !pIElement)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->InsertElement( pIElement, pIPointerStart, pIPointerFinish ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Remove" )))  // pStart, pFin
    {
        IMarkupPointer * pIPointerStart = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerFinish = FindPadPointer( pvarParam3 );

        if (!pIPointerStart || !pIPointerFinish)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->Remove( pIPointerStart, pIPointerFinish ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Move" )))  // pStart, pFin, pTarget
    {
        IMarkupPointer * pIPointerStart = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerFinish = FindPadPointer( pvarParam3 );
        IMarkupPointer * pIPointerTarget = FindPadPointer( pvarParam4 );

        if (!pIPointerStart || !pIPointerFinish || !pIPointerTarget)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->Move( pIPointerStart, pIPointerFinish, pIPointerTarget ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Copy" )))  // pStart, pFin, pTarget
    {
        IMarkupPointer * pIPointerStart = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerFinish = FindPadPointer( pvarParam3 );
        IMarkupPointer * pIPointerTarget = FindPadPointer( pvarParam4 );

        if (!pIPointerStart || !pIPointerFinish || !pIPointerTarget)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->Copy( pIPointerStart, pIPointerFinish, pIPointerTarget ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "FindText" )))  // start, text, end match, end search
    {
        IMarkupPointer * pIPointerStartSearch = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerEndMatch = FindPadPointer( pvarParam4 );
        IMarkupPointer * pIPointerEndSearch = FindPadPointer( pvarParam5 );

        if (!pIPointerStartSearch || !pIPointerStartSearch || !pIPointerEndSearch)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = pIPointerStartSearch->FindText( V_BSTR( pvarParam3 ), 0, pIPointerEndMatch, pIPointerEndSearch );

        pvarRet->vt = VT_I4;
        pvarRet->lVal = hr = S_FALSE ? 0 : 1;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "InsertText" )))  // pointer, text
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );

        if (!pIPointer || !pvarParam3|| V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR(
            g_MS->InsertText(
                V_BSTR( pvarParam3 ),
                FormsStringLen( V_BSTR( pvarParam3 ) ),
                pIPointer ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "RemoveBody" )))  // no args
    {
        hr = THR( MangleRemoveBody() );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "RemoveTables" )))   // no args
    {
        hr = THR( MangleRemoveTables() );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "DumpTree" )))  // pointer [optional]
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );
        IEditDebugServices * pEditDebug;

        hr = THR( pDocDisp->QueryInterface( IID_IEditDebugServices, (void **) & pEditDebug ) );

        if (hr)
            goto Cleanup;

        pEditDebug->DumpTree( pIPointer );

        pEditDebug->Release();
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "CloneElement" )))  // in element, ret clone
    {
        IHTMLElement * pIElement = GetElement( pvarParam2 );
        IHTMLElement * pIElementClone;

        if (!pIElement)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->CloneElement( pIElement, & pIElementClone ) );

        if (hr)
            goto Cleanup;
        
        pvarRet->vt = VT_DISPATCH;
        pvarRet->pdispVal = pIElementClone;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "CreateMarkupPointer" )))  // return pointer
    {
        IMarkupPointer * pIPointer;
        PadPointerData   padPointer;

        hr = THR( g_MS->CreateMarkupPointer( & pIPointer ) );

        if (hr)
            goto Cleanup;

        padPointer._pPointer = pIPointer;
        padPointer._id = _idPadIDNext++;

        _aryPadPointers.AppendIndirect( & padPointer, NULL );

        pvarRet->vt = VT_I4;
        pvarRet->lVal = padPointer._id;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "CreateMarkupContainer" )))  // return pointer
    {
        IMarkupContainer * pIContainer;
        PadContainerData   padContainer;

        hr = THR( g_MS->CreateMarkupContainer( & pIContainer ) );

        if (hr)
            goto Cleanup;

        padContainer._pContainer = pIContainer;
        padContainer._id = _idPadIDNext++;

        _aryPadContainers.AppendIndirect( & padContainer, NULL );

        pvarRet->vt = VT_I4;
        pvarRet->lVal = padContainer._id;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "ReleasePointer" )))  // pointer
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );

        if (!pIPointer)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        pIPointer->Release();

        for ( int i = 0 ; i < _aryPadPointers.Size() ; i++ )
            if (_aryPadPointers[i]._id == V_I4( pvarParam2 ))
                _aryPadPointers.Delete( i );
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "ReleaseContainer" )))  // pointer
    {
        IMarkupContainer * pIContainer = FindPadContainer( pvarParam2 );

        if (!pIContainer)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        pIContainer->Release();

        for ( int i = 0 ; i < _aryPadContainers.Size() ; i++ )
            if (_aryPadContainers[i]._id == V_I4( pvarParam2 ))
                _aryPadContainers.Delete( i );
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "SetGravity" )))  // pStrGravity (left|right)
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );
        POINTER_GRAVITY  eGravity;

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Left" )))
            eGravity = POINTER_GRAVITY_Left;
        else if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Right" )))
            eGravity = POINTER_GRAVITY_Right;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->SetGravity( eGravity ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "SetCling" )))  // bool
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BOOL)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->SetCling( V_BOOL( pvarParam3 ) ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Unposition" )))  // no args
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );

        if (!pIPointer)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->Unposition() );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "MoveToPointer" )))  // p1, p2
    {
        IMarkupPointer * pIPointerMoveMe = FindPadPointer( pvarParam2 );
        IMarkupPointer * pIPointerToHere = FindPadPointer( pvarParam3 );

        if (!pIPointerMoveMe || !pIPointerToHere)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointerMoveMe->MoveToPointer( pIPointerToHere ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "MoveToBeginning" )))  // pointer, container [ optional ]
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );
        IMarkupContainer * pIContainer = FindPadContainer( pvarParam3 );

        if (!pIPointer)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->MoveToContainer( pIContainer ? pIContainer : g_PM, TRUE ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "MoveToEnd" )))  // pointer, container [ optional ]
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );
        IMarkupContainer * pIContainer = FindPadContainer( pvarParam3 );

        if (!pIPointer)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->MoveToContainer( pIContainer ? pIContainer : g_PM, FALSE ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "MovePointer" )))  // pointer, dir, cchMax, ret actual cch
    {
        IMarkupPointer * pIPointer = FindPadPointer( pvarParam2 );
        long             cch;
        BOOL             fLeft;

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Left" )))
            fLeft = TRUE;
        else if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Right" )))
            fLeft = FALSE;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!pvarParam4 || V_VT( pvarParam4 ) == VT_ERROR)
            cch = -1;
        else if (V_VT( pvarParam4 ) != VT_I4)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        else
            cch = V_I4( pvarParam4 );

        if (fLeft)
            hr = THR( pIPointer->Left( TRUE, NULL, NULL, & cch, NULL ) );
        else
            hr = THR( pIPointer->Right( TRUE, NULL, NULL, & cch, NULL ) );

        pvarRet->vt = VT_I4;
        pvarRet->lVal = cch;

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Context" )))  // pointer, dir, ret context (bstr)
    {
        IMarkupPointer *    pIPointer = FindPadPointer( pvarParam2 );
        BOOL                fLeft;
        MARKUP_CONTEXT_TYPE ct;

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Left" )))
            fLeft = TRUE;
        else if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Right" )))
            fLeft = FALSE;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (fLeft)
            hr = THR( pIPointer->Left( FALSE, & ct, NULL, NULL, NULL ) );
        else
            hr = THR( pIPointer->Right( FALSE, & ct, NULL, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        pvarRet->vt = VT_BSTR;

        switch ( ct )
        {
        case CONTEXT_TYPE_None        : hr = THR( FormsAllocStringW( _T( "None" ),       & pvarRet->bstrVal ) ); break;
        case CONTEXT_TYPE_EnterScope  : hr = THR( FormsAllocStringW( _T( "EnterScope" ), & pvarRet->bstrVal ) ); break;
        case CONTEXT_TYPE_ExitScope   : hr = THR( FormsAllocStringW( _T( "ExitScope" ),  & pvarRet->bstrVal ) ); break;
        case CONTEXT_TYPE_NoScope     : hr = THR( FormsAllocStringW( _T( "NoScope" ),    & pvarRet->bstrVal ) ); break;
        case CONTEXT_TYPE_Text        : hr = THR( FormsAllocStringW( _T( "Text" ),       & pvarRet->bstrVal ) ); break;
        }

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "ContextElement" )))  // pointer, dir, ret context (elem)
    {
        IMarkupPointer *    pIPointer = FindPadPointer( pvarParam2 );
        BOOL                fLeft;
        MARKUP_CONTEXT_TYPE ct;
        IHTMLElement *      pIElement = NULL;

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Left" )))
            fLeft = TRUE;
        else if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Right" )))
            fLeft = FALSE;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (fLeft)
            hr = THR( pIPointer->Left( FALSE, & ct, & pIElement, NULL, NULL ) );
        else
            hr = THR( pIPointer->Right( FALSE, & ct, & pIElement, NULL, NULL ) );

        if (hr)
            goto Cleanup;

        pvarRet->vt = VT_DISPATCH;

        if (ct == CONTEXT_TYPE_EnterScope || ct == CONTEXT_TYPE_ExitScope || ct == CONTEXT_TYPE_NoScope)
        {
            hr = THR( pIElement->QueryInterface( IID_IDispatch, (void **) & pvarRet->pdispVal ) );

            if (hr)
                goto Cleanup;
        }
        else
        {
            pvarRet->pdispVal = NULL;
        }

        if (pIElement)
            pIElement->Release();

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "ContextText" )))  // pointer, dir, cchMax, ret text (bstr)
    {
        IMarkupPointer *    pIPointer = FindPadPointer( pvarParam2 );
        BOOL                fLeft;
        MARKUP_CONTEXT_TYPE ct;
        long                cch;

        if (!pIPointer || !pvarParam3 || V_VT( pvarParam3 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Left" )))
            fLeft = TRUE;
        else if (!StrCmpIC( V_BSTR( pvarParam3 ), _T( "Right" )))
            fLeft = FALSE;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!pvarParam4 || V_VT( pvarParam4 ) == VT_ERROR)
            cch = -1;
        else if (V_VT( pvarParam4 ) != VT_I4)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        else
            cch = V_I4( pvarParam4 );

        if (fLeft)
            hr = THR( pIPointer->Left( FALSE, & ct, NULL, & cch, NULL ) );
        else
            hr = THR( pIPointer->Right( FALSE, & ct, NULL, & cch, NULL ) );

        if (hr)
            goto Cleanup;

        pvarRet->vt = VT_BSTR;

        if (ct == CONTEXT_TYPE_Text)
        {
            TCHAR * pch = new TCHAR [ cch + 1 ];

            if (!pch)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            if (fLeft)
                hr = THR( pIPointer->Left( FALSE, NULL, NULL, & cch, pch ) );
            else
                hr = THR( pIPointer->Right( FALSE, NULL, NULL, & cch, pch ) );

            pch[cch] = 0;

            hr = THR( FormsAllocStringW( pch, & pvarRet->bstrVal ) );

            delete pch;
        }
        else
        {
            hr = THR( FormsAllocStringW( _T( "" ), & pvarRet->bstrVal ) );

            if (hr)
                goto Cleanup;
        }

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "MoveAdjacentToElement" )))  // pointer, element, adj
    {
        IMarkupPointer *  pIPointer = FindPadPointer( pvarParam2 );
        IHTMLElement *    pIElement = GetElement( pvarParam3 );

        ELEMENT_ADJACENCY eAdj;

        if (!pIPointer || !pIElement)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!pvarParam4 || V_VT( pvarParam4 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "BeforeBegin" )))
            eAdj = ELEM_ADJ_BeforeBegin;
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "AfterBegin" )))
            eAdj = ELEM_ADJ_AfterBegin;
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "BeforeEnd" )))
            eAdj = ELEM_ADJ_BeforeEnd;
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "AfterEnd" )))
            eAdj = ELEM_ADJ_AfterEnd;
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( pIPointer->MoveAdjacentToElement( pIElement, eAdj ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "Compare" )))  // p1, p2, how, return t/f
    {
        IMarkupPointer *  p1 = FindPadPointer( pvarParam2 );
        IMarkupPointer *  p2 = FindPadPointer( pvarParam3 );
        BOOL              fResult;

        if (!p1 || !p2 || !pvarParam4 || V_VT( pvarParam4 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "IsEqualTo" )))
            hr = THR( p1->IsEqualTo( p2, & fResult ) );
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "IsLeftOf" )))
            hr = THR( p1->IsLeftOf( p2, & fResult ) );
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "IsLeftOfOrEqualTo" )))
            hr = THR( p1->IsLeftOfOrEqualTo( p2, & fResult ) );
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "IsRightOf" )))
            hr = THR( p1->IsRightOf( p2, & fResult ) );
        else if (!StrCmpIC( V_BSTR( pvarParam4 ), _T( "IsRightOfOrEqualTo" )))
            hr = THR( p1->IsRightOfOrEqualTo( p2, & fResult ) );
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        pvarRet->vt = VT_BOOL;
        pvarRet->boolVal = !!fResult;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "BeginUndoUnit" )))  // undo string
    {
        if (!pvarParam2 || V_VT( pvarParam2 ) != VT_BSTR)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR( g_MS->BeginUndoUnit( V_BSTR( pvarParam2 ) ) );

        if (hr)
            goto Cleanup;
    }
    else if (!StrCmpIC( V_BSTR( pvarParam1 ), _T( "EndUndoUnit" )))
    {
        hr = THR( g_MS->EndUndoUnit() );

        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:

    if (g_PM)
    {
        g_PM->Release();
        g_PM = NULL;
    }

    if (pDocDisp)
        pDocDisp->Release();

    if (g_MS)
    {
        g_MS->Release();
        g_MS = NULL;
    }

    RRETURN( hr );
}



// #define SELF_TEST        // define if you want self-test
// #define HIGH_RESOLUTION  // define if 63 bits resolution desired

// If your system doesn't have a rotate function for 32 bits integers,
// then define it thus:
// uint32 _lrotr (uint32 x, int r) {
//   return (x >> r) | (x << (sizeof(x)-r));}

// define parameters
#define KK 17
#define JJ 10
#define R1 13
#define R2  5

typedef unsigned long uint32;       // use 32 bit unsigned integers
#ifdef HIGH_RESOLUTION
typedef long double trfloat;        // define floating point precision
#else
typedef double trfloat;             // define floating point precision
#endif

class TRanrotGenerator {            // encapsulate random number generator
  public:
  void RandomInit(uint32 seed);     // initialization
  void SetInterval(int min, int max); // set interval for iRandom
  int iRandom();                    // get integer random number
  trfloat Random();                 // get floating point random number
  TRanrotGenerator(uint32 seed=-1); // constructor
  protected:
  void step();                      // generate next random number
  union {                           // used for conversion to float
    trfloat randp1;
    uint32 randbits[3];};
  int p1, p2;                       // indexes into buffer
  int imin, iinterval;              // interval for iRandom
  uint32 randbuffer[KK][2];         // history buffer
#ifdef SELF_TEST
  uint32 randbufcopy[KK*2][2];      // used for self-test
#endif
};


TRanrotGenerator::TRanrotGenerator(uint32 seed) {
  // constructor
  RandomInit(seed);  SetInterval(0, 0xfffffff);}


void TRanrotGenerator::SetInterval(int min, int max) {
  // set interval for iRandom
  imin = min; iinterval = max - min + 1;}


void TRanrotGenerator::step() {
  // generate next random number
  uint32 a, b;
  // generate next number
  b = _lrotr(randbuffer[p1][0], R1) + randbuffer[p2][0];
  a = _lrotr(randbuffer[p1][1], R2) + randbuffer[p2][1];
  randbuffer[p1][0] = a; randbuffer[p1][1] = b;
  // rotate list pointers
  if (--p1 < 0) p1 = KK - 1;
  if (--p2 < 0) p2 = KK - 1;
#ifdef SELF_TEST
  // perform self-test
  if (randbuffer[p1][0] == randbufcopy[0][0] &&
    memcmp(randbuffer, randbufcopy[KK-p1], 2*KK*sizeof(uint32)) == 0) {
      // self-test failed
      if ((p2 + KK - p1) % KK != JJ) {
        // note: the way of printing error messages depends on system
        printf("Random number generator not initialized");}
      else {
        printf("Random number generator returned to initial state");}
      abort();}
#endif
  // convert to float
  randbits[0] = a;
#ifdef HIGH_RESOLUTION
  randbits[1] = b | 0x80000000;                // 80 bits floats = 63 bits resolution
#else
  randbits[1] = (b & 0x000FFFFF) | 0x3FF00000; // 64 bits floats = 52 bits resolution
#endif
  }


trfloat TRanrotGenerator::Random() {
  // returns a random number between 0 and 1.
  trfloat r = randp1 - 1.;
  step();
  return r;}


int TRanrotGenerator::iRandom() {
  // get integer random number
  int i = iinterval * Random();
  if (i >= iinterval) i = iinterval;
  return imin + i;}


void TRanrotGenerator::RandomInit (uint32 seed) {
  // this function initializes the random number generator.
  int i, j;
  // make sure seed != 0
  if (seed==0) seed = 0x7fffffff;

  // make random numbers and put them into the buffer
  for (i=0; i<KK; i++) {
    for (j=0; j<2; j++) {
      seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
      randbuffer[i][j] = seed;}}
  // set exponent of randp1
  randp1 = 1.5;
#ifdef HIGH_RESOLUTION
  assert((randbits[2]&0xFFFF)==0x3FFF); // check that Intel 10-byte float format used
#else
  Assert(randbits[1]==0x3FF80000); // check that IEEE double precision float format used
#endif
  Assert(sizeof(uint32)==4);  // check that 32 bits integers used

  // initialize pointers to circular buffer
  p1 = 0;  p2 = JJ;
#ifdef SELF_TEST
  memcpy (randbufcopy, randbuffer, 2*KK*sizeof(uint32));
  memcpy (randbufcopy[KK], randbuffer, 2*KK*sizeof(uint32));
#endif
  // randomize some more
  for (i=0; i<97; i++) step();
}

static TRanrotGenerator r;

HRESULT
CPadDoc::Random ( long range, long * plRet )
{
    *plRet = range > 0 ? r.iRandom() % range : 0;

    return S_OK;
}

HRESULT
CPadDoc::RandomSeed ( long seed )
{
    r.RandomInit( seed );

    return S_OK;
}

void
GetHeapTotals(LONG * pcHeapBlocks, LONG * pcHeapBytes)
{
    HANDLE rgHeaps[256];
    DWORD dwHeap, dwHeaps;
    PROCESS_HEAP_ENTRY he;
    LONG cBlocks = 0, cBytes = 0;

    dwHeaps = GetProcessHeaps(ARRAY_SIZE(rgHeaps), rgHeaps);

    for (dwHeap = 0; dwHeap < dwHeaps; ++dwHeap)
    {
        HeapLock(rgHeaps[dwHeap]);

        memset(&he, 0, sizeof(PROCESS_HEAP_ENTRY));

        while (HeapWalk(rgHeaps[dwHeap], &he))
        {
            if (he.wFlags & PROCESS_HEAP_ENTRY_BUSY)
            {
                cBlocks += 1;
                cBytes += he.cbData;
            }
        }

        HeapUnlock(rgHeaps[dwHeap]);
    }

    *pcHeapBlocks = cBlocks;
    *pcHeapBytes = cBytes;
}

HRESULT
CPadDoc::GetHeapCounter(long iCounter, long * plRet)
{
    static long g_cHeapBlocks = 0;
    static long g_cHeapBytes  = 0;

    *plRet = 0;

    if (iCounter == 0)
    {
        GetHeapTotals(&g_cHeapBlocks, &g_cHeapBytes);
    }

    if (iCounter == 0)
        *plRet = g_cHeapBytes;
    else if (iCounter == 1)
        *plRet = g_cHeapBlocks;
    else
        *plRet = 0;

    return S_OK;
}

HRESULT
CPadDoc::CreateProcess(BSTR bstrCommandLine, VARIANT_BOOL fWait)
{
    char achCommandLine[2048];
    int cch, ret;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };

    cch = WideCharToMultiByte(CP_ACP, 0, bstrCommandLine, _tcslen(bstrCommandLine), achCommandLine, 2048, NULL, NULL);
    if (!cch)
        return E_FAIL;

    achCommandLine[cch] = '\0';
 
    si.cb = sizeof(STARTUPINFOA);

    ret = CreateProcessA(NULL, achCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    if (!ret)
        return E_FAIL;

    CloseHandle(pi.hThread);

    if (fWait)
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    CloseHandle(pi.hProcess);

    return S_OK;
}

HRESULT
CPadDoc::GetCurrentProcessId(long * plRetVal)
{
    *plRetVal = ::GetCurrentProcessId();
    return S_OK;
}
