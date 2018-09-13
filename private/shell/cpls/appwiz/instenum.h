#ifndef __INSTENUM_H_
#define __INSTENUM_H_

/////////////////////////////////////////////////////////////////////////////
// CEnumInstalledApps
class CEnumInstalledApps : public IEnumInstalledApps
{
public:

    CEnumInstalledApps(void);
    
    // *** IUnknown Methods
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumInstalledApps
    STDMETHODIMP Next(IInstalledApp ** ppia);
    STDMETHODIMP Reset(void);

protected:

    virtual ~CEnumInstalledApps(void);

    HRESULT _GetNextLegacyAppFromRegistry(IInstalledApp ** ppia);
    HRESULT _GetNextLegacyApp(IInstalledApp ** ppia);
    HRESULT _GetNextDarwinApp(IInstalledApp ** ppia);
    
    UINT _cRef;
    
    DWORD    _iEnumIndex;     // Total Application Enumeration index.
    DWORD    _iIndexEach;     // Shared index by Legacy or Darwin or SMS 
    BITBOOL  _bEnumLegacy : 1;
    BITBOOL  _bEnumHKCU : 1;
    HKEY     _hkeyUninstall;
};

#endif //__INSTENUM_H_
