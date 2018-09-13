// bjw Win16 <-> Win32 compatibility macros

#include <direct.h>                             // dwb KtoA
#include <stdlib.h>                             // dwb KtoA
#include <io.h>                                 // dwb KtoA
#include <memory.h>                             // dwb KtoA
#include <string.h>                             // dwb KtoA

#ifdef OLDCODE
typedef UINT WPARAM;                            // dwb KtoA

#define MAINENTRY                               APIENTRY
// bjw Win16 <-> Win32 compatibility macros
#define MAINENTRY                               APIENTRY
#define huge                                    FAR
#define WINAPI                                  APIENTRY

#define GET_WM_COMMAND_ID(wParam, lParam)       ((WORD) (wParam))
#define GET_WM_COMMAND_HWND(wParam, lParam)     ((HWND) lParam)
#define GET_WM_COMMAND_CMD(wParam, lParam)      ((WORD) HIWORD(wParam))
#define PACK_WM_COMMAND_WPARAM(w1, w2)          ((UINT) MAKELONG(w1, 2))
#define MAKE_MOUSE_POINT(l)                     (*((POINTS FAR *)&(l)))
#define MDI_CREATE(hwnd, long)                  ((HWND) SendMessage(hwnd, WM_MDICREATE, (UINT) 0, (LONG)long))
#define GET_MDI_ACTIVE(hwnd, w, l)              ((HWND) SendMessage(hwnd, WM_MDIGETACTIVE, (UINT) 0, 0L))
#define MDI_SETMENU(hwnd, a, b)                 ((HMENU) SendMessage(hwnd, WM_MDISETMENU, (UINT) a, (LONG) b))
#define MDI_TILE(hwnd)                          ((LONG) SendMessage(hwnd, WM_MDITILE, (UINT) 0, 0L))
#define GET_WM_CTLCOLOR_HWND(w, l)              ((HWND)  l)
#define DLGDIRSELECT(hwnd, lpsz, ncnt, nid)     ((BOOL)DlgDirSelectEx(hwnd, lpsz, ncnt, nid))
#define DIRSELECTCB(hwnd, lpsz, ncnt, nid)      ((BOOL)DlgDirSelectComboBoxEx(hwnd, lpsz, ncnt, nid))
#define GET_HMODULE(hwnd)                                      ((HMODULE) GetWindowLong(hwnd, GWL_HINSTANCE))
#define GET_HWNDPARENT(hwnd)                    ((HWND) GetWindowLong(hwnd, GWL_HWNDPARENT))
#define GET_ID(hwnd)                            ((UINT) GetWindowLong(hwnd, GWL_ID))
#define GET_WW(hwnd, ndx)                       ((UINT) GetWindowLong(hwnd, ndx))
#define SET_WW(hwnd, ndx, val)                  ((UINT) SetWindowLong(hwnd, ndx, (LONG) val));
#define MOVE_TO(hdc, x, y)                      ((BOOL) MoveToEx(hdc, x, y, NULL))
#define MoveTo(hdc, x, y)                       ((BOOL) MoveToEx(hdc, x, y, NULL))
#define GET_CLASS_HCURSOR(hwnd)                 ((HCURSOR) GetClassLong(hwnd, GCL_HCURSOR))
#define GET_CLASS_HICON(hwnd)                   ((HCURSOR) GetClassLong(hwnd, GCL_HICON))
#define SET_CLASS_HCURSOR(hwnd, h)              ((HCURSOR) SetClassLong(hwnd, GCL_HCURSOR, (LONG) h))
#define SET_CLASS_HICON(hwnd, h)                ((HCURSOR) SetClassLong(hwnd, GCL_HICON, (LONG) h))

#define GET_WM_MENUSELECT_ID(w, l)              ((WORD) w)
#define GET_WM_MENUSELECT_CMD(w, l)             ((WORD) HIWORD(l))
#define GET_WM_MENUSELECT_HMENU(w, l)           ((HMENU) l)
#define GET_WM_ACTIVATE_HWND(w, l)              ((HWND) l)
#define SET_BRUSH_ORG(hdc, x, y)                ((BOOL) SetBrushOrg(hdc, x, y, NULL))

#define GET_WM_HSCROLL_POSITION(w, l)           ((WORD) HIWORD(w))  // dwb KtoA  for 16-bit: #define GET_WM_HSCROLL_POSITION(w, l)           ((WORD) LOWORD(l)) 
#define GET_WM_VSCROLL_POSITION(w, l)           ((WORD) HIWORD(w))  // dwb KtoA  for 16-bit: #define GET_WM_VSCROLL_POSITION(w, l)           ((WORD) LOWORD(l)) 

#define SET_WINDOW_EXT(hdc, x, y)               ((BOOL) SetWindowExtEx(hdc, x, y, NULL))
#define SetWindowExt(hdc, x, y)                 ((BOOL) SetWindowExtEx(hdc, x, y, NULL))
#define SET_WINDOW_ORG(hdc, x, y)               ((BOOL) SetWindowOrgEx(hdc, x, y, NULL))
#define SetWindowOrg(hdc, x, y)                 ((BOOL) SetWindowOrgEx(hdc, x, y, NULL))
#define SET_VIEWPORT_EXT(hdc, x, y)             ((BOOL) SetViewportExtEx(hdc, x, y, NULL))
#define SetViewportExt(hdc, x, y)               ((BOOL) SetViewportExtEx(hdc, x, y, NULL))
#define SET_VIEWPORT_ORG(hdc, x, y)             ((BOOL) SetViewportOrgEx(hdc, x, y, NULL))
#define SetViewportOrg(hdc, x, y)               ((BOOL) SetViewportOrgEx(hdc, x, y, NULL))

LONG FSE_filelength(int hFile);

#define ODS(s)  OutputDebugString(s); OutputDebugString("\n");
#endif // oldcode
// x86 asembly replacements
#define lmovmem(s, d, l)                        memcpy(d, s, l)
#define lsetmem(s, b, l)                        memset(s, b, l)



#ifdef OLDCODE
//--------------------------------------------------------------------------
//-- obsolete functions
//--------------------------------------------------------------------------
#define  CloseSound()
#define  CountVoiceNotes(n)                     ((int) 0)
#define  GetWinFlags()                          ((DWORD) 0)
#define  OpenSound()                            ((int) 0)
#define  SetVoiceAccent(n1,n2,n3,n4,n5)         ((int) 0)
#define  SetVoiceNote(n1, n2, n3, n4)           ((int) 0)
#define  StartSound()                           ((int) 0)
#define  StopSound()                            ((int) 0)
#define  LockData()                             (0)
#define  UnlockData()                           (0)
#define  AccessResource()                       (0)
#define  SetResourceHandler()                   (0)


//--------------------------------------------------------------------------
//-- comm functions that need to be replaced by Windows NT comm functions
//--------------------------------------------------------------------------
#define CloseComm()                             (0)
#define FlushComm()                             (0)
#define GetCommError()                          (0)
#define OpenComm()                              (0)
#define ReadComm()                              (0)
#define SetCommEventMask()                      (0)
#define WriteComm()                             (0)


//--------------------------------------------------------------------------
//-- FSE functions not portable to NT
//--------------------------------------------------------------------------
#endif
