#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/ctype.h>


#if 1

int mbtowc(wchar_t *dst, const char *str, size_t n)
{
//    printf("\t\t\tmbtowc(%p, %p, %d) called.\n", dst, str, n);

    if (n <= 0 || !str)
        return 0;

    *dst = *str;

    if (!*str)
        return 0;
    return 1;
}

#else

int mbtowc(wchar_t *dst, const char *str, size_t n)
{
    if (n <= 0 || !str)
        return 0;
    if (!MultiByteToWideChar(CP_ACP, 0, str, n, dst, (dst == NULL) ? 0 : 1)) {
        DWORD err = GetLastError();
        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            break;
        case ERROR_INVALID_FLAGS:
            break;
        case ERROR_INVALID_PARAMETER:
            break;
        case ERROR_NO_UNICODE_TRANSLATION:
            break;
        default:
            return 1;
        }
        return -1;
    }
    /* return the number of bytes from src that have been used */
    if (!*str)
        return 0;
    if (n >= 2 && isleadbyte(*str) && str[1])
        return 2;
    return 1;
}

#endif
