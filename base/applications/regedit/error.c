#include <regedit.h>
void ErrorMessageBox(HWND hWnd, LPCTSTR title, DWORD code)
{
    LPTSTR lpMsgBuf;
    DWORD status;
    static const TCHAR fallback[] = TEXT("Error displaying error message.\n");
    status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, code, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (!status)
        lpMsgBuf = (LPTSTR)fallback;
    MessageBox(hWnd, lpMsgBuf, title, MB_OK | MB_ICONERROR);
    if (lpMsgBuf != fallback)
        LocalFree(lpMsgBuf);
}
