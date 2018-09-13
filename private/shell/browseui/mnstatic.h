#ifndef MENUST
#define MENUST

#include "mnbase.h"
#include "cwndproc.h"
#include "droptgt.h"

class CMenuBand;

class CMenuStaticToolbar : public CMenuToolbarBase,
                           public CDelegateDropTarget,
                           public CNotifySubclassWndProc
{
public:

    // *** IUnknown (override) ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return CMenuToolbarBase::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void) { return CMenuToolbarBase::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);

    // *** CDelegateDropTarget Methods ***
    virtual HRESULT GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll);
    virtual HRESULT HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD *pdwEffect);
    virtual HRESULT GetObjectDDT (DWORD dwId, REFIID riid, LPVOID * ppvObj);
    virtual HRESULT OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, 
                            DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect);

    //*** IWinEventHandler (override) ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

    // Other public methods
    virtual void GetSize(SIZE* psize);

    virtual void v_SendMenuNotification(UINT idCmd, BOOL fClear);
    virtual BOOL v_TrackingSubContextMenu();
    virtual BOOL v_UpdateIconSize(UINT uIconSize, BOOL fUpdateButtons);
    virtual void v_Show(BOOL fShow, BOOL fForceUpdate);
    virtual void v_UpdateButtons(BOOL fNegotiateSize);

    virtual STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual void CreateToolbar(HWND hwndParent);

    virtual void v_Close(); // override
    virtual void    v_OnEmptyToolbar();        // override
    virtual void v_OnDeleteButton(LPVOID pData);
    virtual HRESULT v_InvalidateItem(LPSMDATA psmd, DWORD dwFlags);

    virtual HRESULT GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags);
    virtual HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);

    CMenuStaticToolbar(CMenuBand* pmb, HMENU hmenu, HWND hwnd, UINT idCmd, DWORD dwFlags);

protected:
    class CMenuStaticData
    {
    public:
        ~CMenuStaticData();
        void SetSubMenu(IUnknown* punk);
        HRESULT GetSubMenu(const GUID* pguidService, REFIID riid, void** ppvObj);
        IUnknown*   _punkSubMenu;
        DWORD       _dwFlags;
    };

    HWND    _hwndMenuOwner;
    HWND    _hwndDD;
    HMENU   _hmenu;
    UINT    _idCmd;
    int     _iDragOverButton;
    IContextMenu* _pcm;

    BITBOOL _fHasTopSep: 1;
    BITBOOL _fHasBottomSep: 1;
    BITBOOL _fTopSepRemoved: 1;
    BITBOOL _fBottomSepRemoved: 1;
    BITBOOL _fDirty: 1;


    LRESULT _OnAccelerator(NMCHAR* pnmChar);
    LRESULT (*_lpfnWndProc)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMenuStaticData* _IDToData(int idCmd);
    HRESULT CallCB(UINT idCmd, DWORD dwMsg, WPARAM wParam, LPARAM lParam);

protected:
    virtual ~CMenuStaticToolbar();

    // Window Proc and message handlers
    virtual LRESULT _DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnNotify(LPNMHDR pnm);

    virtual int  v_GetDragOverButton()
        { return _iDragOverButton; };

    virtual HRESULT v_GetInfoTip(int iCmd, LPTSTR psz, UINT cch);
    virtual HRESULT v_CallCBItem(int idtCmd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual HRESULT v_GetState(int idtCmd, LPSMDATA psmd);
    virtual HRESULT v_ExecItem(int iCmd);
    virtual DWORD v_GetFlags(int iCmd);
    virtual void v_Refresh();
    virtual HRESULT v_GetSubMenu(int iCmd, const GUID* pguidService, REFIID riid, void** ppvObj);
    virtual HRESULT v_CreateTrackPopup(int idCmd, REFIID riid, void** ppvObj);

    LRESULT _OnGetObject(NMOBJECTNOTIFY*);
    LRESULT _OnContextMenu(WPARAM wParam, LPARAM lParam);
    void _FillToolbar();
    void _OnGetDispInfo(LPNMHDR pnm, BOOL fUnicode);
    void _Insert(int iIndex, MENUITEMINFO* pmii);
    void _CheckSeparators();
};


#endif // MENUST
