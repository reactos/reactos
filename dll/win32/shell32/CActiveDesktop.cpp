/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     IActiveDesktop stub
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell_ad);

/***********************************************************************
*   IActiveDesktop implementation
*/

CActiveDesktop::CActiveDesktop()
{
    UNIMPLEMENTED;
}

CActiveDesktop::~CActiveDesktop()
{
    UNIMPLEMENTED;
}

HRESULT WINAPI CActiveDesktop::ApplyChanges(DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetWallpaper(PWSTR pwszWallpaper, UINT cchWallpaper, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::SetWallpaper(PCWSTR pwszWallpaper, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetWallpaperOptions(LPWALLPAPEROPT pwpo, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::SetWallpaperOptions(LPCWALLPAPEROPT pwpo, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetPattern(PWSTR pwszPattern, UINT cchPattern, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::SetPattern(PCWSTR pwszPattern, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetDesktopItemOptions(LPCOMPONENTSOPT pco, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::SetDesktopItemOptions(LPCCOMPONENTSOPT pco, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::AddDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::AddDesktopItemWithUI(HWND hwnd, LPCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::ModifyDesktopItem(LPCCOMPONENT pcomp, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::RemoveDesktopItem(LPCCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetDesktopItemCount(int *pcItems, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetDesktopItem(int nComponent, LPCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetDesktopItemByID(ULONG_PTR dwID, LPCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GenerateDesktopItemHtml(PCWSTR pwszFileName, LPCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::AddUrl(HWND hwnd, PCWSTR pszSource, LPCOMPONENT pcomp, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::GetDesktopItemBySource(PCWSTR pwszSource, LPCOMPONENT pcomp, DWORD dwReserved)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}


/***********************************************************************
*   IPropertyBag implementation
*/

HRESULT WINAPI CActiveDesktop::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI CActiveDesktop::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

