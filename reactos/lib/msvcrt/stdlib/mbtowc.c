#include <msvcrt/stdlib.h>


//int mbtowc(wchar_t* wcDest, const char* mbConvert, size_t size)
int mbtowc(wchar_t *wchar, const char *mbchar, size_t count)
{
    *wchar = (wchar_t)*mbchar;
    return 1;

             // WideCharToMultiByte
             // MultiByteToWideChar
}
