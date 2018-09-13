/*****************************************************************************\
* PWIN16.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides macros to map portable windows code to its 16 bit form.
*
* Copyright (c) 1992-1994, Microsoft Corp.	All rights reserved.
*
\*****************************************************************************/

/*-----------------------------------USER------------------------------------*/
 
DWORD FAR PASCAL     MGetLastError(VOID);
DWORD FAR PASCAL     MSendMsgEM_GETSEL(HWND hDlg, INT FAR *piStart, INT FAR *piEnd);

/* HELPER MACROS */

#define MAPVALUE(v16, v32)              (v16)
#define MAPTYPE(v16, v32)               v16
#define MAKEMPOINT(l)                   (*((MPOINT FAR *)&(l)))
#define MPOINT2POINT(mpt, pt)           (pt = *(POINT FAR *)&(mpt))
#define POINT2MPOINT(pt, mpt)           (mpt = *(MPOINT FAR *)&(pt))
#define LONG2POINT(l, pt)               ((pt).x = (INT)LOWORD(l), (pt).y = (INT)HIWORD(l))

#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowWord(hwnd, index)
#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowWord(hwnd, index, (WORD)(ui))
#define SETCLASSUINT(hwnd, index, ui)   (UINT)SetClassWord(hwnd, index, (WORD)(ui))
#define GETCLASSUINT(hwnd, index)       (UINT)GetClassWord(hwnd, index)

#define GETCBCLSEXTRA(hwnd)             GETCLASSUINT(hwnd, GCW_CBCLSEXTRA)
#define SETCBCLSEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCW_CBCLSEXTRA, cb)
#define GETCBWNDEXTRA(hwnd)             GETCLASSUINT(hwnd, GCW_CBWNDEXTRA)     
#define SETCBWNDEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCW_CBWNDEXTRA, cb) 
#define GETCLASSBRBACKGROUND(hwnd)      (HBRUSH)GETCLASSUINT(hwnd, GCW_HBRBACKGROUND)
#define SETCLASSBRBACKGROUND(hwnd, h)   (HBRUSH)SETCLASSUINT(hwnd, GCW_HBRBACKGROUND, h)
#define GETCLASSCURSOR(hwnd)            (HCURSOR)GETCLASSUINT(hwnd, GCW_HCURSOR)
#define SETCLASSCURSOR(hwnd, h)         (HCURSOR)SETCLASSUINT(hwnd, GCW_HCURSOR, h)
#define GETCLASSHMODULE(hwnd)           (HMODULE)GETCLASSUINT(hwnd, GCW_HMODULE)            
#define SETCLASSHMODULE(hwnd, h)        (HMODULE)SETCLASSUINT(hwnd, GCW_HMODULE, h) 
#define GETCLASSICON(hwnd)              (HICON)GETCLASSUINT((hwnd), GCW_HICON)
#define SETCLASSICON(hwnd, h)           (HICON)SETCLASSUINT((hwnd), GCW_HICON, h)
#define GETCLASSSTYLE(hwnd)             GETCLASSUINT((hwnd), GCW_STYLE)            
#define SETCLASSSTYLE(hwnd, style)      SETCLASSUINT((hwnd), GCW_STYLE, style) 
#define GETHWNDINSTANCE(hwnd)           (HMODULE)GETWINDOWUINT((hwnd), GWW_HINSTANCE)
#define SETHWNDINSTANCE(hwnd, h)        (HMODULE)SETWINDOWUINT((hwnd), GWW_HINSTANCE, h)
#define GETHWNDPARENT(hwnd)             (HWND)GETWINDOWUINT((hwnd), GWW_HWNDPARENT)
#define SETHWNDPARENT(hwnd, h)          (HWND)SETWINDOWUINT((hwnd), GWW_HWNDPARENT, h)
#define GETWINDOWID(hwnd)               GETWINDOWUINT((hwnd), GWW_ID)            
#define SETWINDOWID(hwnd, id)           SETWINDOWUINT((hwnd), GWW_ID, id) 

