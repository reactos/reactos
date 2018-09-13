#include "deskhtm.h"

#define VERSION_2 2 // so we don't get confused by too many integers
#define VERSION_1 1
#define VERSION_0 0

STDAPI CActiveDesktop_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CDeskMovr_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CCopyToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CMoveToMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CWebViewMimeFilter_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CDeskHtmlProp_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CShellDispatch_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);
STDAPI CShellFolderView_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);
STDAPI CShellFolderViewOC_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut);
STDAPI CWebViewFolderContents_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI  CFolderOptionsPsx_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI  CStartMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut);
STDAPI CCmdFileIcon_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk);

STDAPI  CSendToMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk);
STDAPI  CNewMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk);
STDAPI  CStartMenuTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk);
STDAPI  CDesktopTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppunk);
