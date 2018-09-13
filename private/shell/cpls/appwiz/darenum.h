#ifndef __DARENUM_H_
#define __DARENUM_H_


class CDarwinEnumPublishedApps : public IEnumPublishedApps
{
public:

    CDarwinEnumPublishedApps(GUID * pAppCategoryId);
    ~CDarwinEnumPublishedApps();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumPublishedApps
    STDMETHODIMP Next(IPublishedApp ** ppia);
    STDMETHODIMP Reset(void);
    //STDMETHODIMP SetCategory(GUID * pAppCategoryId);

protected:

    UINT    _cRef;
    GUID    _CategoryGUID;
    BOOL    _bGuidUsed;
    DWORD   _dwNumApps;
    DWORD   _dwIndex;
    PMANAGEDAPPLICATION _prgApps;
};

#endif //__DARENUM_H_
