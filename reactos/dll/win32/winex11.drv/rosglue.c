#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
    return 0;
}

//void xinerama_init( unsigned int width, unsigned int height )
//{
//}

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

struct _stat;
int CDECL fstat(int fd, struct _stat* buf)
{
    int CDECL _fstat(int fd, struct _stat* buf);
    return _fstat(fd, buf);
}
