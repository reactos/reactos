/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/io.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


int _unlink(const char* filename)
{
    int result = 0;
    DPRINT("_unlink('%s')\n", filename);
    if (!DeleteFileA(filename))
        result = -1;
    DPRINT("%d\n", result);
    return result;
}
