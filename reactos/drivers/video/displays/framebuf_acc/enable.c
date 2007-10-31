/*
 * ReactOS Generic Framebuffer display driver
 *
 * Copyright (C) 2004 Filip Navara
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "framebuf.h"

static DRVFN DrvFunctionTable[] =
{
   {INDEX_DrvEnablePDEV, (PFN)DrvEnablePDEV},
   {INDEX_DrvCompletePDEV, (PFN)DrvCompletePDEV},
   {INDEX_DrvDisablePDEV, (PFN)DrvDisablePDEV},
   {INDEX_DrvEnableSurface, (PFN)DrvEnableSurface},
   {INDEX_DrvDisableSurface, (PFN)DrvDisableSurface},
   {INDEX_DrvAssertMode, (PFN)DrvAssertMode},
   {INDEX_DrvGetModes, (PFN)DrvGetModes},
   {INDEX_DrvSetPalette, (PFN)DrvSetPalette},
   {INDEX_DrvSetPointerShape, (PFN)DrvSetPointerShape},
   {INDEX_DrvMovePointer, (PFN)DrvMovePointer}

};

/*
 * DrvEnableDriver
 *
 * Initial driver entry point exported by the driver DLL. It fills in a
 * DRVENABLEDATA structure with the driver's DDI version number and the
 * calling addresses of all DDI functions supported by the driver.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
DrvEnableDriver(
   ULONG iEngineVersion,
   ULONG cj,
   PDRVENABLEDATA pded)
{
   if (cj >= sizeof(DRVENABLEDATA))
   {
      pded->c = sizeof(DrvFunctionTable) / sizeof(DRVFN);
      pded->pdrvfn = DrvFunctionTable;
      pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

/*
 * DrvEnablePDEV
 *
 * Returns a description of the physical device's characteristics to GDI.
 *
 * Status
 *    @implemented
 */

DHPDEV APIENTRY
DrvEnablePDEV(
   IN DEVMODEW *pdm,
   IN LPWSTR pwszLogAddress,
   IN ULONG cPat,
   OUT HSURF *phsurfPatterns,
   IN ULONG cjCaps,
   OUT ULONG *pdevcaps,
   IN ULONG cjDevInfo,
   OUT DEVINFO *pdi,
   IN HDEV hdev,
   IN LPWSTR pwszDeviceName,
   IN HANDLE hDriver)
{
   PPDEV ppdev;
   GDIINFO GdiInfo;
   DEVINFO DevInfo;

   ppdev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
   if (ppdev == NULL)
   {
      return NULL;
   }

   ppdev->hDriver = hDriver;

   if (!IntInitScreenInfo(ppdev, pdm, &GdiInfo, &DevInfo))
   {
      EngFreeMem(ppdev);
      return NULL;
   }

   if (!IntInitDefaultPalette(ppdev, &DevInfo))
   {
      EngFreeMem(ppdev);
      return NULL;
   }

   memcpy(pdi, &DevInfo, min(sizeof(DEVINFO), cjDevInfo));
   memcpy(pdevcaps, &GdiInfo, min(sizeof(GDIINFO), cjCaps));

   return (DHPDEV)ppdev;
}

/*
 * DrvCompletePDEV
 *
 * Stores the GDI handle (hdev) of the physical device in dhpdev. The driver
 * should retain this handle for use when calling GDI services.
 *
 * Status
 *    @implemented
 */

VOID APIENTRY
DrvCompletePDEV(
   IN DHPDEV dhpdev,
   IN HDEV hdev)
{
   ((PPDEV)dhpdev)->hDevEng = hdev;
}

/*
 * DrvDisablePDEV
 *
 * Release the resources allocated in DrvEnablePDEV.  If a surface has been
 * enabled DrvDisableSurface will have already been called.
 *
 * Status
 *    @implemented
 */

VOID APIENTRY
DrvDisablePDEV(
   IN DHPDEV dhpdev)
{
   if (((PPDEV)dhpdev)->DefaultPalette)
   {
      EngDeletePalette(((PPDEV)dhpdev)->DefaultPalette);
   }

   if (((PPDEV)dhpdev)->PaletteEntries != NULL)
   {
      EngFreeMem(((PPDEV)dhpdev)->PaletteEntries);
   }

   EngFreeMem(dhpdev);
}
