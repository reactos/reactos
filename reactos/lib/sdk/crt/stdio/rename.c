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
