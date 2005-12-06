/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Added license header
 */

#include <precomp.h>

/*
 * @implemented
 */
wchar_t* _wcsdup(const wchar_t* ptr)
{
    wchar_t* dup;

    dup = malloc((wcslen(ptr) + 1) * sizeof(wchar_t));
    if (dup == NULL) {
        __set_errno(ENOMEM);
        return NULL;
    }
    wcscpy(dup, ptr);
    return dup;
}
