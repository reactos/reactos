// Create Instance functions

#ifndef _SCCLS_H_
#define _SCCLS_H_



#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0
#define COCREATEONLY NULL,NULL,VERSION_0,0,0 // piid,piidEvents,lVersion,dwOleMiscFlags,dwClassFactFlags
#define COCREATEONLY_NOFLAGS NULL,NULL,VERSION_0,0 // piid,piidEvents,lVersion,dwOleMiscFlags



STDAPI  CDocObjectFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CInternetToolbar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CQuickLinks_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAddressBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);   // address.cpp
STDAPI  CAddressEditBox_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);   // aeditbox.cpp
STDAPI  CBandProxy_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);     // bandprxy.cpp
STDAPI  CBrandBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDeskBarApp_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBrowserBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CSearchBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CCommBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CISFBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#ifdef ENABLE_CHANNELS
STDAPI  CChannelBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#endif
STDAPI  CExplorerBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBandSiteMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CComCatCacheTask_CreateInstance( IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi );
STDAPI  CComCatConditionalCacheTask_CreateInstance( IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi );
STDAPI  CAutoComplete_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLHistory_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLIShellFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLMRU_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLMulti_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CCmdFileIcon_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CTrackShellMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  ChannelOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CShellTaskScheduler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CSharedTaskScheduler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CDesktopTask_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CWinListShellProc_CreateInstance (IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAugmentedISF_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAugmentedMergeISF_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CIESplashScreen_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  COrderList_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  CMenuSite_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  CCDFCopyHook_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CRegTreeOptions_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  TaskbarList_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CInternetCacheCleaner_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CThumbnail_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CImgCtxThumb_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CImageListCache_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDocFileInfoTip_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CacheFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI  HistFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI  CCommonBrowser_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDockingBarPropertyBag_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBitmapPreload_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBrowseuiIdentity_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CUserAssist_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CGlobalFolderSettings_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

STDAPI CShellSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CWebSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

STDAPI CBaseBrowser_Validate(HWND hwnd, LPVOID* ppsb);
STDAPI CShellBrowser_CreateInstance(HWND hwnd, LPVOID* ppsb);
STDAPI CExplorerBrowser_CreateInstance(HWND hwnd, LPVOID* ppsb);
STDAPI CSDWindows_CreateInstance(IShellWindows **ppunk);

STDAPI CIEFrameAuto_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk);

// to save some typing:
#define CLSIDOFOBJECT(p)          (*((p)->_pObjectInfo->pclsid))
#define VERSIONOFOBJECT(p)          ((p)->_pObjectInfo->lVersion)
#define EVENTIIDOFCONTROL(p)      (*((p)->_pObjectInfo->piidEvents))
#define OLEMISCFLAGSOFCONTROL(p)    ((p)->_pObjectInfo->dwOleMiscFlags)

extern char g_szLibName[]; // shocx.c

#endif // _SCCLS_H_
