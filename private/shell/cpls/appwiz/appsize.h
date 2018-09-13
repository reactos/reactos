#ifndef __APPSIZE_H_
#define __APPSIZE_H_

#include <runtask.h>

// Folder size computation tree walker callback class
class CAppFolderSize : public IShellTreeWalkerCallBack
{
public:
    CAppFolderSize(ULONGLONG * puSize);
    virtual ~CAppFolderSize();

    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellTreeWalkerCallBack methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd); 
    virtual STDMETHODIMP LeaveFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws);
    virtual STDMETHODIMP HandleError(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, HRESULT hrError);

    // *** Initailze the IShellTreeWalker * by CoCreateInstacing it
    HRESULT Initialize();

protected:
    ULONGLONG * _puSize;
    IShellTreeWalker * _pstw;

    UINT  _cRef;
}; 

#endif // _APPSIZE_H_