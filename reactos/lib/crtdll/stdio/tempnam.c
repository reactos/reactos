#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>


char *_tempnam(const char *dir,const char *prefix )
{
    char *TempFileName = malloc(MAX_PATH);
    char *d;

    if ( dir == NULL )
        d = getenv("TMP");
    else 
        d = (char *)dir;

#ifdef _MSVCRT_LIB_    // TODO: check on difference?
    if (GetTempFileNameA(d, prefix, 1, TempFileName) == 0) {
#else// TODO: FIXME: review which is correct
    if (GetTempFileNameA(d, prefix, 0, TempFileName) == 0) {
#endif /*_MSVCRT_LIB_*/

        free(TempFileName);
        return NULL;
    }
        
    return TempFileName;
}
