/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

// putc can be a macro
#undef putc

int putc(int c, FILE *fp)
{

// valid stream macro should check that fp 
// is dword aligned
	if (!__validfp (fp)) {
		__set_errno(EINVAL);
		return -1;
	}
// check for write access on fp

	if ( !OPEN4WRITING(fp)  ) {
		__set_errno(EINVAL);
		return -1;
	}
	
	fp->_flag |= _IODIRTY;
	if (fp->_cnt > 0 ) {
		fp->_cnt--;
       		*(fp)->_ptr++ = (unsigned char)c;
		return (int)(unsigned char)c; 
	}
	else {
		return _flsbuf((unsigned char)c,fp);
	}
	return EOF;
}

wint_t putwc(wint_t c, FILE *fp)
{
// valid stream macro should check that fp 
// is dword aligned
	if (!__validfp (fp)) {
		__set_errno(EINVAL);
		return -1;
	}
// check for write access on fp

	if ( !OPEN4WRITING(fp)  ) {
		__set_errno(EINVAL);
		return -1;
	}
	// might check on multi bytes if text mode

	fp->_flag |= _IODIRTY;

        if (fp->_cnt > 0 ) {
                fp->_cnt-= sizeof(wchar_t);
 		*((wchar_t *)(fp->_ptr))++  = c;
		return (wint_t)c;
        }
        else
                return  _flswbuf(c,fp);

        return -1;


}
