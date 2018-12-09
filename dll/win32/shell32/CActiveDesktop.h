/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IActiveDesktop header
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _CACTIVEDESKTOP_H_
#define _CACTIVEDESKTOP_H_

#undef AddDesktopItem

class CActiveDesktop :
    public CComCoClass<CActiveDesktop, &CLSID_ActiveDesktop>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IActiveDesktop,
    //public IActiveDesktopP,
    //public IADesktopP2,
    public IPropertyBag
{
public:
    CActiveDesktop();
    virtual ~CActiveDesktop();

    /*** IActiveDesktop methods ***/
    virtual HRESULT WINAPI ApplyChanges(DWORD dwFlags);
    virtual HRESULT WINAPI GetWallpaper(PWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwFlags);
    virtual HRESULT WINAPI SetWallpaper(PCWSTR pwszWallpaper, DWORD dwReserved);
    virtual HRESULT WINAPI GetWallpaperOptions(LPWALLPAPEROPT pwpo, DWORD dwReserved);
    virtual HRESULT WINAPI SetWallpaperOptions(LPCWALLPAPEROPT pwpo, DWORD dwReserved);
    virtual HRESULT WINAPI GetPattern(PWSTR pwszPattern, UINT cchPattern, DWORD dwReserved);
    virtual HRESULT WINAPI SetPattern(PCWSTR pwszPattern, DWORD dwReserved);
    virtual HRESULT WINAPI GetDesktopItemOptions(LPCOMPONENTSOPT pco, DWORD dwReserved);
    virtual HRESULT WINAPI SetDesktopItemOptions(LPCCOMPONENTSOPT pco, DWORD dwReserved);
    virtual HRESULT WINAPI AddDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI AddDesktopItemWithUI(HWND hwnd, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI ModifyDesktopItem(LPCCOMPONENT pcomp, DWORD dwFlags);
    virtual HRESULT WINAPI RemoveDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI GetDesktopItemCount(int *pcItems, DWORD dwReserved);
    virtual HRESULT WINAPI GetDesktopItem(int nComponent, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI GetDesktopItemByID(ULONG_PTR dwID, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI GenerateDesktopItemHtml(PCWSTR pwszFileName, LPCOMPONENT pcomp, DWORD dwReserved);
    virtual HRESULT WINAPI AddUrl(HWND hwnd, PCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwFlags);
    virtual HRESULT WINAPI GetDesktopItemBySource(PCWSTR pwszSource, LPCOMPONENT pcomp, DWORD dwReserved);

    /*** IPropertyBag methods ***/
    virtual HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    virtual HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);


DECLARE_REGISTRY_RESOURCEID(IDR_ACTIVEDESKTOP)
DECLARE_NOT_AGGREGATABLE(CActiveDesktop)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CActiveDesktop)
    COM_INTERFACE_ENTRY_IID(IID_IActiveDesktop, IActiveDesktop)
    //COM_INTERFACE_ENTRY_IID(IID_IActiveDesktopP, IActiveDesktopP)
    //COM_INTERFACE_ENTRY_IID(IID_IADesktopP2, IADesktopP2)
    COM_INTERFACE_ENTRY_IID(IID_IPropertyBag, IPropertyBag)
END_COM_MAP()
};


#endif // _CACTIVEDESKTOP_H_
