#include <io.h>
#include <fcntl.h>

#define NDEBUG
#include <internal/debug.h>


/*
 * @implemented
 */
int _wcreat(const wchar_t* filename, int mode)
{
    DPRINT("_wcreat('%S', mode %x)\n", filename, mode);
    return _wopen(filename,_O_CREAT|_O_TRUNC,mode);
}
