//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padssite.hxx
//
//  Contents:   CPadScriptSite
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

EXTERN_C const GUID CGID_ScriptSite = {0x3050f3f1, 0x98b5, 0x11cf, {0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b}};
#define CMDID_SCRIPTSITE_URL            0
#define CMDID_SCRIPTSITE_HTMLDLGTRUST   1
#define CMDID_SCRIPTSITE_SECSTATE       2
#define CMDID_SCRIPTSITE_SID            3

#undef ASSERT

class CConnectionPoint : public IConnectionPoint
{
public:

    CConnectionPoint(CPadScriptSite *pSite);
    ~CConnectionPoint();

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    STDMETHOD(GetConnectionInterface)(IID * pIID);
    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
    STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(LPENUMCONNECTIONS * ppEnum);

    CPadScriptSite *_pSite;
    ULONG    _ulRefs;
};

CConnectionPoint::CConnectionPoint(CPadScriptSite *pSite)
{
    _ulRefs = 1;
    _pSite = pSite;
    _pSite->AddRef();
}

CConnectionPoint::~CConnectionPoint()
{
    _pSite->Release();
}

ULONG
CConnectionPoint::AddRef()
{
    return _ulRefs += 1;
}

ULONG
CConnectionPoint::Release()
{
    if (--_ulRefs == 0)
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

HRESULT
CConnectionPoint::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IConnectionPoint)
    {
        *ppv = (IConnectionPoint *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CConnectionPoint::GetConnectionInterface(IID * pIID)
{
    *pIID = DIID_PadEvents;
    return S_OK;
}

HRESULT
CConnectionPoint::GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
{
    *ppCPC = _pSite;
    (*ppCPC)->AddRef();
    return S_OK;
}

HRESULT
CConnectionPoint::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    *pdwCookie = 0;

    ClearInterface(&_pSite->_pDispSink);
    RRETURN(THR(pUnkSink->QueryInterface(IID_IDispatch, (void **)&_pSite->_pDispSink)));
}

HRESULT
CConnectionPoint::Unadvise(DWORD dwCookie)
{
    ClearInterface(&_pSite->_pDispSink);
    return S_OK;
}

HRESULT
CConnectionPoint::EnumConnections(LPENUMCONNECTIONS * ppEnum)
{
    *ppEnum = NULL;
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadScriptSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IActiveScriptSite ||
        iid == IID_IUnknown)
    {
        *ppv = (IActiveScriptSite *) this;
    }
    else if (iid == IID_IPad || iid == IID_IDispatch)
    {
        *ppv = (IPad *)this;
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *ppv = (IConnectionPointContainer *)this;
    }
    else if (iid == IID_IProvideClassInfo ||
            iid == IID_IProvideClassInfo2 ||
            iid == IID_IProvideMultipleClassInfo)
    {
        *ppv = (IProvideMultipleClassInfo *)this;
    }
    else if (iid == IID_IActiveScriptSiteWindow)
    {
        *ppv = (IActiveScriptSiteWindow *) this;
    }
    else if (iid == IID_IOleCommandTarget)
    {
        *ppv = (IOleCommandTarget *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite constructor
//
//---------------------------------------------------------------------------

CPadScriptSite::CPadScriptSite(CPadDoc * pDoc)
{
    _pDoc = pDoc;
    _ulRefs = 1;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite destructor
//
//---------------------------------------------------------------------------

CPadScriptSite::~CPadScriptSite()
{
    VariantClear(&_varParam);
    ClearInterface(&_pDispSink);
    Assert(_ulRefs <= 1);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::AddRef, Release
//
//---------------------------------------------------------------------------

ULONG
CPadScriptSite::AddRef()
{
    _ulRefs += 1;
    return _ulRefs;
}

ULONG
CPadScriptSite::Release()
{
    if (--_ulRefs == 0)
    {
        delete this;
        return 0;
    }

    return _ulRefs;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::Init
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::Init(TCHAR *pchType)
{
    HRESULT hr;
    IActiveScriptParse *pParse = NULL;
    static const CLSID CLSID_VBS = { 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8 };
    static const CLSID CLSID_JSCRIPT = { 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58};
    BSTR    bstrName = NULL;
    BOOL    fJscript = FALSE;

    // CoCreate and connect to Script engine

    if (pchType)
    {
        fJscript = (StrCmpIC(pchType, _T(".js")) == 0);
    }

    hr = THR(CoCreateInstance(
            pchType && fJscript ? CLSID_JSCRIPT : CLSID_VBS,
            NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, (void **)&_pScript));
    if (hr)
        goto Cleanup;

    hr = THR(_pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pParse));
    if (hr)
        goto Cleanup;

    hr = THR(_pScript->SetScriptSite(this));
    if (hr)
        goto Cleanup;

    hr = THR(pParse->InitNew());
    if (hr)
        goto Cleanup;

    hr = THR(_pScript->AddNamedItem(_T("Pad"), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS));
    if (hr)
        goto Cleanup;

    if (fJscript)
    {
        //
        // For JavaScript we have to manually hook up known event handlers.
        // All four of these methods _must_ exist in the .js file.
        //

        // Add any pad events that we know people use.
        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadLoad()"),
                     _T("Pad"),
                     NULL,
                     _T("Load"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;

        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadDocLoaded(fLoaded)"),
                     _T("Pad"),
                     NULL,
                     _T("DocLoaded(fLoaded)"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;

        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadStatus(Status)"),
                     _T("Pad"),
                     NULL,
                     _T("Status(Status)"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;

        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadTimer()"),
                     _T("Pad"),
                     NULL,
                     _T("Timer"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;

        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadUnload()"),
                     _T("Pad"),
                     NULL,
                     _T("Unload"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;

        hr = THR(pParse->AddScriptlet(
                     NULL,
                     _T("PadPerfCtl(dwArg)"),
                     _T("Pad"),
                     NULL,
                     _T("PerfCtl(dwArg)"),
                     _T("\""),
                     0,
                     0,
                     0,
                     &bstrName,
                     NULL));
        FormsFreeString(bstrName);
        bstrName = NULL;
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pParse);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::Close
//
//---------------------------------------------------------------------------

void
CPadScriptSite::Close()
{
    if (_pScript)
    {
        IGNORE_HR(_pScript->Close());
        ClearInterface(&_pScript);
    }
}


//---------------------------------------------------------------------------
//
//  Member:     CPadScriptSite::GetClassInfo, IProvideClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetClassInfo(ITypeInfo **ppTypeInfo)
{
    HRESULT hr;

    hr = _pDoc->LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    *ppTypeInfo = _pDoc->_pTypeInfoCPad;
    (*ppTypeInfo)->AddRef();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CPadScriptSite::GetGUID, IProvideClassInfo2
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetGUID(DWORD dwGuidKind, GUID * pGUID)
{
    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        *pGUID = DIID_PadEvents;
    }
    else
    {
        return E_NOTIMPL;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CPadScriptSite::GetMultiTypeInfoCount, IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------


HRESULT
CPadScriptSite::GetMultiTypeInfoCount(ULONG *pc)
{
    *pc = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:     CPadScriptSite::GetInfoOfIndex, IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetInfoOfIndex(
    ULONG       itinfo,
    DWORD       dwFlags,
    ITypeInfo** pptinfoCoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    Assert(itinfo == 0);

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        *pptinfoCoClass = _pDoc->_pTypeInfoCPad;
        (*pptinfoCoClass)->AddRef();
        if (pdwTIFlags)
            *pdwTIFlags = 0;
    }

    if (dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS)
    {
        *pcdispidReserved = 100;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDPRIMARY)
    {
        *piidPrimary = IID_IPad;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDSOURCE)
    {
        *piidSource = DIID_PadEvents;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadScriptSite::EnumConnectionPoints, IConnectionPointContainer
//
//---------------------------------------------------------------------------
HRESULT
CPadScriptSite::EnumConnectionPoints(LPENUMCONNECTIONPOINTS *)
{
    // I hate this interface. This method is stupid.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CPadScriptSite::FindConnectionPoint, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT* ppCpOut)
{
    HRESULT hr;

    if (iid == DIID_PadEvents || iid == IID_IDispatch)
    {
        *ppCpOut = new CConnectionPoint(this);
        hr = *ppCpOut ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::GetLCID, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetLCID(LCID *plcid)
{
  return E_NOTIMPL;     // Use system settings
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::GetItemInfo, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetItemInfo(
      LPCOLESTR   pstrName,
      DWORD       dwReturnMask,
      IUnknown**  ppunkItemOut,
      ITypeInfo** pptinfoOut)
{
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        if (!pptinfoOut)
            return E_INVALIDARG;
        *pptinfoOut = NULL;
    }

    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        if (!ppunkItemOut)
            return E_INVALIDARG;
        *ppunkItemOut = NULL;
    }

    if (!StrCmpIC(_T("Pad"), pstrName))
    {
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
        {
            *pptinfoOut = PadDoc()->_pTypeInfoCPad;
            (*pptinfoOut)->AddRef();
        }
        if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
        {
            *ppunkItemOut = (IPad *)this;
            (*ppunkItemOut)->AddRef();
        }
        return S_OK;
    }


    return TYPE_E_ELEMENTNOTFOUND;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::GetDocVersionString, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::GetDocVersionString(BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::RequestItems()
{
    return _pScript->AddNamedItem(_T("Pad"), SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::RequestTypeLibs, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::RequestTypeLibs()
{
    return _pScript->AddTypeLib(LIBID_Pad, 1, 0, 0);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::OnScriptTerminate, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::OnScriptTerminate(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    // UNDONE: Put up error dlg here
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::OnStateChange, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::OnStateChange(SCRIPTSTATE ssScriptState)
{
    // Don't care about notification
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::OnScriptError, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::OnScriptError(IActiveScriptError *pse)
{
    BSTR        bstrLine = NULL;
    BSTR        bstr;
    TCHAR *     pchDescription;
    TCHAR       achDescription[256];
    TCHAR *     pchMessage = NULL;
    EXCEPINFO   ei;
    DWORD       dwSrcContext;
    ULONG       ulLine;
    LONG        ichError;
    HRESULT     hr;

    hr = THR(pse->GetExceptionInfo(&ei));
    if (hr)
        goto Cleanup;

    hr = THR(pse->GetSourcePosition(&dwSrcContext, &ulLine, &ichError));
    if (hr)
        goto Cleanup;

    hr = THR(pse->GetSourceLineText(&bstrLine));
    if (hr)
        hr = S_OK;  // Ignore this error, there may not be source available

    if (ei.bstrDescription)
    {
        pchDescription = ei.bstrDescription;
    }
    else
    {
        achDescription[0] = 0;
        FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                ei.scode,
                LANG_SYSTEM_DEFAULT,
                achDescription,
                ARRAY_SIZE(achDescription),
                NULL);
        pchDescription = achDescription;
    }

    hr = THR(Format(FMT_OUT_ALLOC, &pchMessage, 0,
        _T("File: <0s>\r\n")
        _T("Line: <1d>\r\n")
        _T("Char: <2d>\r\n")
        _T("Text: <3s>\r\n")
        _T("Scode: <4x>\r\n")
        _T("Source: <5s>\r\n")
        _T("Description: <6s>\r\n"),
        _achPath,
        (long)(ulLine + 1),
        (long)(ichError),
        bstrLine ? bstrLine : _T(""),
        ei.scode,
        ei.bstrSource ? ei.bstrSource : _T(""),
        pchDescription));
    if (hr)
        goto Cleanup;

    bstr = SysAllocString(pchMessage);

    // PrintLog needs a BSTR
    _pDoc->PrintLog(bstr);

    SysFreeString(bstr);

    if (!IsTagEnabled(tagAssertExit))
    {
        MSGBOXPARAMS mbp;
        memset(&mbp, 0, sizeof(mbp));
        mbp.cbSize = sizeof(mbp);
        mbp.hwndOwner = PadDoc()->_hwnd;
        mbp.lpszText = pchMessage;
        mbp.lpszCaption = _T("Error");
        mbp.dwStyle = MB_APPLMODAL | MB_ICONERROR | MB_OK;

        MessageBoxIndirect(&mbp);
    }
    else
    {
        AssertSz(FALSE, "Script Error Occurred. (See logfile)");
    }

Cleanup:
    if(ei.bstrSource)
        SysFreeString(ei.bstrSource);
    if(ei.bstrDescription)
        SysFreeString(ei.bstrDescription);
    if(ei.bstrHelpFile)
        SysFreeString(ei.bstrHelpFile);
    MemFree(pchMessage);
    if (bstrLine)
        SysFreeString(bstrLine);
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::OnEnterScript, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT CPadScriptSite::OnEnterScript()
{
    // No need to do anything
    return S_OK;
}


//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::OnLeaveScript, IActiveScriptSite
//
//---------------------------------------------------------------------------

HRESULT CPadScriptSite::OnLeaveScript()
{
    // No need to do anything
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::, IActiveScriptSiteWindow
//
//---------------------------------------------------------------------------

HRESULT CPadScriptSite::GetWindow(HWND *phwndOut)
{
    *phwndOut = PadDoc()->_hwnd;
    return S_OK;
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::EnableModeless, IActiveScriptSiteWindow
//
//---------------------------------------------------------------------------

HRESULT CPadScriptSite::EnableModeless(BOOL fEnable)
{
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CPadScriptSite::ExecuteScriptFile
//
//  Load and execute script file
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::ExecuteScriptFile(TCHAR *pchPath)
{
    HRESULT     hr;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    DWORD       cchFile;
    DWORD       cbRead;
    char *      pchBuf = 0;
    TCHAR *     pchBufWide = 0;
    TCHAR *     pchFile;

    GetFullPathName(pchPath, ARRAY_SIZE(_achPath), _achPath, &pchFile);

    // Load script file

    hFile = CreateFile(pchPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    cchFile = GetFileSize(hFile, NULL);
    if (cchFile == 0xFFFFFFFF)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    pchBuf = new char[cchFile + 1];
    pchBufWide = new TCHAR[cchFile + 1];
    if (!pchBuf || !pchBufWide)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!ReadFile(hFile, pchBuf, cchFile, &cbRead, 0))
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }
    pchBuf[cbRead] = 0;

    MultiByteToWideChar(CP_ACP, 0, pchBuf, -1, pchBufWide, cchFile + 1);

    // Execute script

    _pDoc->SetStatusText(NULL);

    hr = ExecuteScriptStr(pchBufWide);
    if(hr)
        goto Cleanup;

    _pDoc->SetStatusText(NULL);

Cleanup:
    delete pchBuf;
    delete pchBufWide;
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::ExecuteScriptStr
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::ExecuteScriptStr(TCHAR * pchScript)
{
    HRESULT hr;
    IActiveScriptParse * pParse = NULL;

    hr = THR(_pScript->QueryInterface(IID_IActiveScriptParse, (void **)&pParse));
    if (hr)
        goto Cleanup;

    hr = THR(pParse->ParseScriptText(pchScript, _T("Pad"), NULL, NULL, 0, 0, 0L, NULL, NULL));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pParse);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::SetScriptState
//
//---------------------------------------------------------------------------

HRESULT
CPadScriptSite::SetScriptState(SCRIPTSTATE ss)
{
    return _pScript->SetScriptState(ss);
}

//---------------------------------------------------------------------------
//
// Method:  CPadScriptSite::xxx, IPad
//
//          The implementation of IPad passed to the script engine
//          cannot be the same as that of the CPadDoc because this
//          causes a reference count loop with the script engine.
//
//---------------------------------------------------------------------------

HRESULT CPadScriptSite::GetTypeInfoCount(UINT FAR* pctinfo)
            { return PadDoc()->GetTypeInfoCount(pctinfo); }
HRESULT CPadScriptSite::GetTypeInfo(
  UINT itinfo,
  LCID lcid,
  ITypeInfo FAR* FAR* pptinfo)
            { return PadDoc()->GetTypeInfo(itinfo, lcid, pptinfo); }
HRESULT CPadScriptSite::GetIDsOfNames(
  REFIID riid,
  OLECHAR FAR* FAR* rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID FAR* rgdispid)
            { return PadDoc()->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
HRESULT CPadScriptSite::Invoke(
  DISPID dispidMember,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS FAR* pdispparams,
  VARIANT FAR* pvarResult,
  EXCEPINFO FAR* pexcepinfo,
  UINT FAR* puArgErr)
            { return PadDoc()->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }
HRESULT CPadScriptSite::DoEvents(VARIANT_BOOL Wait)
            { return PadDoc()->DoEvents(Wait); }
HRESULT CPadScriptSite::EndEvents()
            { return PadDoc()->EndEvents(); }
HRESULT CPadScriptSite::ClearDownloadCache()
            { return PadDoc()->ClearDownloadCache(); }
HRESULT CPadScriptSite::WaitForRecalc()
            { return PadDoc()->WaitForRecalc(); }
HRESULT CPadScriptSite::SetPerfCtl(DWORD dwFlags)
            { return PadDoc()->SetPerfCtl(dwFlags); }
HRESULT CPadScriptSite::LockWindowUpdate(VARIANT_BOOL fLock)
            { return PadDoc()->LockWindowUpdate(fLock); }
HRESULT CPadScriptSite::LockKeyState(VARIANT_BOOL fShift, VARIANT_BOOL fControl, VARIANT_BOOL fAlt)
            { return PadDoc()->LockKeyState(fShift, fControl, fAlt); }
HRESULT CPadScriptSite::UnlockKeyState()
            { return PadDoc()->UnlockKeyState(); }
HRESULT CPadScriptSite::SendKeys(BSTR Keys, VARIANT_BOOL Wait)
            { return PadDoc()->SendKeys(Keys, Wait); }
HRESULT CPadScriptSite::OpenFile(BSTR Path, BSTR ProgID)
            { return PadDoc()->OpenFile(Path, ProgID); }
HRESULT CPadScriptSite::SaveFile(BSTR Path)
            { return PadDoc()->SaveFile(Path); }
HRESULT CPadScriptSite::CloseFile()
            { return PadDoc()->CloseFile(); }
HRESULT CPadScriptSite::ExecuteCommand(long CmdID, VARIANT * Data)
            { return PadDoc()->ExecuteCommand(CmdID, Data); }
HRESULT CPadScriptSite::QueryCommandStatus(long CmdID, VARIANT * Status)
            { return PadDoc()->QueryCommandStatus(CmdID, Status); }
HRESULT CPadScriptSite::ExecuteScript(BSTR Path, VARIANT *ScriptParam, VARIANT_BOOL Async)
            { return PadDoc()->ExecuteScript(Path, ScriptParam, Async); }
HRESULT CPadScriptSite::RegisterControl(BSTR Path)
            { return PadDoc()->RegisterControl(Path); }
HRESULT CPadScriptSite::IncludeScript(BSTR Path)
            { return PadDoc()->IncludeScript(Path); }
HRESULT CPadScriptSite::SetProperty(IDispatch * pDisp, BSTR bstrProp, VARIANT * pVar)
            { return PadDoc()->SetProperty(pDisp, bstrProp, pVar); }
HRESULT CPadScriptSite::GetProperty(IDispatch * pDisp, BSTR bstrProp, VARIANT * pVar)
            { return PadDoc()->GetProperty(pDisp, bstrProp, pVar); }
HRESULT CPadScriptSite::get_ScriptPath(long Level, BSTR * Path)
            { return PadDoc()->get_ScriptPath(Level, Path); }
HRESULT CPadScriptSite::get_ProcessorArchitecture(BSTR * MachineType)
            { return PadDoc()->get_ProcessorArchitecture(MachineType); }
HRESULT CPadScriptSite::get_ScriptParam(VARIANT *ScriptParam)
            { return PadDoc()->get_ScriptParam(ScriptParam); }
HRESULT CPadScriptSite::get_ScriptObject(IDispatch **ScriptObject)
            { return PadDoc()->get_ScriptObject(ScriptObject); }
HRESULT CPadScriptSite::get_CurrentTime(long * Time)
            { return PadDoc()->get_CurrentTime(Time); }
HRESULT CPadScriptSite::get_Document(IDispatch * * Document)
            { return PadDoc()->get_Document(Document); }
HRESULT CPadScriptSite::get_TempPath(BSTR * Path)
            { return PadDoc()->get_TempPath(Path); }
HRESULT CPadScriptSite::GetTempFileName(BSTR * Name)
            { return PadDoc()->GetTempFileName(Name); }
HRESULT CPadScriptSite::PrintStatus(BSTR Message)
            { return PadDoc()->PrintStatus(Message); }
HRESULT CPadScriptSite::PrintLog(BSTR Line)
            { return PadDoc()->PrintLog(Line); }
HRESULT CPadScriptSite::PrintDebug(BSTR Line)
            { return PadDoc()->PrintDebug(Line); }
HRESULT CPadScriptSite::CreateObject(BSTR ProgID, IDispatch **ppDisp)
            { return PadDoc()->CreateObject(ProgID, ppDisp); }
HRESULT CPadScriptSite::GetObject(BSTR FileName, BSTR ProgID, IDispatch **ppDisp)
            { return PadDoc()->GetObject(FileName, ProgID, ppDisp); }
HRESULT CPadScriptSite::CompareFiles(BSTR File1, BSTR File2, VARIANT_BOOL * FilesMatch)
            { return PadDoc()->CompareFiles(File1, File2, FilesMatch); }
HRESULT CPadScriptSite::CopyThisFile(BSTR File1, BSTR File2, VARIANT_BOOL * Success)
            { return PadDoc()->CopyThisFile(File1, File2, Success); }
HRESULT CPadScriptSite::DRTPrint(long Flags, VARIANT_BOOL * Success)
            { return PadDoc()->DRTPrint(Flags, Success); }
HRESULT CPadScriptSite::SetDefaultPrinter(BSTR bstrNewDefaultPrinter, VARIANT_BOOL * Success)
            { return PadDoc()->SetDefaultPrinter(bstrNewDefaultPrinter, Success); }
HRESULT CPadScriptSite::FileExists(BSTR File, VARIANT_BOOL * pfFileExists)
            { return PadDoc()->FileExists(File, pfFileExists); }
HRESULT CPadScriptSite::get_TimerInterval(long * Interval)
            { return PadDoc()->get_TimerInterval(Interval); }
HRESULT CPadScriptSite::put_TimerInterval(long Interval)
            { return PadDoc()->put_TimerInterval(Interval); }
HRESULT CPadScriptSite::DisableDialogs()
            { return PadDoc()->DisableDialogs(); }
HRESULT CPadScriptSite::ShowWindow(long lCmdShow)
            { return PadDoc()->ShowWindow(lCmdShow); }
HRESULT CPadScriptSite::MoveWindow(long x,long y, long cx, long cy)
            { return PadDoc()->MoveWindow(x, y, cx, cy); }
HRESULT CPadScriptSite::get_WindowLeft(long *x)
            { return PadDoc()->get_WindowLeft(x); }
HRESULT CPadScriptSite::get_WindowTop(long *y)
            { return PadDoc()->get_WindowTop(y); }
HRESULT CPadScriptSite::get_WindowWidth(long *cx)
            { return PadDoc()->get_WindowWidth(cx); }
HRESULT CPadScriptSite::get_WindowHeight(long *cy)
            { return PadDoc()->get_WindowHeight(cy); }
HRESULT CPadScriptSite::get_DialogsEnabled(VARIANT_BOOL *Enabled)
            { return PadDoc()->get_DialogsEnabled(Enabled); }
HRESULT CPadScriptSite::StartCAP()
            { return PadDoc()->StartCAP(); }
HRESULT CPadScriptSite::StopCAP()
            { return PadDoc()->StopCAP(); }
HRESULT CPadScriptSite::SuspendCAP()
            { return PadDoc()->SuspendCAP(); }
HRESULT CPadScriptSite::ResumeCAP()
            { return PadDoc()->ResumeCAP(); }
HRESULT CPadScriptSite::TicStartAll()
            { return PadDoc()->TicStartAll(); }
HRESULT CPadScriptSite::TicStopAll()
            { return PadDoc()->TicStopAll(); }
HRESULT CPadScriptSite::ASSERT(VARIANT_BOOL Assertion, BSTR LogMsg)
            { return PadDoc()->ASSERT(Assertion, LogMsg); }
HRESULT CPadScriptSite::get_Lines(IDispatch * pObject, long *pl)
            { return PadDoc()->get_Lines(pObject, pl); }
HRESULT CPadScriptSite::get_Line(IDispatch * pObject, long l, IDispatch **ppLine)
            { return PadDoc()->get_Line(pObject, l, ppLine); }
HRESULT CPadScriptSite::get_Cascaded(IDispatch * pObject, IDispatch **ppCascaded)
            { return PadDoc()->get_Cascaded(pObject, ppCascaded); }
HRESULT CPadScriptSite::EnableTraceTag(BSTR bstrTag, BOOL fEnable)
            { return PadDoc()->EnableTraceTag(bstrTag, fEnable); }
HRESULT CPadScriptSite::get_Dbg(long * plDbg)
            { return PadDoc()->get_Dbg(plDbg); }
HRESULT CPadScriptSite::CleanupTempFiles()
            { return PadDoc()->CleanupTempFiles(); }
HRESULT CPadScriptSite::WsClear()
            { return PadDoc()->WsClear(); }
HRESULT CPadScriptSite::WsTakeSnapshot()
            { return PadDoc()->WsTakeSnapshot(); }
HRESULT CPadScriptSite::get_WsModule(long row, BSTR *pbstrModule)
            { return PadDoc()->get_WsModule(row, pbstrModule); }
HRESULT CPadScriptSite::get_WsSection(long row, BSTR *pbstrSection)
            { return PadDoc()->get_WsSection(row, pbstrSection); }
HRESULT CPadScriptSite::get_WsSize(long row, long *plWsSize)
            { return PadDoc()->get_WsSize(row, plWsSize); }
HRESULT CPadScriptSite::get_WsCount(long *plCount)
            { return PadDoc()->get_WsCount(plCount); }
HRESULT CPadScriptSite::get_WsTotal(long *plTotal)
            { return PadDoc()->get_WsTotal(plTotal); }
HRESULT CPadScriptSite::WsStartDelta()
            { return PadDoc()->WsStartDelta(); }
HRESULT CPadScriptSite::WsEndDelta(long *pnPageFaults)
            { return PadDoc()->WsEndDelta(pnPageFaults); }
HRESULT CPadScriptSite::SetRegValue(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT value)
            { return PadDoc()->SetRegValue(hkey, bstrSubKey, bstrValueName, value); }
HRESULT CPadScriptSite::CoMemoryTrackDisable(VARIANT_BOOL fDisable)
            { return PadDoc()->CoMemoryTrackDisable(fDisable); }
HRESULT CPadScriptSite::get_UseShdocvw(VARIANT_BOOL *pfHosted)
            { return PadDoc()->get_UseShdocvw(pfHosted); }
HRESULT CPadScriptSite::put_UseShdocvw(VARIANT_BOOL fHosted)
            { return PadDoc()->put_UseShdocvw(fHosted); }
HRESULT CPadScriptSite::GoBack(VARIANT_BOOL *pfWentBack)
            { return PadDoc()->GoBack(pfWentBack); }
HRESULT CPadScriptSite::GoForward(VARIANT_BOOL *pfWentForward)
            { return PadDoc()->GoForward(pfWentForward); }
HRESULT CPadScriptSite::TestExternal(BSTR bstrDLLName, BSTR bstrFunctionName, VARIANT *pParam, long *plRetVal)
            { return PadDoc()->TestExternal(bstrDLLName, bstrFunctionName, pParam, plRetVal); }
HRESULT CPadScriptSite::UnLoadDLL()
            { return PadDoc()->UnLoadDLL(); }
HRESULT CPadScriptSite::DeinitDynamicLibrary(BSTR bstrDLLName)
            { return PadDoc()->DeinitDynamicLibrary(bstrDLLName); }
HRESULT CPadScriptSite::IsDynamicLibraryLoaded(BSTR bstrDLLName, VARIANT_BOOL * pfLoaded)
            { return PadDoc()->IsDynamicLibraryLoaded(bstrDLLName, pfLoaded); }
HRESULT CPadScriptSite::GetRegValue(long hkey, BSTR bstrSubKey, BSTR bstrValueName, VARIANT *pValue)
            { return PadDoc()->GetRegValue(hkey, bstrSubKey, bstrValueName, pValue); }
HRESULT CPadScriptSite::DeleteRegValue(long hkey, BSTR bstrSubKey, BSTR bstrValueName)
            { return PadDoc()->DeleteRegValue(hkey, bstrSubKey, bstrValueName); }
HRESULT CPadScriptSite::TrustProvider(BSTR bstrKey, BSTR bstrProvider, VARIANT *poldValue)
            { return PadDoc()->TrustProvider(bstrKey, bstrProvider, poldValue); }
HRESULT CPadScriptSite::RevertTrustProvider(BSTR bstrKey)
            { return PadDoc()->RevertTrustProvider(bstrKey); }
HRESULT CPadScriptSite::DoReloadHistory()
            { return PadDoc()->DoReloadHistory(); }
HRESULT CPadScriptSite::ComputeCRC(BSTR bstrKey, VARIANT * pCRC)
            { return PadDoc()->ComputeCRC(bstrKey, pCRC); }
HRESULT CPadScriptSite::OpenFileStream(BSTR bstrPath)
            { return PadDoc()->OpenFileStream(bstrPath); }
HRESULT CPadScriptSite::get_ViewChangesFired(long *plCount)
            { return PadDoc()->get_ViewChangesFired(plCount); }
HRESULT CPadScriptSite::get_DataChangesFired(long *plCount)
            { return PadDoc()->get_DataChangesFired(plCount); }
HRESULT CPadScriptSite::get_DownloadNotifyMask(ULONG *pulMask)
            { return PadDoc()->get_DownloadNotifyMask(pulMask); }
HRESULT CPadScriptSite::put_DownloadNotifyMask(ULONG ulMask)
            { return PadDoc()->put_DownloadNotifyMask(ulMask); }
HRESULT CPadScriptSite::DumpMeterLog(BSTR bstrFileName)
            { return PadDoc()->DumpMeterLog(bstrFileName); }
HRESULT CPadScriptSite::GetSwitchTimers(VARIANT * pValue)
            { return PadDoc()->GetSwitchTimers(pValue); }
HRESULT CPadScriptSite::MoveMouseTo(int X, int Y, VARIANT_BOOL fLeftButton, int keyState)
            { return PadDoc()->MoveMouseTo(X, Y, fLeftButton, keyState); }
HRESULT CPadScriptSite::DoMouseButton(VARIANT_BOOL fLeftButton, BSTR action, int keyState)
            { return PadDoc()->DoMouseButton(fLeftButton, action, keyState); }
HRESULT CPadScriptSite::DoMouseButtonAt(int X, int Y,VARIANT_BOOL fLeftButton, BSTR action, int keyState)
            { return PadDoc()->DoMouseButtonAt(X, Y, fLeftButton, action, keyState); }            
HRESULT CPadScriptSite::TimeSaveDocToDummyStream(long * plTimeMicros)
            { return PadDoc()->TimeSaveDocToDummyStream(plTimeMicros); }
HRESULT CPadScriptSite::Sleep (int nTimeout)
            { return PadDoc()->Sleep (nTimeout); }
HRESULT CPadScriptSite::IsWin95 (long * pfWin95)
            { return PadDoc()->IsWin95(pfWin95); }
HRESULT CPadScriptSite::GetAccWindow( IDispatch ** ppAccWindow)
        { return PadDoc()->GetAccWindow( ppAccWindow ); }
HRESULT CPadScriptSite::GetAccObjAtPoint( long x, long y, IDispatch **ppAccObject )
        { return PadDoc()->GetAccObjAtPoint(x, y, ppAccObject ); }
HRESULT CPadScriptSite::SetKeyboard(BSTR bstrKeyboard)
        { return PadDoc()->SetKeyboard(bstrKeyboard); }
HRESULT CPadScriptSite::GetKeyboard(VARIANT *pKeyboard)
        { return PadDoc()->GetKeyboard(pKeyboard); }
HRESULT CPadScriptSite::ToggleIMEMode(BSTR bstrIME)
        { return PadDoc()->ToggleIMEMode(bstrIME); }
HRESULT CPadScriptSite::SendIMEKeys(BSTR bstrKeys)
        { return PadDoc()->SendIMEKeys(bstrKeys); }
HRESULT CPadScriptSite::Markup(VARIANT * p1,VARIANT * p2,VARIANT * p3, VARIANT * p4, VARIANT * p5, VARIANT * p6, VARIANT * p7 )
        { return PadDoc()->Markup( p1, p2, p3, p4, p5, p6, p7 ); }
HRESULT CPadScriptSite::Random(long a, long * b )
        { return PadDoc()->Random( a, b ); }
HRESULT CPadScriptSite::RandomSeed(long a )
        { return PadDoc()->RandomSeed( a ); }
HRESULT CPadScriptSite::GetHeapCounter(long a, long * b )
        { return PadDoc()->GetHeapCounter( a, b ); }
HRESULT CPadScriptSite::CreateProcess(BSTR bstrCommandLine, VARIANT_BOOL fWait)
        { return PadDoc()->CreateProcess(bstrCommandLine, fWait); }
HRESULT CPadScriptSite::GetCurrentProcessId(long * plRetVal)
        { return PadDoc()->GetCurrentProcessId(plRetVal); }

HRESULT 
CPadScriptSite::QueryStatus(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    return S_OK;
}

HRESULT 
CPadScriptSite::Exec(
        const GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT                     hr = S_OK;
    IDispatch *                 pDisp = NULL;
    IInternetSecurityManager *  pSecMgr = NULL;
    CVariant                    VarUrl;
    TCHAR                       ach[4096];  // pdlUrlLen
    DWORD                       dwSize;
    BYTE                        abSID[MAX_SIZE_SECURITY_ID];
    DWORD                       cbSID = ARRAY_SIZE(abSID);

    if (!pguidCmdGroup || *pguidCmdGroup != CGID_ScriptSite)
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
        goto Cleanup;
    }

    switch (nCmdID)
    {
    case CMDID_SCRIPTSITE_SID:
    case CMDID_SCRIPTSITE_URL:
        if (!pvarargOut)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        V_VT(pvarargOut) = VT_BSTR;

        hr = THR(_pDoc->_pObject->QueryInterface(IID_IDispatch, (void **) &pDisp));
        if (hr)
            goto Cleanup;

        // call invoke DISPID_SECURITYCTX off pDisp to get SID
        hr = THR_NOTRACE(GetDispProp(
                pDisp,
                DISPID_SECURITYCTX,
                LOCALE_SYSTEM_DEFAULT,
                &VarUrl,
                NULL,
                FALSE));
        if (hr) 
            goto Cleanup;

        if (V_VT(&VarUrl) != VT_BSTR || !V_BSTR(&VarUrl))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = THR(CoInternetParseUrl(
                V_BSTR(&VarUrl), 
                PARSE_ENCODE, 
                0, 
                ach, 
                ARRAY_SIZE(ach), 
                &dwSize, 
                0));
        if (hr)
            goto Cleanup;

        if (nCmdID == CMDID_SCRIPTSITE_URL)
        {
            hr = FormsAllocStringLen(ach, dwSize, &V_BSTR(pvarargOut));
            if (hr)
                goto Cleanup;
        }
        else    // nCmdID == CMDID_SCRIPTSITE_SID
        {
            hr = THR(CoInternetCreateSecurityManager(NULL, &pSecMgr, 0));
            if (hr)
                goto Cleanup;

            memset(abSID, 0, cbSID);

            hr = THR(pSecMgr->GetSecurityId(
                    ach, 
                    abSID, 
                    &cbSID,
                    0));
            if (hr)
                goto Cleanup;

            hr = FormsAllocStringLen(NULL, MAX_SIZE_SECURITY_ID, &V_BSTR(pvarargOut));
            if (hr)
                goto Cleanup;

            memcpy(V_BSTR(pvarargOut), abSID, MAX_SIZE_SECURITY_ID);
        }

        break;

    case CMDID_SCRIPTSITE_HTMLDLGTRUST:
        {
            if (!pvarargOut)
            {
                hr = E_POINTER;
                goto Cleanup;
            }
            V_VT(pvarargOut) = VT_BOOL;
            V_BOOL(pvarargOut) = TRUE;      // Trusted
            break;
        }

    case CMDID_SCRIPTSITE_SECSTATE:
        {
            if (!pvarargOut)
            {
                hr = E_POINTER;
                goto Cleanup;
            }
            V_VT(pvarargOut) = VT_I4;
            V_I4(pvarargOut) = (long) 2;    // SSL_SECURITY_SECURE;
            break;
        }

    default:
        hr = OLECMDERR_E_NOTSUPPORTED;
        break;

    }

Cleanup:
    ReleaseInterface(pDisp);
    ReleaseInterface(pSecMgr);
    RRETURN(hr);
}
