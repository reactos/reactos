#include "shellmenu.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern "C"
BOOL Shell_GetShellProgram(LPWSTR pszProgram, SIZE_T cchProgramMax)
{
    if (!cchProgramMax)
        return FALSE;

    LONG err;
    HKEY hKey;
    err = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                        0, KEY_READ, &hKey);
    if (err)
    {
        ERR("err: %ld\n", err);
        StringCchCopyW(pszProgram, cchProgramMax, L"explorer.exe");
        return FALSE;
    }

    DWORD cbData = cchProgramMax * sizeof(WCHAR);
    err = RegQueryValueExW(hKey, L"Shell", NULL, NULL, (LPBYTE)pszProgram, &cbData);
    if (err)
    {
        StringCchCopyW(pszProgram, cchProgramMax, L"explorer.exe");
        ERR("err: %ld\n", err);
    }

    RegCloseKey(hKey);

    return !err;
}
