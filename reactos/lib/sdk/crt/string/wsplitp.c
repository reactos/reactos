
#define _UNICODE
#define UNICODE

#include <tchar.h>

#include "splitp.c"

/******************************************************************
 *		_wsplitpath_s (MSVCRT.@)
 *
 * Secure version of _wsplitpath
 */
int _wsplitpath_s(const wchar_t* inpath,
                  wchar_t* drive, size_t sz_drive,
                  wchar_t* dir, size_t sz_dir,
                  wchar_t* fname, size_t sz_fname,
                  wchar_t* ext, size_t sz_ext)
{
    const wchar_t *p, *end;

    if (!inpath) return EINVAL;
    if (!drive && sz_drive) return EINVAL;
    if (drive && !sz_drive) return EINVAL;
    if (!dir && sz_dir) return EINVAL;
    if (dir && !sz_dir) return EINVAL;
    if (!fname && sz_fname) return EINVAL;
    if (fname && !sz_fname) return EINVAL;
    if (!ext && sz_ext) return EINVAL;
    if (ext && !sz_ext) return EINVAL;

    if (inpath[0] && inpath[1] == ':')
    {
        if (drive)
        {
            if (sz_drive <= 2) goto do_error;
            drive[0] = inpath[0];
            drive[1] = inpath[1];
            drive[2] = 0;
        }
        inpath += 2;
    }
    else if (drive) drive[0] = '\0';

    /* look for end of directory part */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '/' || *p == '\\') end = p + 1;

    if (end)  /* got a directory */
    {
        if (dir)
        {
            if (sz_dir <= end - inpath) goto do_error;
            memcpy( dir, inpath, (end - inpath) * sizeof(wchar_t) );
            dir[end - inpath] = 0;
        }
        inpath = end;
    }
    else if (dir) dir[0] = 0;

    /* look for extension: what's after the last dot */
    end = NULL;
    for (p = inpath; *p; p++) if (*p == '.') end = p;

    if (!end) end = p; /* there's no extension */

    if (fname)
    {
        if (sz_fname <= end - inpath) goto do_error;
        memcpy( fname, inpath, (end - inpath) * sizeof(wchar_t) );
        fname[end - inpath] = 0;
    }
    if (ext)
    {
        if (sz_ext <= strlenW(end)) goto do_error;
        strcpyW( ext, end );
    }
    return 0;
do_error:
    if (drive)  drive[0] = '\0';
    if (dir)    dir[0] = '\0';
    if (fname)  fname[0]= '\0';
    if (ext)    ext[0]= '\0';
    return ERANGE;
}
