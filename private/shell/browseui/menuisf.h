#ifndef _MENUISF_H
#define _MENUISF_H

#include "iface.h"
#include "caggunk.h"
#include "menubar.h"


//  Object that uses TrackPopupMenu as its implementation for IMenuPopup.
//

class CTrackPopupBar : public CMenuDeskBar
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef) () ;
    STDMETHOD_(ULONG,Release) ();

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);
    
    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd) { return E_NOTIMPL; }
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL bEnterMode) { return E_NOTIMPL; }

    // *** IMenuPopup methods ***
    virtual STDMETHODIMP OnSelect(DWORD dwSelectType);
    virtual STDMETHODIMP SetSubMenu(IMenuPopup* pmp, BOOL fSet);
    virtual STDMETHODIMP Popup(POINTL *ppt, RECTL *prcExclude, DWORD dwFlags);

    CTrackPopupBar(void* pvContext, int id, HMENU hmenu, HWND hwnd);
    ~CTrackPopupBar();
    
    HMENU GetPopupMenu() { return GetSubMenu(_hmenu, _id); };
    void SelectFirstItem();
    
protected:
    int     _id;
    HMENU   _hmenu;
    HWND    _hwndParent;
    void*   _pvContext;
    
    // Popup message to indicate - "Ignore next MENUSELECT clear msg"
    UINT _nMBIgnoreNextDeselect;
};
#endif
