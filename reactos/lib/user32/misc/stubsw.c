#include <windows.h>



WINBOOL
STDCALL
GetUserObjectInformationW(
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
SetUserObjectInformationW(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength)
{
	return FALSE;
}





HKL
STDCALL
LoadKeyboardLayoutW(
    LPCWSTR pwszKLID,
    UINT Flags)
{
	return 0;
}

WINBOOL
STDCALL
GetKeyboardLayoutNameW(
    LPWSTR pwszKLID)
{
	return FALSE;
}


HHOOK
STDCALL
SetWindowsHookW(
    int nFilterType,
    HOOKPROC lpfn)
{
	return 0;
}




int
STDCALL
DrawTextExW(HDC hDC, LPWSTR str, int i, LPRECT r, UINT u, LPDRAWTEXTPARAMS p) { return 0; }

WINBOOL
STDCALL
GrayStringW(
    HDC hDC,
    HBRUSH hBrush,
    GRAYSTRINGPROC lpOutputFunc,
    LPARAM lpData,
    int nCount,
    int X,
    int Y,
    int nWidth,
    int nHeight) { return 0; }

WINBOOL STDCALL DrawStateW(HDC hDC, HBRUSH hBrush, DRAWSTATEPROC d, LPARAM lParam, WPARAM wParam, int i, int j, int k, int l, UINT u) { return 0; }




HWND
STDCALL
FindWindowW(
    LPCWSTR lpClassName ,
    LPCWSTR lpWindowName) { return 0; }

HWND
STDCALL
FindWindowExW(HWND hWnd, HWND hWnd2, LPCWSTR str, LPCWSTR s) { return 0; }






int
STDCALL
GetClipboardFormatNameW(
    UINT format,
    LPWSTR lpszFormatName,
    int cchMaxCount) { return 0; }

WINBOOL
STDCALL
CharToOemW(
    LPCWSTR lpszSrc,
    LPSTR lpszDst) { return 0; }

WINBOOL
STDCALL
OemToCharW(
    LPCSTR lpszSrc,
    LPWSTR lpszDst) { return 0; }

WINBOOL
STDCALL
CharToOemBuffW(
    LPCWSTR lpszSrc,
    LPSTR lpszDst,
    DWORD cchDstLength) { return 0; }

WINBOOL
STDCALL
OemToCharBuffW(
    LPCSTR lpszSrc,
    LPWSTR lpszDst,
    DWORD cchDstLength) { return 0; }



int
STDCALL
GetKeyNameTextW(
    LONG lParam,
    LPWSTR lpString,
    int nSize
    ) { return 0; }

SHORT
STDCALL
VkKeyScanW(
    WCHAR ch) { return 0; }

SHORT
STDCALL VkKeyScanExW(
    WCHAR  ch,
    HKL   dwhkl) { return 0; }

UINT
STDCALL
MapVirtualKeyW(
    UINT uCode,
    UINT uMapType) { return 0; }

UINT
STDCALL
MapVirtualKeyExW(
    UINT uCode,
    UINT uMapType,
    HKL dwhkl) { return 0; }

HACCEL
STDCALL
LoadAcceleratorsW(
    HINSTANCE hInstance,
    LPCWSTR lpTableName) { return 0; }

HACCEL
STDCALL
CreateAcceleratorTableW(
    LPACCEL l, int i) { return 0; }

int
STDCALL
CopyAcceleratorTableW(
    HACCEL hAccelSrc,
    LPACCEL lpAccelDst,
    int cAccelEntries) { return 0; }

int
STDCALL
TranslateAcceleratorW(
    HWND hWnd,
    HACCEL hAccTable,
    LPMSG lpMsg) { return 0; }








HCURSOR
STDCALL
LoadCursorFromFileW(
    LPCWSTR    lpFileName) { return 0; }



HANDLE
STDCALL
LoadImageW(
    HINSTANCE hInst,
    LPCWSTR str,
    UINT u,
    int i,
    int j,
    UINT k) { return 0; }

int
STDCALL
LoadStringW(
    HINSTANCE hInstance,
    UINT uID,
    LPWSTR lpBuffer,
    int nBufferMax) { return 0; }


int
STDCALL
DlgDirListW(
    HWND hDlg,
    LPWSTR lpPathSpec,
    int nIDListBox,
    int nIDStaticPath,
    UINT uFileType) { return 0; }

WINBOOL
STDCALL
DlgDirSelectExW(
    HWND hDlg,
    LPWSTR lpString,
    int nCount,
    int nIDListBox) { return 0; }

int
STDCALL
DlgDirListComboBoxW(
    HWND hDlg,
    LPWSTR lpPathSpec,
    int nIDComboBox,
    int nIDStaticPath,
    UINT uFiletype) { return 0; }

WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
    HWND hDlg,
    LPWSTR lpString,
    int nCount,
    int nIDComboBox) { return 0; }

LRESULT
STDCALL
DefFrameProcW(
    HWND hWnd,
    HWND hWndMDIClient ,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) { return 0; }

LRESULT
STDCALL
DefMDIChildProcW(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) { return 0; }

HWND
STDCALL
CreateMDIWindowW(
    LPWSTR lpClassName,
    LPWSTR lpWindowName,
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
WinHelpW(
    HWND hWndMain,
    LPCWSTR lpszHelp,
    UINT uCommand,
    DWORD dwData
    ) { return 0; }

LONG
STDCALL
ChangeDisplaySettingsW(
    LPDEVMODE lpDevMode,
    DWORD dwFlags) { return 0; }

WINBOOL
STDCALL
EnumDisplaySettingsW(
    LPCWSTR lpszDeviceName,
    DWORD iModeNum,
    LPDEVMODE lpDevMode) { return 0; }




HSZ WINAPI
DdeCreateStringHandleW (DWORD dw, LPCWSTR str, int i) { return 0; }

UINT WINAPI
DdeInitializeW (DWORD *dw, CALLB c, DWORD x, DWORD y) { return 0; }

DWORD WINAPI
DdeQueryStringW (DWORD dw, HSZ h, LPCWSTR str, DWORD t, int i) { return 0; }