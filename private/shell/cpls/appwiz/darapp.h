#ifndef __DARAPP_H_
#define __DARAPP_H_


/////////////////////////////////////////////////////////////////////////////
// CDarwinPublishedApp
class CDarwinPublishedApp : public IPublishedApp
{
public:
    // Constructor for Darwin Apps
    CDarwinPublishedApp(MANAGEDAPPLICATION * ppdi);
    ~CDarwinPublishedApp();

    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellApp
    STDMETHODIMP GetAppInfo(PAPPINFODATA pai);
    STDMETHODIMP GetPossibleActions(DWORD * pdwActions);
    STDMETHODIMP GetSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP GetCachedSlowAppInfo(PSLOWAPPINFO psai);
    STDMETHODIMP IsInstalled(void);
    
    // *** IPublishedApp
    STDMETHODIMP Install(LPSYSTEMTIME pftInstall);
    STDMETHODIMP GetPublishedAppInfo(PPUBAPPINFO ppai);
    STDMETHODIMP Unschedule(void);
    
protected:

    LONG _cRef;
    DWORD _dwAction;

    // Specific info on this Darwin App
    MANAGEDAPPLICATION _ma;
};

#endif //__DARAPP_H_
