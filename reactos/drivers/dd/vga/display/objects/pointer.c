#include "../vgaddi.h"

ULONG oldx, oldy;
PUCHAR behindCursor;

void vgaHideCursor(PPDEV ppdev);
void vgaShowCursor(PPDEV ppdev);

BOOL InitPointer(PPDEV ppdev)
{
  ULONG CursorWidth = 16, CursorHeight = 16;

  // Determine the size of the pointer attributes
  ppdev->PointerAttributes = sizeof(VIDEO_POINTER_ATTRIBUTES) +
    (CursorWidth * CursorHeight) * 2; // space for two cursors (data and mask); we assume 4bpp.. but use 8bpp for speed

  // Allocate memory for pointer attributes
  ppdev->pPointerAttributes = EngAllocMem(0, ppdev->PointerAttributes, ALLOC_TAG);

  ppdev->pPointerAttributes->Flags = 0; // FIXME: Do this right
  ppdev->pPointerAttributes->Width = CursorWidth;
  ppdev->pPointerAttributes->Height = CursorHeight;
  ppdev->pPointerAttributes->WidthInBytes = CursorWidth;
  ppdev->pPointerAttributes->Enable = 0;
  ppdev->pPointerAttributes->Column = 0;
  ppdev->pPointerAttributes->Row = 0;

  // Allocate memory for the pixels behind the cursor
  behindCursor = EngAllocMem(0, ppdev->pPointerAttributes->WidthInBytes * ppdev->pPointerAttributes->Height, ALLOC_TAG);

  return TRUE;
}

VOID VGADDIMovePointer(PSURFOBJ pso, LONG x, LONG y, PRECTL prcl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;

  if(x == -1)
  {
    // x == -1 and y == -1 indicates we must hide the cursor
    vgaHideCursor(ppdev);
    return;
  }

  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  vgaShowCursor(ppdev);

  // Give feedback on the new cursor rectangle
//  if (prcl != NULL) ComputePointerRect(ppdev, prcl);
}

ULONG VGADDISetPointerShape(PSURFOBJ pso, PSURFOBJ psoMask, PSURFOBJ psoColor, PXLATEOBJ pxlo,
			    LONG xHot, LONG yHot, LONG x, LONG y,
			    PRECTL prcl, ULONG fl)
{
  PPDEV ppdev = (PPDEV)pso->dhpdev;
  ULONG cursorBytes = ppdev->pPointerAttributes->WidthInBytes * ppdev->pPointerAttributes->Height;

  // Hide the cursor (if it's there -- FIXME?)
  if(ppdev->pPointerAttributes->Enable != 0) vgaHideCursor(ppdev);

  // Copy the mask and color bitmaps into the PPDEV
  RtlCopyMemory(ppdev->pPointerAttributes->Pixels, psoMask->pvBits, cursorBytes);
  if(psoColor != NULL) RtlCopyMemory(ppdev->pPointerAttributes->Pixels + cursorBytes, psoColor->pvBits, cursorBytes);

  // Set the new cursor position
  ppdev->xyCursor.x = x;
  ppdev->xyCursor.y = y;

  // Show the cursor
  vgaShowCursor(ppdev);
}

void vgaHideCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy, bitpos;

  // Display what was behind cursor
  DFB_BltToVGA(oldx, oldx, oldy,
               ppdev->pPointerAttributes->Width-1,
               ppdev->pPointerAttributes->Height-1,
               behindCursor);

  oldx = ppdev->xyCursor.x;
  oldy = ppdev->xyCursor.y;

  ppdev->pPointerAttributes->Enable = 0;
}

void vgaShowCursor(PPDEV ppdev)
{
  ULONG i, j, cx, cy, bitpos;

  if(ppdev->pPointerAttributes->Enable != 0) vgaHideCursor(ppdev);

  // Capture pixels behind the cursor
  cx = ppdev->xyCursor.x;
  cy = ppdev->xyCursor.y;
  bitpos = 0;
  for (j=0; j<ppdev->pPointerAttributes->Height; j++)
  {
    cx = ppdev->xyCursor.x;
    for (i=0; i<ppdev->pPointerAttributes->Width; i++)
    {
      behindCursor[bitpos] = vgaGetPixel(cx, cy);
      bitpos++;
      cx++;
    }
    cy++;
  }

  // Display the cursor
  DIB_BltToVGA(ppdev->xyCursor.x, ppdev->xyCursor.x, ppdev->xyCursor.y,
               ppdev->pPointerAttributes->Width-1,
               ppdev->pPointerAttributes->Height-1,
               ppdev->pPointerAttributes->Pixels);

  ppdev->pPointerAttributes->Enable = 1;
}
