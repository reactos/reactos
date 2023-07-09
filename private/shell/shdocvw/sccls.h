// Create Instance functions

#ifndef _SCCLS_H_
#define _SCCLS_H_



#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0
#define COCREATEONLY NULL,NULL,VERSION_0,0,0 // piid,piidEvents,lVersion,dwOleMiscFlags,dwClassFactFlags
#define COCREATEONLY_NOFLAGS NULL,NULL,VERSION_0,0 // piid,piidEvents,lVersion,dwOleMiscFlags

STDAPI  CDocObjectFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CInternetFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI  CWebBrowserOC_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CInternetToolbar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CQuickLinks_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CQuickLinksOld_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAddressBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);   // address.cpp
STDAPI  CAddressEditBox_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);   // aeditbox.cpp
STDAPI  CBandProxy_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);     // bandprxy.cpp
STDAPI  CBrandBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CTargetMenu_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI  CSHItemOC_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CShellHlinkFrame_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CUrlHistory_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CURLSearchHook_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDeskBarApp_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuDeskBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CStartMenuBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBrowserBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CSearchBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CCommBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CStubBSC_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CShellDataSource_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CISFBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CFavBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CHistBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#ifdef ENABLE_CHANNELS
STDAPI  CChannelBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#endif  // ENABLE_CHANNELS
STDAPI  CExplorerBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CBandSiteMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAutoComplete_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLHistory_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLIShellFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLMRU_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CACLMulti_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CShellUIHelper_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CIntShcut_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CCmdFileIcon_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuBand_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  ChannelOC_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CShellTaskScheduler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CSharedTaskScheduler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CStartMenuTask_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CDesktopTask_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CBaseBrowser2_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);

STDAPI  CShellFolderView_CreateInstance (IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CWinListShellProc_CreateInstance (IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CAugmentedISF_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CMenuISF_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CIESplashScreen_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  COrderList_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  CActiveDesktop_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  CMenuSite_CreateInstance(IUnknown * pUnkOuter, IUnknown ** punk, LPCOBJECTINFO poi);
STDAPI  CCDFCopyHook_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CRegTreeOptions_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  TaskbarList_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CInternetCacheCleaner_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  COfflinePagesCacheCleaner_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CImgCtxThumb_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CImageListCache_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI  CDocFileInfoTip_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CDocHostUIHandler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CToolbarExtBand_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CToolbarExtExec_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
STDAPI  CNscTree_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi);
#ifdef _HSFOLDER
STDAPI  CacheFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI  HistFolder_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
#endif

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
extern LCID g_lcidLocale; // shocx.c

#endif // _SCCLS_H_






