/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <windows.h>
#include <libc/file.h>

#undef putc
int putc(int c, FILE *fp)
{

	int r;

//	if(c =='\n' && __is_text_file(fp))
//		putc('\r',fp);


	if ( ((fp->_flag) & _IOSTRG) == _IOSTRG) {
		if(fp->_cnt>0)
		{
			fp->_cnt--;
			return((unsigned char)(*(fp->_ptr++)=(unsigned char)c));
		}
		return(_flsbuf(c,fp));
	}
	if ( !WriteFile(_get_osfhandle(fp->_file),&c,1,&r,NULL) ) 
		return -1;
	return r;

}
