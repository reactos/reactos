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
    STDMETHOD(ApplyChanges)(DWORD dwFlags) override;
    STDMETHOD(GetWallpaper)(PWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwFlags) override;
    STDMETHOD(SetWallpaper)(PCWSTR pwszWallpaper, DWORD dwReserved) override;
    STDMETHOD(GetWallpaperOptions)(LPWALLPAPEROPT pwpo, DWORD dwReserved) override;
    STDMETHOD(SetWallpaperOptions)(LPCWALLPAPEROPT pwpo, DWORD dwReserved) override;
    STDMETHOD(GetPattern)(PWSTR pwszPattern, UINT cchPattern, DWORD dwReserved) override;
    STDMETHOD(SetPattern)(PCWSTR pwszPattern, DWORD dwReserved) override;
    STDMETHOD(GetDesktopItemOptions)(LPCOMPONENTSOPT pco, DWORD dwReserved) override;
    STDMETHOD(SetDesktopItemOptions)(LPCCOMPONENTSOPT pco, DWORD dwReserved) override;
    STDMETHOD(AddDesktopItem)(LPCCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(AddDesktopItemWithUI)(HWND hwnd, LPCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(ModifyDesktopItem)(LPCCOMPONENT pcomp, DWORD dwFlags) override;
    STDMETHOD(RemoveDesktopItem)(LPCCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(GetDesktopItemCount)(int *pcItems, DWORD dwReserved) override;
    STDMETHOD(GetDesktopItem)(int nComponent, LPCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(GetDesktopItemByID)(ULONG_PTR dwID, LPCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(GenerateDesktopItemHtml)(PCWSTR pwszFileName, LPCOMPONENT pcomp, DWORD dwReserved) override;
    STDMETHOD(AddUrl)(HWND hwnd, PCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwFlags) override;
    STDMETHOD(GetDesktopItemBySource)(PCWSTR pwszSource, LPCOMPONENT pcomp, DWORD dwReserved) override;

    /*** IPropertyBag methods ***/
    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) override;
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar) override;

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
