#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


void _searchenv(const char* file,const char* var,char* path)
{
    char* env = getenv(var);
    char* x;
    char* y;
    char* FilePart;

    DPRINT("_searchenv()\n");

    x = strchr(env,'=');
    if ( x != NULL ) {
        *x = 0;
        x++;
    }
    y = strchr(env,';');
    while ( y != NULL ) {
        *y = 0;
        if ( SearchPathA(x,file,NULL,MAX_PATH,path,&FilePart) > 0 ) {
            return;
        }
        x = y+1;
        y = strchr(env,';');
    }
    return;
}
