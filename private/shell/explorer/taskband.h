#ifndef TASKBAND_H_
#define TASKBAND_H_

typedef struct _CTasks {
    HWND hwnd;          // the view window
    HWND hwndTab;        // owner draw listbox of tasks

    UINT WM_ShellHook;

    int iSysMenuCount;
    HWND    hwndSysMenu;
    DWORD   dwPos;
    HWND    hwndLastRude;

} CTasks, * PTasks;


#ifdef __cplusplus

class CSimpleOleWindow : public IDeskBar // public IOleWindow, 
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IDeskBar ***
    STDMETHOD(OnPosRectChangeDB)(THIS_ LPRECT prc)
        { ASSERT(0); return E_NOTIMPL; }
    STDMETHOD(SetClient)          (THIS_ IUnknown* punkClient)
        { return E_NOTIMPL; }
    STDMETHOD(GetClient)          (THIS_ IUnknown** ppunkClient)
        { return E_NOTIMPL; }

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }

    CSimpleOleWindow(HWND hwnd);
    
protected:
    
    virtual ~CSimpleOleWindow();
    
    UINT _cRef;
    HWND _hwnd;
};

extern "C" CTasks g_tasks;
#endif


#endif //TASKBAND_H_
