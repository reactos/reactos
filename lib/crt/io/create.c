/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Created
 */

#include <precomp.h>

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
