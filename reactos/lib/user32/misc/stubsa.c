#include <windows.h>

WINBOOL
STDCALL
GetUserObjectInformationA(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded)
{
	return FALSE;
}
 
WINBOOL
STDCALL
SetUserObjectInformationA(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength)
{
	return FALSE;
}
 



HKL
STDCALL
LoadKeyboardLayoutA(
    LPCSTR pwszKLID,
    UINT Flags)
{
	return 0;
}
 
WINBOOL
STDCALL
GetKeyboardLayoutNameA(
    LPSTR pwszKLID)
{
	return FALSE;
}

HHOOK
STDCALL
SetWindowsHookA(
    int nFilterType,
    HOOKPROC lpfn)
{
	return 0; 
}



int
STDCALL
DrawTextExA(HDC HDC, LPSTR str, int i, LPRECT r, UINT u, LPDRAWTEXTPARAMS x) { return 0; }
 
WINBOOL
STDCALL
GrayStringA(
    HDC hDC,
    HBRUSH hBrush,
    GRAYSTRINGPROC lpOutputFunc,
    LPARAM lpData,
    int nCount,
    int X,
    int Y,
    int nWidth,
    int nHeight) { return 0; }
 
WINBOOL
STDCALL
DrawStateA(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC p, LPARAM lParam, WPARAM wParam, int i, int j, int k, int l, UINT u) { return 0; }


 



 
HWND
STDCALL
FindWindowA(
    LPCSTR lpClassName ,
    LPCSTR lpWindowName) { return 0; }
 
HWND
STDCALL
FindWindowExA(HWND hWnd, HWND hWnd2, LPCSTR str, LPCSTR s) { return 0; }
 

 

 
int
STDCALL
GetClipboardFormatNameA(
    UINT format,
    LPSTR lpszFormatName,
    int cchMaxCount) { return 0; }
 
WINBOOL
STDCALL
CharToOemA(
    LPCSTR lpszSrc,
    LPSTR lpszDst) { return 0; }
 
WINBOOL
STDCALL
OemToCharA(
    LPCSTR lpszSrc,
    LPSTR lpszDst) { return 0; }
 
WINBOOL
STDCALL
CharToOemBuffA(
    LPCSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength) { return 0; }
 
WINBOOL
STDCALL
OemToCharBuffA(
    LPCSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength) { return 0; }
 


 
int
STDCALL
GetKeyNameTextA(
    LONG lParam,
    LPSTR lpString,
    int nSize
    ) { return 0; }
 
SHORT
STDCALL
VkKeyScanA(
    CHAR ch) { return 0; }
 
SHORT
STDCALL VkKeyScanExA(
    CHAR  ch,
    HKL   dwhkl) { return 0; }
 
UINT
STDCALL
MapVirtualKeyA(
    UINT uCode,
    UINT uMapType) { return 0; }
 
UINT
STDCALL
MapVirtualKeyExA(
    UINT uCode,
    UINT uMapType,
    HKL dwhkl) { return 0; }
 
HACCEL
STDCALL
LoadAcceleratorsA(
    HINSTANCE hInstance,
    LPCSTR lpTableName) { return 0; }
 
HACCEL
STDCALL
CreateAcceleratorTableA(
    LPACCEL l, int i) { return 0; }
 
int
STDCALL
CopyAcceleratorTableA(
    HACCEL hAccelSrc,
    LPACCEL lpAccelDst,
    int cAccelEntries) { return 0; }
 
int
STDCALL
TranslateAcceleratorA(
    HWND hWnd,
    HACCEL hAccTable,
    LPMSG lpMsg) { return 0; }
 


 

HCURSOR
STDCALL
LoadCursorFromFileA(
    LPCSTR    lpFileName) { return 0; }
 

HANDLE
STDCALL
LoadImageA(
    HINSTANCE hInst,
    LPCSTR str,
    UINT u,
    int i,
    int j,
    UINT k) { return 0; }
 
int
STDCALL
LoadStringA(
    HINSTANCE hInstance,
    UINT uID,
    LPSTR lpBuffer,
    int nBufferMax) { return 0; }
 

int
STDCALL
DlgDirListA(
    HWND hDlg,
    LPSTR lpPathSpec,
    int nIDListBox,
    int nIDStaticPath,
    UINT uFileType) { return 0; }
 
WINBOOL
STDCALL
DlgDirSelectExA(
    HWND hDlg,
    LPSTR lpString,
    int nCount,
    int nIDListBox) { return 0; }
 
int
STDCALL
DlgDirListComboBoxA(
    HWND hDlg,
    LPSTR lpPathSpec,
    int nIDComboBox,
    int nIDStaticPath,
    UINT uFiletype) { return 0; }
 
WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
    HWND hDlg,
    LPSTR lpString,
    int nCount,
    int nIDComboBox) { return 0; }
 
LRESULT
STDCALL
DefFrameProcA(
    HWND hWnd,
    HWND hWndMDIClient ,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) { return 0; }
 
LRESULT
STDCALL
DefMDIChildProcA(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) { return 0; }
 
HWND
STDCALL
CreateMDIWindowA(
    LPSTR lpClassName,
    LPSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HINSTANCE hInstance,
    LPARAM lParam
    ) { return 0; }
 
WINBOOL
STDCALL
WinHelpA(
    HWND hWndMain,
    LPCSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    ) { return 0; }
 
LONG
STDCALL
ChangeDisplaySettingsA(
    LPDEVMODE lpDevMode,
    DWORD dwFlags) { return 0; }
 
WINBOOL
STDCALL
EnumDisplaySettingsA(
    LPCSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODE lpDevMode) { return 0; }
 




HSZ WINAPI
DdeCreateStringHandleA (DWORD dw, LPSTR str, int i) { return 0; }

UINT WINAPI
DdeInitializeA (DWORD *dw, CALLB c, DWORD x, DWORD y) { return 0; }

DWORD WINAPI
DdeQueryStringA (DWORD dw, HSZ h, LPSTR str, DWORD t, int i) { return 0; }