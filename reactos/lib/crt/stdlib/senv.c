#include "precomp.h"
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#define NDEBUG
#include <internal/msvcrtdbg.h>


/*
 * @implemented
 */
void _tsearchenv(const _TCHAR* file,const _TCHAR* var,_TCHAR* path)
{
    _TCHAR* env = _tgetenv(var);
    _TCHAR* x;
    _TCHAR* y;
    _TCHAR* FilePart;

    DPRINT(#_tsearchenv"()\n");

    x = _tcschr(env,'=');
    if ( x != NULL ) {
        *x = 0;
        x++;
    }
    y = _tcschr(env,';');
    while ( y != NULL ) {
        *y = 0;
        if ( SearchPath(x,file,NULL,MAX_PATH,path,&FilePart) > 0 ) {
            return;
        }
        x = y+1;
        y = _tcschr(env,';');
    }
    return;
}
