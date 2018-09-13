#ifndef _CHANBAR_H
#define _CHANBAR_H

#include "dbapp.h"

#define SZ_REGKEY_CHANBAR   TEXT("Software\\Microsoft\\Internet Explorer\\Channels")
#define SZ_REGVALUE_CHANBAR TEXT("window_placement")

typedef struct tagCISSTRUCT {
    UINT iVer; 
    RECT rc; 
} CISSTRUCT;

class CChannelDeskBarApp : public CDeskBarApp
{
public:
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);


    // *** IPersistPropertyBag ***
    virtual HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pPropBag,
                                           IErrorLog *pErrorLog);
protected:
    virtual LRESULT v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static  LRESULT CALLBACK DummyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnCommand(UINT msg, WPARAM wparam, LPARAM lparam);
    virtual void _OnDisplayChange();
    virtual void _OnCreate();
    virtual void _OnSize(void);
    virtual void _UpdateCaptionTitle();
    void _CreateToolbar();
    void _PositionTB();
    void _SizeTB();
    void _PersistState();
    
    BOOL _OnCloseBar(BOOL fConfirm);
    
    HWND _hwndTB;
    HWND _hwndDummy; // for propagating WM_PALETTE* messages

    LONG _cxScreen, _cyScreen;   // the screen resolution

    friend HRESULT ChannelDeskBarApp_Create(IUnknown** ppunk, IUnknown** ppbs);
} ;

HRESULT ChannelDeskBarApp_Create(IUnknown** ppunk, IUnknown** ppbs);
void ChanBarSetAutoLaunchRegValue(BOOL);
void DesktopChannel();


#endif // _CHANBAR_H
