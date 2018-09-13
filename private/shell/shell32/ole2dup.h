#include <shellp.h>

STDAPI SHCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI SHExtCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown* pUnkOuter, REFIID riid, void **ppv);
STDAPI SHCLSIDFromString(LPCTSTR lpsz, LPCLSID pclsid);
STDAPI_(HINSTANCE) SHPinDllOfCLSIDStr(LPCTSTR pszCLSID);

#define CH_GUIDFIRST TEXT('{') // '}'

//===========================================================================
// IDL DataObject
//===========================================================================
HRESULT CIDLData_CreateFromHDrop(HDROP hdrop, IDataObject **ppdtobj);
