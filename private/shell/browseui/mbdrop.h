#ifndef _MBDROP_H_
#define _MBDROP_H_

// The CMenuBand class handles all menu behavior for bands.  

class CMenuBandDropTarget : public IDropTarget
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *dtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    
    CMenuBandDropTarget(HWND hwnd, int idTarget, DWORD dwFlagsMBIF);

protected:

    // Member variables
    int     _cRef;
    HWND    _hwndParent;
    IDropTarget *_pdrop;    // hand on to the the favorites target
    int     _iDropType;     // Which format data is in.
    int     _idTimer;

    int     _idTarget;      // ID of menu item we're hovering over
    DWORD   _dwFlagsMBIF;   // MBIF_*
};


#endif  // _MBDROP_H_

