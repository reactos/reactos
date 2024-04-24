/*
 *  Rebar band site
 *
 *  Copyright 2007  Hervé Poussineau
 *  Copyright 2009  Andrew Hill
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

extern inline BOOL _ILIsDesktop(LPCITEMIDLIST pidl)
{
    return (pidl == NULL || pidl->mkid.cb == 0);
}

class CBandSiteBase :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IBandSite,
    public IInputObjectSite,
    public IInputObject,
    public IDeskBarClient,
    public IWinEventHandler,
    public IPersistStream,
    public IDropTarget,
    public IServiceProvider,
    public IBandSiteHelper,
    public IOleCommandTarget
{
private:
    struct BandObject
    {
        IDeskBand                           *DeskBand;
        IOleWindow                          *OleWindow;
        IWinEventHandler                    *WndEvtHandler;
        DESKBANDINFO                        dbi;
        BOOL                                bHiddenTitle;
    };

    LONG                                    m_cBands;
    LONG                                    m_cBandsAllocated;
    struct BandObject                       *m_bands;
    HWND                                    m_hwndRebar;
    CComPtr<IOleWindow>                     m_site;
    DWORD                                   m_dwState; /* BSSF_ flags */
    DWORD                                   m_dwStyle; /* BSIS_ flags */
public:
    CBandSiteBase();
    ~CBandSiteBase();

    // *** IBandSite methods ***
    STDMETHOD(AddBand)(IUnknown *punk) override;
    STDMETHOD(EnumBands)(UINT uBand, DWORD *pdwBandID) override;
    STDMETHOD(QueryBand)(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName) override;
    STDMETHOD(SetBandState)(DWORD dwBandID, DWORD dwMask, DWORD dwState) override;
    STDMETHOD(RemoveBand)(DWORD dwBandID) override;
    STDMETHOD(GetBandObject)(DWORD dwBandID, REFIID riid, void **ppv) override;
    STDMETHOD(SetBandSiteInfo)(const BANDSITEINFO *pbsinfo) override;
    STDMETHOD(GetBandSiteInfo)(BANDSITEINFO *pbsinfo) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDeskBarClient methods ***
    STDMETHOD(SetDeskBarSite)(IUnknown *punkSite) override;
    STDMETHOD(SetModeDBC)(DWORD dwMode) override;
    STDMETHOD(UIActivateDBC)(DWORD dwState) override;
    STDMETHOD(GetSize)(DWORD dwWhich, LPRECT prc) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IInputObjectSite methods ***
    STDMETHOD(OnFocusChangeIS)(struct IUnknown *paramC, int param10) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IDropTarget methods ***
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

    // *** IBandSiteHelper methods ***
    STDMETHOD(LoadFromStreamBS)(IStream *, const GUID &, void **) override;
    STDMETHOD(SaveToStreamBS)(IUnknown *, IStream *) override;

private:
    UINT _GetBandID(struct BandObject *Band);
    struct BandObject *_GetBandByID(DWORD dwBandID);
    void _FreeBand(struct BandObject *Band);
    DWORD _GetViewMode();
    VOID _BuildBandInfo(struct BandObject *Band, REBARBANDINFOW *prbi);
    HRESULT _UpdateBand(struct BandObject *Band);
    HRESULT _UpdateAllBands();
    HRESULT _UpdateBand(DWORD dwBandID);
    struct BandObject *_GetBandFromHwnd(HWND hwnd);
    HRESULT _IsBandDeletable(DWORD dwBandID);
    HRESULT _OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plrResult);

    BEGIN_COM_MAP(CBandSiteBase)
        COM_INTERFACE_ENTRY_IID(IID_IBandSite, IBandSite)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBarClient, IDeskBarClient)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IDropTarget, IDropTarget)
        COM_INTERFACE_ENTRY_IID(IID_IBandSiteHelper, IBandSiteHelper)
    END_COM_MAP()
};

class CBandSite :
    public CComCoClass<CBandSite, &CLSID_RebarBandSite>,
    public CBandSiteBase
{
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_BANDSITE)
    DECLARE_AGGREGATABLE(CBandSite)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
};
