#ifndef __ui_h
#define __ui_h

#define UIKEY_CLASS     0
#define UIKEY_BASECLASS 1
#define UIKEY_ROOT      2
#define UIKEY_MAX       3

HRESULT GetKeysForIdList(LPCITEMIDLIST pidl, LPIDLISTDATA pData, INT cKeys, HKEY* aKeys);
HRESULT GetKeysForClass(LPWSTR pObjectClass, BOOL fIsContainer, INT cKeys, HKEY* aKeys);
void TidyKeys(INT cKeys, HKEY* aKeys);

HRESULT ShowObjectProperties(HWND hwndParent, LPDATAOBJECT pDataObject);

#endif
