
#ifndef _bandsite_h
#define _bandsite_h

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

void BandSite_HandleDelayBootStuff(IUnknown *punk);
BOOL BandSite_HandleMessage(IUnknown *punk, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);
void BandSite_SetMode(IUnknown *punk, DWORD dwMode);
void BandSite_Update(IUnknown *punk);
void BandSite_UIActivateDBC(IUnknown *punk, DWORD dwState);
void BandSite_HandleMenuCommand(IUnknown* punk, UINT idCmd);
void BandSite_AddMenus(IUnknown* punk, HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);
void BandSite_Load();
HRESULT UnkTranslateAcceleratorIO(IUnknown* punk, LPMSG lpMsg);
HRESULT UnkUIActivateIO(IUnknown *punkThis, BOOL fActivate, LPMSG lpMsg);
HRESULT UnkOnFocusChangeIS(IUnknown *punkThis, IUnknown *punkSrc, BOOL fSetFocus);
HRESULT BandSite_SimulateDrop(IUnknown* punk, IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
HRESULT BandSite_DragEnter(IUnknown* punk, IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
HRESULT BandSite_DragOver(IUnknown* punk, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
HRESULT BandSite_DragLeave(IUnknown* punk);
STDAPI_(LRESULT) Tray_OnMarshalBS(WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
};       /* End of extern "C" { */
#endif // __cplusplus

#endif

