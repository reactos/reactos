#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>


/*
 * @implemented
 */
int _wrename(const wchar_t* old_, const wchar_t* new_)
{
    if (old_ == NULL || new_ == NULL)
        return -1;

    if (!MoveFileW(old_, new_))
        return -1;

    return 0;
}
