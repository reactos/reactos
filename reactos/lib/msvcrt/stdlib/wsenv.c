#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


void _wsearchenv(const wchar_t* file,const wchar_t* var,wchar_t* path)
{
    wchar_t* env = _wgetenv(var);
    wchar_t* x;
    wchar_t* y;
    wchar_t* FilePart;

    DPRINT("_wsearchenv()\n");
    x = wcschr(env,L'=');
    if ( x != NULL ) {
        *x = 0;
        x++;
    }
    y = wcschr(env,L';');
    while ( y != NULL ) {
        *y = 0;
        if ( SearchPathW(x,file,NULL,MAX_PATH,path,&FilePart) > 0 ) {
            return;
        }
        x = y+1;
        y = wcschr(env,L';');
    }
    return;
}
