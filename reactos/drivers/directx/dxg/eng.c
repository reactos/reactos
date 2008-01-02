/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver for dxg implementation
 * FILE:             drivers/directx/dxg/main.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       30/12-2007   Magnus Olsen
 */


#include <dxg_int.h>


PDD_SURFACE_LOCAL
STDCALL
DxDdLockDirectDrawSurface(HANDLE hDdSurface)
{
   PEDD_SURFACE pEDDSurface = NULL;
   PDD_SURFACE_LOCAL pSurfacelcl = NULL;

   pEDDSurface = DdHmgLock(hDdSurface, ObjType_DDSURFACE_TYPE, FALSE);
   if (pEDDSurface != NULL)
   {
        pSurfacelcl = &pEDDSurface->ddsSurfaceLocal;
   }

   return pSurfacelcl;
}

BOOL
STDCALL
DxDdUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    BOOL retVal = FALSE;
    PEDD_SURFACE pEDDSurface  = NULL;

    if (pSurface)
    {
        pEDDSurface = (PEDD_SURFACE)( ((PBYTE)pSurface) - sizeof(DD_BASEOBJECT));
        InterlockedDecrement(&pEDDSurface->pobj.cExclusiveLock);
        retVal = TRUE;
    }

    return retVal;
}

HANDLE
STDCALL
DxDdCreateDirectDrawObject(HDC hdc)
{
    HANDLE hDirectDraw = NULL;
    DHPDEV  hPdev; // PGDIDEVICE
    ULONG  iDitherFormat;

    DC *pDC; = gpEngFuncs[DXENG_INDEX_DxEngLockDC]();

    if (pDC != NULL)
    {
        hPdev = gpEngFuncs[DXENG_INDEX_DxEngGetDCState](hdc, 3);

        if (hPdev != 0)
        {
            /*  Get DC display flag */
            if (gpEngFuncs[DXENG_INDEX_DxEngGetHdevData](hPdev, 12))
            {
                EDD_DEVLOCK(hdc, esi);

                if (!gpEngFuncs[DXENG_INDEX_DxEngGetHdevData](hPdev,19))
                {
                    CheckAgpHeaps( gpEngFuncs[DXENG_INDEX_DxEngGetHdevData](hPdev,7) );
                }

                iDitherFormat = gpEngFuncs[DXENG_INDEX_DxEngGetHdevData](hPdev,2);

                .text:00019FF1                 cmp     eax, 3
                .text:00019FF4                 jb      short loc_1A001

                hDirectDraw = hDdCreateDirectDrawLocal(hPdev);

                loc_1A001:
                if (hdc != NULL)
                {
                    gpEngFuncs[DXENG_INDEX_DxEngUnlockHdev](hdc);
                }
            }
        }

        gpEngFuncs[DXENG_INDEX_DxEngUnlockDC](pDC);
    }

    return hDirectDraw;

}


int __stdcall hDdCreateDirectDrawLocal(HDEV hDEV)
{
  int v1; // eax@1
  int v2; // edi@1
  int result; // eax@2
  int v4; // ebx@2
  struct HDD_OBJ__ *v5; // eax@5
  struct HDD_OBJ__ *v6; // esi@5
  int v7; // eax@1
  int v8; // eax@6
  int v9; // eax@6
  int v10; // eax@6
  int _EAX; // eax@6
  signed int _ECX; // ecx@6
  int v18; // [sp+8h] [bp-4h]@1

  v18 = 0;
  v7 = gpEngFuncs[DXENG_INDEX_DxEngGetHdevData](hPdev,7);
  v2 = v7;
  v1 = *(_DWORD *)(v7 + 0x30);
  if ( v1 )
  {
    result = EngAllocMem(1, 4 * v1, 1885627463);
    v4 = result;
    if ( !result )
      return result;
  }
  else
  {
    v4 = 0;
  }
  v5 = DdHmgAlloc(0x54u, 1u, 1u);
  v6 = v5;
  if ( v5 )
  {
    *((_DWORD *)v5 + 12) = *(_DWORD *)(v2 + 1448);
    *(_DWORD *)(v2 + 1448) = v5;
    *((_DWORD *)v5 + 8) = (char *)v5 + 28;
    *((_DWORD *)v5 + 7) = (char *)v5 + 28;
    *((_DWORD *)v5 + 9) = v2;
    *((_DWORD *)v5 + 4) = v2;
    v8 = PsGetCurrentThread();
    *((_DWORD *)v6 + 15) = PsGetThreadProcessId(v8);
    v9 = PsGetCurrentProcess();
    *((_DWORD *)v6 + 19) = 0;
    *((_DWORD *)v6 + 16) = v9;
    v10 = a1;
    *((_DWORD *)v6 + 17) = v4;
    (*(int (__cdecl **)(_DWORD))(gpEngFuncs + 108))(*(_DWORD *)v10);
    v18 = *(_DWORD *)v6;
    _EAX = (int)((char *)v6 + 8);
    _ECX = -1;
    __asm { lock xadd [eax], ecx }
    MapAllAgpHeaps(v6);
  }
  else
  {
    if ( v4 )
      EngFreeMem(v4);
  }
  return v18;
}







