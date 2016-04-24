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
    };

    LONG                                    fBandsCount;
    LONG                                    fBandsAllocated;
    struct BandObject                       *fBands;
    HWND                                    fRebarWindow;
    CComPtr<IOleWindow>                     fOleWindow;
public:
    CBandSiteBase();
    ~CBandSiteBase();

    // *** IBandSite methods ***
    virtual HRESULT STDMETHODCALLTYPE AddBand(IUnknown *punk);
    virtual HRESULT STDMETHODCALLTYPE EnumBands(UINT uBand, DWORD *pdwBandID);
    virtual HRESULT STDMETHODCALLTYPE QueryBand(DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName);
    virtual HRESULT STDMETHODCALLTYPE SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE RemoveBand(DWORD dwBandID);
    virtual HRESULT STDMETHODCALLTYPE GetBandObject(DWORD dwBandID, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetBandSiteInfo(const BANDSITEINFO *pbsinfo);
    virtual HRESULT STDMETHODCALLTYPE GetBandSiteInfo(BANDSITEINFO *pbsinfo);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDeskBarClient methods ***
    virtual HRESULT STDMETHODCALLTYPE SetDeskBarSite(IUnknown *punkSite);
    virtual HRESULT STDMETHODCALLTYPE SetModeDBC(DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE UIActivateDBC(DWORD dwState);
    virtual HRESULT STDMETHODCALLTYPE GetSize(DWORD dwWhich, LPRECT prc);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(struct IUnknown *paramC, int param10);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IDropTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual HRESULT STDMETHODCALLTYPE DragLeave();
    virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IBandSiteHelper methods ***
    virtual HRESULT STDMETHODCALLTYPE LoadFromStreamBS(IStream *, const GUID &, void **);
    virtual HRESULT STDMETHODCALLTYPE SaveToStreamBS(IUnknown *, IStream *);

private:
    UINT GetBandID(struct BandObject *Band);
    struct BandObject *GetBandByID(DWORD dwBandID);
    void FreeBand(struct BandObject *Band);
    DWORD GetBandSiteViewMode();
    VOID BuildRebarBandInfo(struct BandObject *Band, REBARBANDINFOW *prbi);
    HRESULT UpdateSingleBand(struct BandObject *Band);
    HRESULT UpdateAllBands();
    HRESULT UpdateBand(DWORD dwBandID);
    struct BandObject *GetBandFromHwnd(HWND hwnd);

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
    DECLARE_REGISTRY_RESOURCEID(IDR_REBARBANDSITE)
    DECLARE_AGGREGATABLE(CBandSite)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
};
