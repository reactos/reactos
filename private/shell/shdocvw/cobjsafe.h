
#ifndef SAFEOBJ_H_
#define SAFEOBJ_H_

// Static functions of interest to others
HRESULT DefaultGetSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
HRESULT DefaultSetSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
HRESULT MakeSafeForScripting(IUnknown **punk); // returns TRUE if punk is safe for scripting

class CObjectSafety : public IObjectSafety
{
public:
    // IUnknown (we multiply inherit from IUnknown, disambiguate here)
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    
    // IObjectSafety
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
    
protected:
    DWORD           _dwSafetyOptions;   // IObjectSafety IID_IDispatch options

};
   
#endif
