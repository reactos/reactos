#include <windows.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


extern int BlockEnvToEnviron(); // defined in misc/dllmain.c

/*
 * @implemented
 */
int _putenv(const char* val)
{
    char* buffer;
    char* epos;
    int res;

    DPRINT("_putenv('%s')\n", val);
    epos = strchr(val, '=');
    if ( epos == NULL )
        return -1;
    buffer = (char*)malloc(epos - val + 1);
    if (buffer == NULL)
        return -1;
    strncpy(buffer, val, epos - val);
    buffer[epos - val] = 0;
    res = SetEnvironmentVariableA(buffer, epos+1);
    free(buffer);
    if (BlockEnvToEnviron())
        return 0;
    return res;
}
