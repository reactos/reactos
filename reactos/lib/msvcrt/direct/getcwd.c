#include <windows.h>
#include <msvcrt/direct.h>
#include <msvcrt/stdlib.h>


/*
 * @implemented
 */
char *_getcwd(char* buffer, int maxlen)
{
    char *cwd;
    int len;

    if (buffer == NULL) {
        cwd = malloc(MAX_PATH);
        len = MAX_PATH;
    } else {
        cwd = buffer;
        len = maxlen;
    }
    if (GetCurrentDirectoryA(len, cwd) == 0) {
        return NULL;
    }
    return cwd;
}
