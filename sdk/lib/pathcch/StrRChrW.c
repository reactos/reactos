
#include <windef.h>
#include <winbase.h>

// Implementation from string.c copied here in order not
// to depend on the whole file just for the PathCch library.
WCHAR * WINAPI StrRChrW(const WCHAR *str, const WCHAR *end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + lstrlenW(str);
    while (str < end)
    {
        if (*str == ch) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}
