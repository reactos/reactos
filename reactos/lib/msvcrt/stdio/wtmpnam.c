#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>


/*
 * @implemented
 */
wchar_t* _wtmpnam(wchar_t* s)
{
    wchar_t PathName[MAX_PATH];
    static wchar_t static_buf[MAX_PATH];

    GetTempPathW(MAX_PATH, PathName);
    GetTempFileNameW(PathName, L"ARI", 007, static_buf);
    wcscpy(s, static_buf);

    return s;
}
