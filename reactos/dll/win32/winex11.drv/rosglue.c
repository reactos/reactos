#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <share.h>
#include <io.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11glue);

int usleep (unsigned int useconds)
{
    //return _usleep(useconds);
    UNIMPLEMENTED;
    return 0;
}

WCHAR *wine_get_dos_file_name(char *path)
{
    ERR("wine_get_dos_file_name(%s) unimplemented!\n", path);
    return NULL;
}

/***********************************************************************
 *		X11DRV_AttachEventQueueToTablet (X11DRV.@)
 */
int CDECL X11DRV_AttachEventQueueToTablet(HWND hOwner)
{
    return 0;
}

/***********************************************************************
 *		X11DRV_GetCurrentPacket (X11DRV.@)
 */
int CDECL X11DRV_GetCurrentPacket(void *packet)
{
    //*packet = gMsgPacket;
    return 1;
}

/***********************************************************************
 *             X11DRV_LoadTabletInfo (X11DRV.@)
 */
void CDECL X11DRV_LoadTabletInfo(HWND hwnddefault)
{
}

/***********************************************************************
 *		X11DRV_WTInfoW (X11DRV.@)
 */
UINT CDECL X11DRV_WTInfoW(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
    return FALSE;
}

INT X11DRV_DCICommand(INT cbInput, const DCICMD *lpCmd, LPVOID lpOutData)
{
  TRACE("(%d,(%d,%d,%d),%p)\n", cbInput, lpCmd->dwCommand,
	lpCmd->dwParam1, lpCmd->dwParam2, lpOutData);

  return FALSE;
}

void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr, LPVIDMEM fb_mem)
{
}


/* CRT compatibility HACKS */

struct _stat;
int CDECL fstat(int fd, struct _stat* buf)
{
    int CDECL _fstat(int fd, struct _stat* buf);
    return _fstat(fd, buf);
}

char *__cdecl strdup(const char *_Src)
{
    return _strdup(_Src);
}

int __cdecl open(const char *path,int flags,...)
{
  va_list ap;

  if (flags & O_CREAT)
  {
    int pmode;
    va_start(ap, flags);
    pmode = va_arg(ap, int);
    va_end(ap);
    return _sopen( path, flags, _SH_DENYNO, pmode );
  }
  else
    return _sopen( path, flags, _SH_DENYNO);
}

int __cdecl close(int _FileHandle)
{
    return _close(_FileHandle);
}

int __cdecl read(int _FileHandle,void *_DstBuf,unsigned int _MaxCharCount)
{
    return _read(_FileHandle, _DstBuf, _MaxCharCount);
}

int __cdecl unlink(const char *_Filename)
{
    return _unlink(_Filename);
}

int __cdecl access(const char *_Filename,int _AccessMode)
{
    return _access(_Filename, _AccessMode);
}


