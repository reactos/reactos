#include <windows.h>
#include <msvcrt/stdlib.h>

#if 1
/*
 * @unimplemented
 */
size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t count)
{
	size_t size;
    int i;

    printf("\nmbstowcs(%p, %p, %d) called.\n\n", wcstr, mbstr, count);

    if (count <= 0 || !mbstr)
        return 0;

    if (!*mbstr)
        return 0;


    if (wcstr == NULL) {
        // return required size for the converted string
        return strlen(mbstr); // TODO: fixme
    }
    for (size = 0, i = 0; i < count; size++) {
        int result;

////int mbtowc( wchar_t *wchar, const char *mbchar, size_t count )
////        result = mbtowc(wcstr + size, mbstr + i, count - i);
//        result = mbtowc(wcstr + size, mbstr + i, 1);

/////////////////////////////////////////
        if (mbstr[i] == 0) {
            result = 0;
        } else {
            wcstr[size] = mbstr[i];
            result = 1;
        }
/////////////////////////////////////////
        if (result == -1) {
            return -1;
        } else if (result == 0) {
            wcstr[size] = L'\0';
            break;
        } else {
            i += result;
        }

    }
	return size;
}

#else
#if 1

//int mbtowc(wchar_t *dst, const char *str, size_t n)
size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t count)
{
    size_t len;

    if (count <= 0 || !mbstr)
        return 0;
    len = MultiByteToWideChar(CP_ACP, 0, mbstr, count, wcstr, (wcstr == NULL) ? 0 : count);

    if (!len) {
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
    if (!*mbstr)
        return 0;
//    if (count >= 2 && isleadbyte(*mbstr) && mbstr[1])
//        return 2;
    return len;
}

#else

size_t mbstowcs(wchar_t* wcstr, const char* mbstr, size_t count)
{
	size_t size;
    int i;

    if (wcstr == NULL) {
        // return required size for the converted string
        return strlen(mbstr); // TODO: fixme
    }
    for (size = 0, i = 0; i < count; size++) {
        int result;

//int mbtowc( wchar_t *wchar, const char *mbchar, size_t count )
//        result = mbtowc(wcstr + size, mbstr + i, count - i);
        result = mbtowc(wcstr + size, mbstr + i, 1);
        if (result == -1) {
            return -1;
        } else if (result == 0) {
            wcstr[size] = L'\0';
            break;
        } else {
            i += result;
        }

    }
	return (size_t)size;
}

#endif
#endif
