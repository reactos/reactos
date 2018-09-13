#ifndef _INFOTIP_H_
#define _INFOTIP_H_

STDAPI CreateInfoTipFromText(LPCTSTR pszText, REFIID riid, void **ppv);
STDAPI CreateInfoTipFromItem(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPCWSTR pszProps, REFIID riid, void **ppv);

#endif // _INFOTIP_H_
