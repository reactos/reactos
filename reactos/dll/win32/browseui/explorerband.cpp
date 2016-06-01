#include "precomp.h"

#if 1
#undef UNIMPLEMENTED

#define UNIMPLEMENTED DbgPrint("%s is UNIMPLEMENTED!\n", __FUNCTION__)
#endif

extern "C"
HRESULT WINAPI CExplorerBand_Constructor(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CExplorerBand>(riid, ppv);
}

CExplorerBand::CExplorerBand()
{
}

CExplorerBand::~CExplorerBand()
{
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetWindow(HWND *lphwnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::CloseDW(DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::ShowDW(BOOL fShow)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::SetSite(IUnknown *pUnkSite)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSite(REFIID riid, void **ppvSite)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::HasFocusIO()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetClassID(CLSID *pClassID)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::IsDirty()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::IsWindowOwner(HWND hWnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IBandNavigate methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::Select(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** INamespaceProxy ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetNavigateTarget(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::OnSelectionChanged(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::RefreshFlags(long paramC, long param10, long param14)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::CacheItem(long paramC)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDispatch methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfoCount(UINT *pctinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDropTarget methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::DragEnter(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragOver(DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::DragLeave()
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::Drop(IDataObject *pObj, DWORD glfKeyState, POINTL pt, DWORD *pdwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IDropSource methods ***
HRESULT STDMETHODCALLTYPE CExplorerBand::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CExplorerBand::GiveFeedback(DWORD dwEffect)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
