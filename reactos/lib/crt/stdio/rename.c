#include "precomp.h"
#include <stdio.h>
#include <io.h>
#include <tchar.h>

/*
 * @implemented
 */
int _trename(const _TCHAR* old_, const _TCHAR* new_)
{
    if (old_ == NULL || new_ == NULL)
        return -1;

    if (!MoveFile(old_, new_))
        return -1;

    return 0;
}
