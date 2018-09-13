#ifndef MENUSF
#define MENUSF

#include "mnbase.h"
#include "sftbar.h"

class CMenuData : public IBDATA
{
protected:
    IUnknown* _punkSubmenu;

public:
    CMenuData(PORDERITEM poi) : IBDATA(poi) {};
    virtual ~CMenuData();

    void SetSubMenu(IUnknown* pmb2);
    HRESULT GetSubMenu(const GUID* pguidService, REFIID riid, void**);
};

class CMenuBand;

class CMenuSFToolbar :  public CSFToolbar,
                        public CMenuToolbarBase
{
public:

    // *** IUnknown (override) ***
    //   This deliberately forwards AddRef and Release to CMenuToolbarBase, and
    //   forwards QI separately to CSFToolbar.
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CMenuToolbarBase::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void) { return CMenuToolbarBase::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** IShellChangeNotify methods ***
    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    // *** IWinEventHandler methods (override) ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

    // *** CDelegateDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual HRESULT HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD *pdwEffect);
    virtual HRESULT GetObjectDDT (DWORD dwId, REFIID riid, LPVOID * ppvObj);
    virtual HRESULT OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

    // Other public methods
    virtual HWND v_GetHWND() 
        { return _hwndTB; };

    virtual void v_ForwardMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void v_SendMenuNotification(UINT idCmd, BOOL fClear);
    virtual BOOL v_TrackingSubContextMenu()  { return (BOOL) ( _pcm2 ? 1 : 0 );}; // Win64: should be ok since _pcm2 used as boolean in all contexts
    virtual BOOL v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons);
    virtual void v_Close();
    virtual void v_Show(BOOL fShow, BOOL fForceUpdate);
    virtual void v_UpdateButtons(BOOL fNegotiateSize);
    virtual void NegotiateSize();
    virtual void Expand(BOOL fExpand);

    virtual void GetSize(SIZE* size);
    virtual void SetWindowPos(LPSIZE psize, LPRECT prc, DWORD dwFlags);
    virtual void CreateToolbar(HWND hwndParent);

    virtual void    SetParent(HWND hwndParent);
    virtual HRESULT GetShellFolder(LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj);
    virtual void    v_OnEmptyToolbar();        // override
    virtual void v_OnDeleteButton(LPVOID pData);
    virtual HRESULT v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual HRESULT SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidl);

    // Change HKEY to IStream*
    CMenuSFToolbar(CMenuBand* pmb, IShellFolder* psf, LPCITEMIDLIST pidl, HKEY hKey, DWORD dwFlags);

protected:
    ~CMenuSFToolbar();

    // Window Proc Overrides
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam);
    virtual void _OnDragBegin(int iItem, DWORD dwPreferedEffect);
    virtual void _OnFSNotifyAdd(LPCITEMIDLIST pidl);
    virtual void _OnFSNotifyRemove(LPCITEMIDLIST pidl);
    virtual HRESULT OnTranslatedChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

    virtual void _FillToolbar();
    virtual LRESULT _OnNotify(LPNMHDR pnm);
    virtual LRESULT _OnTimer(WPARAM wParam);
    virtual LRESULT _OnCustomDraw(NMCUSTOMDRAW * pnmcd) 
        { return CMenuToolbarBase::_OnCustomDraw(pnmcd); };
    virtual void    _SetToolbarState()
        { CMenuToolbarBase::_SetToolbarState(); };

    virtual void EmptyToolbar()
        { CMenuToolbarBase::EmptyToolbar(); };

    virtual int  v_GetDragOverButton()
        { return _tbim.iButton; };

    virtual HRESULT v_GetSubMenu(int iCmd, const GUID* pguidService, REFIID riid, void** pObj);
    virtual HRESULT v_GetInfoTip(int iCmd, LPTSTR psz, UINT cch);
    virtual HRESULT v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT v_GetState(int idtCmd, LPSMDATA psmd);

    virtual HRESULT v_ExecItem(int iCmd);
    virtual DWORD v_GetFlags(int iCmd);
    virtual void v_Refresh() 
        { _Refresh(); };
    virtual void v_CalcWidth(int* pcxMin, int* pcxMax) 
        { CMenuToolbarBase::v_CalcWidth(pcxMin, pcxMax); };

    // Utility Functions
    virtual BOOL _AddPidl(LPITEMIDLIST pidl, int index);
    virtual PIBDATA _AddOrderItemTB(PORDERITEM poi, int index, TBBUTTON* ptbb);
    virtual void _Dropped(int nIndex, BOOL fDroppedOnSource);
    virtual HRESULT _TBStyleForPidl(LPCITEMIDLIST pidl, DWORD * pdwStyle, 
        DWORD* pdwState, DWORD * pdwFlags,int* piIcon);

    virtual void _ToolbarChanged();
    virtual void _FillDPA(HDPA hdpa, HDPA hdpaSort, DWORD dwEnumFlags);
    virtual BOOL _FilterPidl(LPCITEMIDLIST pidl);
    virtual void _ObtainPIDLName(LPCITEMIDLIST pidl, LPTSTR psz, int cchMax);
    virtual HMENU _GetContextMenu(IContextMenu* pcm, int* pid);
    virtual void _OnDefaultContextCommand(int idCmd);
    virtual void v_NewItem(LPCITEMIDLIST pidl);
    virtual void _SetDirty(BOOL fDirty);
    virtual void _SetFontMetrics();
    virtual int  _GetBitmap(int iCommandID, PIBDATA pibdata, BOOL fUseCache);
    virtual HWND GetHWNDForUIObject();
    virtual HWND CreateWorkerWindow();

    static  void s_IconCallback(LPVOID pvData, UINT uId, UINT iIconIndex);

    void _AddChevron();
    void _RemoveChevron();
    void _RefreshInfo();

    void BroadcastIntelliMenuState(LPCITEMIDLIST pidlItem, BOOL fPromoted);

    BOOL _ReBindToFolder(LPCITEMIDLIST pidl);
    
    virtual HRESULT _LoadOrderStream();
    virtual HRESULT _SaveOrderStream();

    virtual HRESULT _AfterLoad();
    HRESULT _GetInfo(LPCITEMIDLIST pidl, SMINFO* pmbiinfo);
    HRESULT CallCB(LPCITEMIDLIST pidl, DWORD dwMsg, WPARAM wParam, LPARAM lParam);
    HKEY _GetKey(LPCITEMIDLIST pidl);
    void _MarkItem(int idCmd);
    LRESULT _OnGetObject(NMOBJECTNOTIFY*);
    void _FindMinPromotedItems(BOOL fSetOrderStream);


    virtual LRESULT _OnDropDown(LPNMTOOLBAR pnmtb);

    virtual HRESULT v_CreateTrackPopup(int, REFIID, void**) 
        { AssertMsg(0, TEXT("MenuSF trying to get a TrackPopup")); return E_FAIL; };

    virtual PIBDATA _CreateItemData(PORDERITEM poi);

    // Member Variables
    HKEY    _hKey;
    BITBOOL _fPreventToolbarChange: 1;

    IAugmentedShellFolder2* _pasf2;

    int     _iDefaultIconIndex;

    int     _cMinPromotedItems;

};

#define MNFOLDER_NORODER   -5       // Some random negative number to denote there is no order
#define MNFOLDER_IS_PARENT -1       // -1 is passed to the child as uIdParent so that 
                                    // it knows it's not rooted at a static menu
#endif  // MENUSF
