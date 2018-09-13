#ifndef _DROPTGT_H_
#define _DROPTGT_H_

// There are two drag-drop support objects:
//
//  CDropTargetWrap -       This object takes a collection of drop-target
//                          objects and wraps them as one drop-target
//                          handler.  The first drop-target wins over the
//                          the last one if there is a conflict in who
//                          will take the drop.
//
//  CDelegateDropTarget -   This class implements IDropTarget given an 
//                          IDelegateDropTargetCB interface.  It handles 
//                          all hit testing, caching, and scrolling for you.
//                          Use this class by inheriting it in your derived
//                          class; it is not intended to be instantiated alone.
//

// Event notifications for HitTestDDT
#define HTDDT_ENTER     0
#define HTDDT_OVER      1
#define HTDDT_LEAVE     2

class CDelegateDropTarget : public IDropTarget
{        
public:
    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** Other methods to be implemented by derived class ***

    virtual HRESULT GetWindowsDDT (HWND * phwndLock, HWND * phwndScroll) PURE;
    virtual HRESULT HitTestDDT (UINT nEvent, LPPOINT ppt, DWORD * pdwId, DWORD *pdwEffect) PURE;
    virtual HRESULT GetObjectDDT (DWORD dwId, REFIID riid, LPVOID * ppvObj) PURE;
    virtual HRESULT OnDropDDT (IDropTarget *pdt, IDataObject *pdtobj, 
                            DWORD * pgrfKeyState, POINTL pt, DWORD *pdwEffect) PURE;

    friend IDropTarget* DropTargetWrap_CreateInstance(IDropTarget* pdtPrimary, 
                                           IDropTarget* pdtSecondary,
                                           HWND hwnd, IDropTarget* pdt3 = NULL);
protected:
    CDelegateDropTarget();
    virtual ~CDelegateDropTarget();

    BOOL IsValid() { return (_hwndLock && _hwndScroll); }
    void SetCallback(IDelegateDropTargetCB* pdtcb);
    HRESULT Init(); // init lock + scroll windows
    friend IDropTarget* DelegateDropTarget_CreateInstance(IDelegateDropTargetCB* pdtcb);

private:
    void _ReleaseCurrentDropTarget();

    // the below are parameters we use to implement this IDropTarget
    HWND                    _hwndLock;
    HWND                    _hwndScroll;

    // the object we are dragging
    LPDATAOBJECT            _pDataObj;      // from DragEnter()/Drop()

    // the below indicate the current drag state
    BITBOOL                 _fPrime:1;      // TRUE iff _itemOver/_grfKeyState is valid
    DWORD                   _itemOver;      // item we are visually dragging over
    IDropTarget*            _pdtCur;        // drop target for _itemOver
    DWORD                   _grfKeyState;   // cached key state
    DWORD                   _dwEffectOut;   // last *pdwEffect out
    POINT                   _ptLast;        // last dragged position

    // for scrolling
    RECT                    _rcLockWindow;  // WindowRect of hwnd for DAD_ENTER
    AUTO_SCROLL_DATA        _asd;           // for auto scrolling
    
} ;

// dummy drop target to only call DAD_DragEnterEx() on DragEnter();

class CDropDummy : public IDropTarget
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void)   
    { 
        DAD_DragLeave();  
        return(S_OK); 
    };
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)   
    { 
        DragLeave();
        return(S_OK); 
    };

    CDropDummy(HWND hwndLock) : _hwndLock(hwndLock), _cRef(1)  { return; };
protected:
    ~CDropDummy()    { return; };
private:
    HWND _hwndLock;         // window for dummy drop target.
    int  _cRef;

};


#endif // _DROPTGT_H_
