/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Enumeration Functions
 * FILE:              subsys/win32k/eng/enum.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/winddi.h>

ULONG CLIPOBJ_cEnumStart(IN PCLIPOBJ  ClipObj,
                         IN BOOL  ShouldDoAll,
                         IN ULONG  ClipType,
                         IN ULONG  BuildOrder,
                         IN ULONG  MaxRects)
{
   // Sets the parameters for enumerating rectables in the given clip region

   ULONG enumCount = 0;
   ENUMRECTS enumRects;

   // MUCH WORK TO DO HERE

   // Return the number of rectangles enumerated
   if(enumCount>MaxRects)
   {
     enumCount = 0xFFFFFFFF;
   }
   return enumCount;
}

BOOL CLIPOBJ_bEnum(IN PCLIPOBJ  ClipObj,
                   IN ULONG  ObjSize,
                   OUT ULONG  *EnumRects)
{
}
