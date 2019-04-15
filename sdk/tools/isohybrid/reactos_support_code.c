#include "reactos_support_code.h"

void
isohybrid_error(int eval, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "isohybrid: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(eval);
}

void
isohybrid_warning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "isohybrid: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#ifdef _WIN32
int
fsync(int fd)
{
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE)
        return 1;

    return !FlushFileBuffers(hFile);
}

int
getppid(void)
{
    // Just return any nonzero value under Windows to enable isohybrid's usage
    // as a part of srand initialization.
    return 1;
}
#endif
