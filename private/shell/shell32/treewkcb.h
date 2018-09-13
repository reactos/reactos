#ifndef _TREEWKCB_H_
#define _TREEWKCB_H_

#ifdef __cplusplus

#include "treewalk.h" // for IShellTreeWalkerCallBack

class CBaseTreeWalkerCB : public IShellTreeWalkerCallBack
{
public:
    CBaseTreeWalkerCB();
    virtual ~CBaseTreeWalkerCB();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IShellTreeWalkerCallBack methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszFile, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd); 
    virtual STDMETHODIMP LeaveFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws);
    virtual STDMETHODIMP HandleError(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, HRESULT hrError);

    // *** Initailze the IShellTreeWalker * by CoCreateInstacing it
    virtual STDMETHODIMP Initialize();

protected:
    
    // IUnknown 
    UINT _cRef;
    IShellTreeWalker * _pstw;
    HRESULT _hrInit;
}; 
#endif // __cplusplus


STDAPI FolderSize(LPCTSTR pszDir, FOLDERCONTENTSINFO * pfci);


#endif // _TREEWKCB_H_
