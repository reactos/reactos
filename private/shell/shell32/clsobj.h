#include "unicpp\deskhtm.h"

#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0

STDAPI CActiveDesktop_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDeskMovr_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCopyToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CMoveToMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewMimeFilter_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CDeskHtmlProp_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellDispatch_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CShellFolderViewOC_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CWebViewFolderContents_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CFolderOptionsPsx_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CStartMenu_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CCmdFileIcon_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);

STDAPI CSendToMenu_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CNewMenu_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CStartMenuTask_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDesktopTask_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CFolderShortcut_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CFileSearchBand_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
#ifdef WINNT
STDAPI CMountedVolume_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
#endif
STDAPI CFileTypes_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDelegateFolder_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CDragImages_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv);
STDAPI CExeDropTarget_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
STDAPI CShellMonikerHelper_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);


#ifdef FEATURE_FOLDER_INFOTIP    
STDAPI CFolderInfoTip_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
#endif FEATURE_FOLDER_INFOTIP    


STDAPI CEncryptionContextMenuHandler_CreateInstance(IUnknown *punk, REFIID riid, void **pcpOut);


