#include <io.h>
#include <fcntl.h>

#define NDEBUG
#include <internal/debug.h>


/*
 * @implemented
 */
int _creat(const char* filename, int mode)
{
    DPRINT("_creat('%s', mode %x)\n", filename, mode);
    return _open(filename,_O_CREAT|_O_TRUNC,mode);
}
