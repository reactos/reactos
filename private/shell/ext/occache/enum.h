#ifndef __CONTROL_ENUM__
#define __CONTROL_ENUM__

#include "general.h"

class CControlFolderEnum : public IEnumIDList
{
public:
    CControlFolderEnum(STRRET &str, LPCITEMIDLIST pidl, UINT shcontf);
    
    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumIDList Methods 
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(LPENUMIDLIST *ppenum);

protected:
    ~CControlFolderEnum();

    UINT                m_cRef;      // ref count
    UINT                m_shcontf;   // enumeration flags
    LPMALLOC            m_pMalloc;
    HANDLE              m_hEnumControl;
    BOOL                m_bEnumStarted;
    TCHAR               m_szCachePath[MAX_PATH];
};

#endif
