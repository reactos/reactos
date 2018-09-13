//
// FILE:    oleglue.h
//
// NOTES:   All OLE-related outbound references from PBrush
//

#include <ole2.h>

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if DBG
#define DOUT(t)    OutputDebugString(t)
#define DOUTR(t)   OutputDebugString(t L"\n")
#else // !DBG
#define DOUT(t)
#define DOUTR(t)
#endif

extern BOOL gfInDialog;
extern BOOL gfTerminating;
extern BOOL gfClosing;

extern DWORD dwOleBuildVersion;
extern BOOL gfOleInitialized;

extern BOOL gfUserClose;

extern BOOL gfStandalone;
extern BOOL gfEmbedded;
extern BOOL gfLinked;
extern BOOL gfInPlace;

extern BOOL gfLoading;
extern BOOL gfInvisible;
extern int iExitWithSaving;

extern HICON ghiconApp;

extern OLECHAR gachLinkFilename[_MAX_PATH];

extern LPWSTR GetClientObjName(void);

extern BOOL gfWholeHog;
extern void GetPickRect(LPRECT prc);

void LockPBObject(void);
void UnLockPBObject(void);

//
// helpers for getting our clipboard on (and safely removed from)
// the OLE clipboard...
extern BOOL gfXBagOnClipboard;
void TransferToClipboard(void);

//
// Pick state save/restore helpers
//
void SavePickState(void);
void SelectWholePicture(void);
void RestorePickState(void);

//
// Transfer Helpers
//
extern CLIPFORMAT gcfToPaste;
extern HGLOBAL ghGlobalToPaste;
extern HBITMAP ghBitmapSnapshot;
extern HPALETTE ghPaletteSnapshot;
extern RECT grcSnapshot;
extern BOOL gfTransfer;

void SetTransferFlag(BOOL fTransfer);

void FlushOleClipboard(void);
void FreeGlobalToPaste(void);
BOOL OleClipboardContainsAcceptableFormats(CLIPFORMAT FAR* lpcf);
BOOL GetTypedHGlobalFromOleClipboard(CLIPFORMAT cf, HGLOBAL FAR* lphGlobal);
void SetupForDrop(CLIPFORMAT cf, POINTL ptl);
void PasteTypedHGlobal(CLIPFORMAT cf, HGLOBAL hGlobal);
HPALETTE GetTransferPalette(void);
HBITMAP GetTransferBitmap(void);


BOOL InitializePBS(HINSTANCE hInst, LPTSTR lpCmdLine);
HRESULT ReleasePBClassFactory(void);
BOOL CreatePBClassFactory(HINSTANCE hinst,BOOL fEmbedded);
BOOL CreateStandaloneObject(void);

void BuildUniqueLinkName(void);

void RegisterAsDropTarget(HWND hwnd);
void RevokeOurDropTarget(void);


void SetNativeExtents( int cx, int cy );
void GetInPlaceInfo(LPOLEINPLACEFRAME *ppFrame, OLEINPLACEFRAMEINFO **ppInfo);
int CalcMenuPos(int iMenu);
HWND GetInPlaceFrameWindow();

void DoOleClose(BOOL fSave);
void DoOleSave(void);
void TerminateServer(void);
void FlushOleClipboard(void);
void AdviseDataChange(void);
void AdviseRename(LPTSTR lpname);

HANDLE GetNativeData(void);
BOOL PutNativeData(LPBYTE lpbData, HWND hWnd);
void RenderPicture(HDC hdc, LPCRECTL lprectl);

//
// for Access to global hwnds
//
#define iFrame  0
#define iPaint  1
#define iTool   2
#define iSize   3
#define iColor  4
extern HWND *gpahwndApp;
extern LPRECT gprcApp;

#ifndef SHOWWINDOW
//
// PBrush C stuff accessed by C++ classes
//
/* application messages */
#define WM_HIDECURSOR WM_USER
#define WM_TERMINATE  (WM_USER + 1)
#define WM_CHANGEFONT (WM_USER + 2)
#define WM_ZOOMUNDO   (WM_USER + 3)
#define WM_ZOOMACCEPT (WM_USER + 4)
#define WM_SCROLLINIT (WM_USER + 5)
#define WM_SCROLLDONE (WM_USER + 6)
#define WM_SCROLLVIEW (WM_USER + 7)
#define WM_PICKFLIPH  (WM_USER + 8)
#define WM_PICKFLIPV  (WM_USER + 9)
#define WM_PICKINVERT (WM_USER + 10)
#define WM_PICKSG     (WM_USER + 11)
#define WM_PICKTILT   (WM_USER + 12)
#define WM_PICKCLEAR  (WM_USER + 13)
#define WM_MOUSEPOS   (WM_USER + 14)
#define WM_COPYTO     (WM_USER + 15)
#define WM_PASTEFROM  (WM_USER + 16)
#define WM_WHOLE      (WM_USER + 17)
#define WM_SHOWCURSOR (WM_USER + 18)
#define WM_MOUSESYS   (WM_USER + 19)
#define WM_ERRORMSG   (WM_USER + 20)
#define WM_SELECTTOOL (WM_USER + 21)
#define WM_OUTLINE    (WM_USER + 22)

#define SHOWWINDOW      1
#define HIDEWINDOW      0
#define NOCHANGEWINDOW  2
void CalcWnds(int disptools, int displine, int dispcolor, int disppaint);
void MenuCmd(HWND hWnd, UINT item);
void ResetPaintWindow(void);

#endif //SHOWWINDOW

#ifndef MAXmenus
#define MAXmenus 6
#endif

extern HMENU ghMenuFrame;
extern BOOL gafMenuPresent[MAXmenus];

#define FILEclear      107

#define IDSEdit                 401
#define IDSFileOpen             1043

#ifdef __cplusplus
}
#endif  /* __cplusplus */


