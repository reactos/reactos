#pragma once

#define USE_CUSTOM_MENUBAND 1
#define USE_CUSTOM_MERGEDFOLDER 1
#define USE_CUSTOM_ADDRESSBAND 1
#define USE_CUSTOM_ADDRESSEDITBOX 1
#define USE_CUSTOM_BANDPROXY 1
#define USE_CUSTOM_BRANDBAND 1
#define USE_CUSTOM_EXPLORERBAND 1
#define USE_CUSTOM_INTERNETTOOLBAR 1

HRESULT CAddressBand_CreateInstance(REFIID riid, void **ppv);
HRESULT CAddressEditBox_CreateInstance(REFIID riid, void **ppv);
HRESULT CBandProxy_CreateInstance(REFIID riid, void **ppv);
HRESULT CBrandBand_CreateInstance(REFIID riid, void **ppv);
HRESULT CExplorerBand_CreateInstance(REFIID riid, LPVOID *ppv);
HRESULT CInternetToolbar_CreateInstance(REFIID riid, void **ppv);
HRESULT CMergedFolder_CreateInstance(REFIID riid, void **ppv);
HRESULT CMenuBand_CreateInstance(REFIID iid, LPVOID *ppv);
HRESULT CShellBrowser_CreateInstance(LPITEMIDLIST pidl, DWORD dwFlags, REFIID riid, void **ppv);
HRESULT CTravelLog_CreateInstance(REFIID riid, void **ppv);
HRESULT CBaseBar_CreateInstance(REFIID riid, void **ppv, BOOL vertical);
HRESULT CBaseBarSite_CreateInstance(REFIID riid, void **ppv, BOOL bVertical);
HRESULT CToolsBand_CreateInstance(REFIID riid, void **ppv);
