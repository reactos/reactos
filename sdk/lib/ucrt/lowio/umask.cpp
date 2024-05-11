/***
*umask.c - set file permission mask
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _umask() - sets file permission mask of current process*
*       affecting files created by creat, open, or sopen.
*
*******************************************************************************/

#include <corecrt_internal_lowio.h>
#include <sys\stat.h>

extern "C" int _umaskval = 0;

/***
*errno_t _umask(mode, poldmode) - set the file mode mask
*
*Purpose :
*    Works similiar to umask except it validates input params.
*
*
*******************************************************************************/

extern "C" errno_t __cdecl _umask_s(
    int  const mode,
    int* const old_mode
    )
{
    _VALIDATE_RETURN_ERRCODE(old_mode != nullptr, EINVAL);
    *old_mode = _umaskval;
    _VALIDATE_RETURN_ERRCODE((mode & ~(_S_IREAD | _S_IWRITE)) == 0, EINVAL);

    _umaskval = mode;
    return 0;
}

/***
*int _umask(mode) - set the file mode mask
*
*Purpose:
*       Sets the file-permission mask of the current process* which
*       modifies the permission setting of new files created by creat,
*       open, or sopen.
*
*Entry:
*       int mode - new file permission mask
*                  may contain _S_IWRITE, _S_IREAD, _S_IWRITE | _S_IREAD.
*                  The S_IREAD bit has no effect under Win32
*
*Exit:
*       returns the previous setting of the file permission mask.
*
*Exceptions:
*
*******************************************************************************/
extern "C" int __cdecl _umask(int const mode)
{
    // Silently ignore non-Windows modes:
    int const masked_mode = mode & (_S_IREAD | _S_IWRITE);

    int old_mode = 0;
    _umask_s(masked_mode, &old_mode);
    return old_mode;
}
