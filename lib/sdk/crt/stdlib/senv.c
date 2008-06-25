/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#ifdef _UNICODE
   #define sT "S"
#else
   #define sT "s"
#endif

#define MK_STR(s) #s


/*
 * @implemented
 */
void _tsearchenv(const _TCHAR* file,const _TCHAR* var,_TCHAR* path)
{
    _TCHAR* env = _tgetenv(var);
    _TCHAR* x;
    _TCHAR* y;
    _TCHAR* FilePart;

    TRACE(MK_STR(_tsearchenv)"()\n");

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
