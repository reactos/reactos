/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Bitmap Functions
 * FILE:              subsys/win32k/eng/palette.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 11/7/1999: Created
 */

#include <ddk/winddi.h>

HPALETTE EngCreatePalette(IN ULONG  Mode,
                          IN ULONG  NumColors,
                          IN PULONG  *Colors,
                          IN ULONG  Red,
                          IN ULONG  Green,
                          IN ULONG  Blue)
{
  /* We need to take the colors given to us and generate a nice default color
     model */

  if(Mode==PAL_INDEXED)
  {
    /* For now the ultimate color model is just colors.. */
  }

  /* FIXME: Add support for other given palette types */

  /* FIXME: Generate a handle for Colors */

  return Colors;
}

BOOL EngDeletePalette(IN HPALETTE hpal)
{
  /* Should actually get the pointer from this handle.. which for now IS
     the pointer */

  EngFreeMem(hpal);
}
