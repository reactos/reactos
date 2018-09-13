#ifndef __APPPUB_H_
#define __APPPUB_H_

/////////////////////////////////////////////////////////////////////////////
// CDarwinAppPublisher
class CDarwinAppPublisher : public IAppPublisher
{
public:
    CDarwinAppPublisher();
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAppPublisher
    STDMETHODIMP GetNumberOfCategories(DWORD * pdwCat);
    STDMETHODIMP GetCategories(APPCATEGORYINFOLIST * pAppCategoryList);
    STDMETHODIMP GetNumberOfApps(DWORD * pdwApps);
    STDMETHODIMP EnumApps(GUID * pAppCategoryId, IEnumPublishedApps ** ppepa);

protected:

    virtual ~CDarwinAppPublisher();

    UINT _cRef;
};

#endif //__APPPUB_H_
