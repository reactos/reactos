/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/wchar.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>

int putc(int c, FILE *fp)
{

	if ( c == 0 )
		c = ' '; 
// valid stream macro should check that fp 
// is dword aligned
	if (  fp == NULL ) {
		__set_errno(EINVAL);
		return -1;
	}
// check for write access on fp

	//if ( !WRITE_STREAM(fp)  ) {
	//	__set_errno(EINVAL);
	//	return -1;
	//}
	
	fp->_flag |= _IODIRTY;
	if (fp->_cnt > 0 ) {
		fp->_cnt--;
       		*(fp)->_ptr++ = (char)c;
		return (int)c; 
	}
	else {
		return _flsbuf(c,fp);
	}


	return -1;
}

wint_t putwc(wchar_t c, FILE *fp)
{
	// might check on multi bytes if text mode
 
        if (fp->_cnt > 0 ) {
                fp->_cnt-= sizeof(wchar_t);
 		*((wchar_t *)(fp->_ptr))++  = c;
		return (wint_t)c;
        }
        else
                return  _flswbuf(c,fp);

        return -1;


}
