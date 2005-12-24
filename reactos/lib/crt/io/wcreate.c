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
int _wcreat(const wchar_t* filename, int mode)
{
    DPRINT("_wcreat('%S', mode %x)\n", filename, mode);
    return _wopen(filename,_O_CREAT|_O_TRUNC,mode);
}
