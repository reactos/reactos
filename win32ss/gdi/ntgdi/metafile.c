/*
 * PROJECT:         ReactOS Win32k Subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32k/objects/metafile.c
 * PURPOSE:         Metafile Implementation
 * PROGRAMMERS:     ...
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* System Service Calls ******************************************************/

/*
 * @unimplemented
 */
LONG
APIENTRY
NtGdiConvertMetafileRect(IN HDC hDC,
                         IN OUT PRECTL pRect)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
HDC
APIENTRY
NtGdiCreateMetafileDC(IN HDC hdc)
{
   PDC pDc;
   HDC ret = NULL;

   if (hdc)
   {
      pDc = DC_LockDc(hdc);
      if (pDc)
      { // Not sure this is right for getting the HDEV handle, maybe Timo could help or just if'ed it out.
         ret = IntGdiCreateDisplayDC(pDc->ppdev->BaseObject.hHmgr, DC_TYPE_INFO, TRUE);
         DC_UnlockDc(pDc);
      }
   }
   else
   {
       ret = UserGetDesktopDC(DC_TYPE_INFO, TRUE, FALSE);
   }
   return ret;
}

/*
 * @unimplemented
 */
HANDLE
APIENTRY
NtGdiCreateServerMetaFile(IN DWORD iType,
                          IN ULONG cjData,
                          IN PBYTE pjData,
                          IN DWORD mm,
                          IN DWORD xExt,
                          IN DWORD yExt)
{
    UNIMPLEMENTED;
    return NULL;
}
 
/*
 * @unimplemented
 */
ULONG
APIENTRY
NtGdiGetServerMetaFileBits(IN HANDLE hmo,
                           IN ULONG cjData,
                           OUT OPTIONAL PBYTE pjData,
                           OUT PDWORD piType,
                           OUT PDWORD pmm,
                           OUT PDWORD pxExt,
                           OUT PDWORD pyExt)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
