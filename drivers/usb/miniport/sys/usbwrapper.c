/*
 * Interface calls to BIOS
 *
 * 2003-06-21 Georg Acher (georg@acher.org)
 *
 */

#include "boot.h"
#include <stdarg.h>
#include "video.h"

/*------------------------------------------------------------------------*/ 
// Output window for USB messages
int usb_curs_x=0;
int usb_curs_y=0;

void zxprintf(char* fmt, ...)
{
        va_list ap;
        char buffer[1024];
	int tmp_x, tmp_y;
	tmp_x=VIDEO_CURSOR_POSX;
	tmp_y=VIDEO_CURSOR_POSY;
	
	VIDEO_CURSOR_POSX=usb_curs_x;
	VIDEO_CURSOR_POSY=usb_curs_y;
	       
	if ((VIDEO_CURSOR_POSY==0) || (VIDEO_CURSOR_POSY > (vmode.height -16)))
	{
		BootVideoClearScreen(&jpegBackdrop, 3*vmode.height/4, 
				     vmode.height);
		VIDEO_CURSOR_POSY=3*vmode.height/4;
	}

        va_start(ap, fmt);
        vsprintf(buffer,fmt,ap);
        //printk(buffer);
        va_end(ap);

	usb_curs_x=VIDEO_CURSOR_POSX;
	usb_curs_y=VIDEO_CURSOR_POSY;
	VIDEO_CURSOR_POSX=tmp_x;
	VIDEO_CURSOR_POSY=tmp_y;
}
/*------------------------------------------------------------------------*/ 
int zxsnprintf(char *buffer, size_t s, char* fmt, ...)
{
        va_list ap;
        int x;
        va_start(ap, fmt);
        x=vsprintf(buffer,fmt,ap);
        va_end(ap);
        return x;
}
/*------------------------------------------------------------------------*/ 
int zxsprintf(char *buffer, char* fmt, ...)
{
        va_list ap;
        int x;
        va_start(ap, fmt);
        x=vsprintf(buffer,fmt,ap);
        va_end(ap);
        return x;
}
/*------------------------------------------------------------------------*/ 