/* USER API */

#define MDlgDirSelect(hDlg, lpstr, nLength, nIDListBox) \
            DlgDirSelect(hDlg, lpstr, nIDListBox)
            
#define MDlgDirSelectCOMBOBOX(hDlg, lpstr, nLength, nIDComboBox) \
            DlgDirSelectComboBox(hDlg, lpstr, nIDComboBox)

#define MMain(hInst, hPrevInst, lpCmdLine, nCmdShow) \
   INT PASCAL WinMain(HANDLE hInst, HANDLE hPrevInst, LPSTR lpCmdLine, \
   INT nCmdShow) {  \
   INT _argc;       \
   CHAR **_argv;    

/* USER MESSAGES: */

#define GET_WPARAM(wp, lp)                      (wp)
#define GET_LPARAM(wp, lp)                      (lp)

#define WM_CTLCOLORMSGBOX       0x0132
#define WM_CTLCOLOREDIT         0x0133
#define WM_CTLCOLORLISTBOX      0x0134
#define WM_CTLCOLORBTN          0x0135
#define WM_CTLCOLORDLG          0x0136
#define WM_CTLCOLORSCROLLBAR    0x0137
#define WM_CTLCOLORSTATIC       0x0138

#define GET_WM_ACTIVATE_STATE(wp, lp)               (wp)
#define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)          (BOOL)HIWORD(lp)
#define GET_WM_ACTIVATE_HWND(wp, lp)                (HWND)LOWORD(lp)
#define GET_WM_ACTIVATE_MPS(s, fmin, hwnd)   \
        (WPARAM)(s), MAKELONG(hwnd, fmin)
    
#define GET_WM_CHARTOITEM_CHAR(wp, lp)              (CHAR)(wp)
#define GET_WM_CHARTOITEM_POS(wp, lp)               HIWORD(lp)
#define GET_WM_CHARTOITEM_HWND(wp, lp)              (HWND)LOWORD(lp)
#define GET_WM_CHARTOITEM_MPS(ch, pos, hwnd) \
        (WPARAM)(ch), MAKELONG(hwnd, pos)
  
#define GET_WM_COMMAND_ID(wp, lp)                   (wp)
#define GET_WM_COMMAND_HWND(wp, lp)                 (HWND)LOWORD(lp)
#define GET_WM_COMMAND_CMD(wp, lp)                  HIWORD(lp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)    \
        (WPARAM)(id), MAKELONG(hwnd, cmd)
     
#define GET_WM_CTLCOLOR_HDC(wp, lp, msg)            (HDC)(wp)
#define GET_WM_CTLCOLOR_HWND(wp, lp, msg)           (HWND)LOWORD(lp)
#define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)           HIWORD(lp)
#define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type) \
        (WPARAM)(hdc), MAKELONG(hwnd, type)
     
#define GET_WM_MENUSELECT_CMD(wp, lp)               (wp)            
#define GET_WM_MENUSELECT_FLAGS(wp, lp)             LOWORD(lp)
#define GET_WM_MENUSELECT_HMENU(wp, lp)             (HMENU)HIWORD(lp)
#define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)  \
        (WPARAM)(cmd), MAKELONG(f, hmenu)
  
// Note: the following are for interpreting MDIclient to MDI child messages.
#define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (BOOL)(wp)         
#define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)HIWORD(lp)
#define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)LOWORD(lp)
// Note: the following is for sending to the MDI client window.
#define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA)\
        (WPARAM)(hwndA), 0
 
#define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW) 0, MAKELONG(hmenuF, hmenuW)
  
#define GET_WM_MENUCHAR_CHAR(wp, lp)                (CHAR)(wp)
#define GET_WM_MENUCHAR_HMENU(wp, lp)               (HMENU)LOWORD(lp)
#define GET_WM_MENUCHAR_FMENU(wp, lp)               (BOOL)HIWORD(lp)
#define GET_WM_MENUCHAR_MPS(ch, hmenu, f)    \
        (WPARAM)(ch), MAKELONG(hmenu, f)
    
#define GET_WM_PARENTNOTIFY_MSG(wp, lp)             (wp)
#define GET_WM_PARENTNOTIFY_ID(wp, lp)              HIWORD(lp)
#define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)       (HWND)LOWORD(lp)
#define GET_WM_PARENTNOTIFY_X(wp, lp)               (INT)LOWORD(lp)
#define GET_WM_PARENTNOTIFY_Y(wp, lp)               (INT)HIWORD(lp)
#define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd) \
        (WPARAM)(msg), MAKELONG(hwnd, id)
#define GET_WM_PARENTNOTIFY2_MPS(msg, x, y) \
        (WPARAM)(msg), MAKELONG(x, y)

#define GET_WM_VKEYTOITEM_CODE(wp, lp)              (wp)
#define GET_WM_VKEYTOITEM_ITEM(wp, lp)              (INT)HIWORD(lp)
#define GET_WM_VKEYTOITEM_HWND(wp, lp)              (HWND)LOWORD(lp)
#define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) \
        (WPARAM)(code), MAKELONG(hwnd, item)

#define GET_EM_SETSEL_START(wp, lp)                 LOWORD(lp)
#define GET_EM_SETSEL_END(wp, lp)                   HIWORD(lp)
#define GET_EM_SETSEL_MPS(iStart, iEnd) \
        0, MAKELONG(iStart, iEnd)
      
#define GET_EM_LINESCROLL_MPS(vert, horz)     \
        0, MAKELONG(vert, horz)
  
#define GET_WM_HSCROLL_CODE(wp, lp)                 (wp)
#define GET_WM_HSCROLL_POS(wp, lp)                  LOWORD(lp)
#define GET_WM_HSCROLL_HWND(wp, lp)                 (HWND)HIWORD(lp)
#define GET_WM_HSCROLL_MPS(code, pos, hwnd)    \
        (WPARAM)(code), MAKELONG(pos, hwnd)
     
#define GET_WM_VSCROLL_CODE(wp, lp)                 (wp)
#define GET_WM_VSCROLL_POS(wp, lp)                  LOWORD(lp)
#define GET_WM_VSCROLL_HWND(wp, lp)                 (HWND)HIWORD(lp)
#define GET_WM_VSCROLL_MPS(code, pos, hwnd)    \
        (WPARAM)(code), MAKELONG(pos, hwnd)
                                      
#define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)       (HWND)LOWORD(lp)
     
#define DDEFREE(msg, lp)

#define GET_WM_DDE_ACK_STATUS(wp, lp)               LOWORD(lp)
#define GET_WM_DDE_ACK_ITEM(wp, lp)                 (ATOM)HIWORD(lp)
#define MPostWM_DDE_ACK(hTo, hFrom, wStatus, aItem) \
        PostMessage(hTo, WM_DDE_ACK, (WPARAM)hFrom, MAKELONG(wStatus, aItem))

#define GET_WM_DDE_ADVISE_HOPTIONS(wp, lp)          (HANDLE)LOWORD(lp)
#define GET_WM_DDE_ADVISE_ITEM(wp, lp)              (ATOM)HIWORD(lp)
#define MPostWM_DDE_ADVISE(hTo, hFrom, hOptions, aItem) \
        PostMessage(hTo, WM_DDE_ADVISE, (WPARAM)hFrom, MAKELONG(hOptions, aItem))
  
#define GET_WM_DDE_DATA_HDATA(wp, lp)               (HANDLE)LOWORD(lp)
#define GET_WM_DDE_DATA_ITEM(wp, lp)                (ATOM)HIWORD(lp)
#define MPostWM_DDE_DATA(hTo, hFrom, hData, aItem) \
        PostMessage(hTo, WM_DDE_DATA, (WPARAM)hFrom, MAKELONG(hData, aItem))
  
#define GET_WM_DDE_EXECUTE_HDATA(wp, lp)            (HANDLE)HIWORD(lp)
#define MPostWM_DDE_EXECUTE(hTo, hFrom, hDataExec) \
        PostMessage(hTo, WM_DDE_EXECUTE, (WPARAM)hFrom, MAKELONG(0, hDataExec))
  
#define GET_WM_DDE_POKE_HDATA(wp, lp)               (HANDLE)LOWORD(lp)
#define GET_WM_DDE_POKE_ITEM(wp, lp)                (ATOM)HIWORD(lp)
#define MPostWM_DDE_POKE(hTo, hFrom, hData, aItem) \
        PostMessage(hTo, WM_DDE_POKE, (WPARAM)hFrom, MAKELONG(hData, aItem))
    
#define GET_WM_DDE_EXECACK_STATUS(wp, lp)           (WORD)LOWORD(lp)
#define GET_WM_DDE_EXECACK_HDATA(wp, lp)            (HANDLE)HIWORD(lp)
#define MPostWM_DDE_EXECACK(hTo, hFrom, hCommands, wStatus) \
        PostMessage(hTo, WM_DDE_ACK, (WPARAM)hFrom, MAKELONG(wStatus, hCommands))
    
#define GET_WM_DDE_REQUEST_FORMAT(wp, lp)           (ATOM)LOWORD(lp)
#define GET_WM_DDE_REQUEST_ITEM(wp, lp)             (ATOM)HIWORD(lp)
#define MPostWM_DDE_REQUEST(hTo, hFrom, fmt, aItem) \
        PostMessage(hTo, WM_DDE_REQUEST, (WPARAM)hFrom, MAKELONG(fmt, aItem))

#define GET_WM_DDE_UNADVISE_FORMAT(wp, lp)          (ATOM)LOWORD(lp)
#define GET_WM_DDE_UNADVISE_ITEM(wp, lp)            (ATOM)HIWORD(lp)
#define MPostWM_DDE_UNADVISE(hTo, hFrom, fmt, aItem) \
        PostMessage(hTo, WM_DDE_UNADVISE, (WPARAM)hFrom, MAKELONG(fmt, aItem))
        
#define MPostWM_DDE_TERMINATE(hTo, hFrom) \
        PostMessage(hTo, WM_DDE_TERMINATE, (WPARAM)hFrom, 0)

/*-----------------------------------GDI-------------------------------------*/

BOOL  FAR PASCAL     MGetAspectRatioFilter(HDC hdc, INT FAR * pcx, INT FAR * pcy);
BOOL  FAR PASCAL     MGetBitmapDimension(HANDLE hBitmap, INT FAR * pcx, INT FAR * pcy);
BOOL  FAR PASCAL     MGetBrushOrg(HDC hdc, INT FAR * px, INT FAR * py);
BOOL  FAR PASCAL     MGetCurrentPosition(HDC hdc, INT FAR * px, INT FAR * py);
BOOL  FAR PASCAL     MGetTextExtent(HDC hdc, LPSTR lpstr, INT cnt, INT FAR * pcx, INT FAR * pcy);
BOOL  FAR PASCAL     MGetViewportExt(HDC hdc, INT FAR * pcx, INT FAR * pcy);
BOOL  FAR PASCAL     MGetViewportOrg(HDC hdc, INT FAR * px, INT FAR * py);
BOOL  FAR PASCAL     MGetWindowExt(HDC hdc, INT FAR * pcx, INT FAR * pcy);
BOOL  FAR PASCAL     MGetWindowOrg(HDC hdc, INT FAR * px, INT FAR * py);

#define MCreateDiscardableBitmap CreateDiscardableBitmap
#define MMoveTo                  (VOID)MoveTo
#define MOffsetViewportOrg       (VOID)OffsetViewportOrg
#define MOffsetWindowOrg         (VOID)OffsetWindowOrg
#define MScaleViewportExt        (VOID)ScaleViewportExt
#define MScaleWindowExt          (VOID)ScaleWindowExt
#define MSetBitmapDimension      (VOID)SetBitmapDimension
#define MSetBrushOrg             (VOID)SetBrushOrg
#define MSetViewportExt          (VOID)SetViewportExt
#define MSetViewportOrg          (VOID)SetViewportOrg
#define MSetWindowExt            (VOID)SetWindowExt        
#define MSetWindowOrg            (VOID)SetWindowOrg
#define MUnrealizeObject         UnrealizeObject


/*-------------------------------------DEV-----------------------------------*/

DWORD FAR PASCAL     MDeviceCapabilities(LPSTR lpDriverName,
    LPSTR lpDeviceName, LPSTR lpPort, WORD2DWORD nIndex, LPSTR lpOutput,
    LPDEVMODE lpDevMode);
BOOL  FAR PASCAL     MDeviceMode(HWND hWnd, LPSTR lpDriverName,
    LPSTR lpDeviceName, LPSTR lpOutput);
WORD2DWORD FAR PASCAL    MExtDeviceMode(HWND hWnd,LPSTR lpDriverName,
    LPDEVMODE lpDevModeOutput, LPSTR lpDeviceName, LPSTR lpPort,
    LPDEVMODE lpDevModeInput, LPSTR lpProfile, WORD2DWORD flMode);
    
/*-----------------------------------KERNEL----------------------------------*/

HANDLE FAR PASCAL   MLoadLibrary(LPSTR lpszFilename);
BOOL FAR PASCAL MDeleteFile(LPSTR lpPathName);

#define DLLMEM_MOVEABLE         LMEM_MOVEABLE
#define DLLMEM_ZEROINIT         LMEM_ZEROINIT 
#define GETMAJORVERSION(x)      LOBYTE(x)
#define GETMINORVERSION(x)      HIBYTE(x)

#define MCATCHBUF               CATCHBUF
#define LPMCATCHBUF             LPCATCHBUF

/* FUNCTION MAPPINGS */

#define MLocalInit               LocalInit
#define MLockData(dummy)         LockData(dummy)
#define MUnlockData(dummy)       UnlockData(dummy)
#define MDllSharedAlloc          LocalAlloc
#define MDllSharedFlags          LocalFlags
#define MDllSharedFree           LocalFree
#define MDllSharedHandle         LocalHandle
#define MDllSharedLock           LocalLock 
#define MDllSharedRealloc        LocalReAlloc
#define MDllSharedSize           LocalSize
#define MDllSharedUnlock         LocalUnlock
#define MFreeDOSEnvironment(p)   TRUE
#define MGetCurrentTask          GetCurrentTask
#define MGetDOSEnvironment       GetDOSEnvironment
#define MGetDriveType            GetDriveType
#define MGetModuleUsage          GetModuleUsage
#define MGetTempDrive            GetTempDrive
#define MGetTempFileName         GetTempFileName
#define MGetWinFlags             GetWinFlags
#define MOpenComm                (HFILE)OpenComm
#define MSetCommState(fh, lpDCB) SetCommState(lpDCB)
#define MReadComm                ReadComm
#define MWriteComm               WriteComm
#define MCloseComm               CloseComm
#define MOpenFile                (HFILE)OpenFile
#define MThrow                   Throw
#define MCatch                   Catch
#define M_lclose                 _lclose
#define M_lcreat                 (HFILE)_lcreat
#define M_llseek                 _llseek
#define M_lopen                  (HFILE)_lopen
#define M_lread                  _lread
#define M_lwrite                 _lwrite
#define MGetMetaFileBits         GetMetaFileBits
#define MSetMetaFileBits         SetMetaFileBits
