#include <ddk/ntddk.h>
#include <win32k/dc.h>
#include "../../drivers/input/include/mouse.h"
#include "objects.h"

BOOLEAN SafetySwitch = FALSE, SafetySwitch2 = FALSE, MouseEnabled = FALSE;
LONG mouse_x, mouse_y;
UINT mouse_width = 0, mouse_height = 0;

INT MouseSafetyOnDrawStart(PSURFOBJ SurfObj, PSURFGDI SurfGDI, LONG HazardX1, LONG HazardY1, LONG HazardX2, LONG HazardY2)
{
  RECTL MouseRect;
  LONG tmp;

  if(SurfObj == NULL) return 0;

  if((SurfObj->iType != STYPE_DEVICE) || (MouseEnabled == FALSE)) return 0;

  if(HazardX1 > HazardX2) { tmp = HazardX2; HazardX2 = HazardX1; HazardX1 = tmp; }
  if(HazardY1 > HazardY2) { tmp = HazardY2; HazardY2 = HazardY1; HazardY1 = tmp; }

  if( (mouse_x + mouse_width >= HazardX1)  && (mouse_x <= HazardX2) &&
      (mouse_y + mouse_height >= HazardY1) && (mouse_y <= HazardY2) )
  {
    SurfGDI->MovePointer(SurfObj, -1, -1, &MouseRect);
    SafetySwitch = TRUE;
  }

  // Mouse is not allowed to move if GDI is busy drawing
  SafetySwitch2 = TRUE;

  return 1;
}

INT MouseSafetyOnDrawEnd(PSURFOBJ SurfObj, PSURFGDI SurfGDI)
{
  RECTL MouseRect;

  if(SurfObj == NULL) return 0;

  if((SurfObj->iType != STYPE_DEVICE) || (MouseEnabled == FALSE)) return 0;

  if(SafetySwitch == TRUE)
  {
    SurfGDI->MovePointer(SurfObj, mouse_x, mouse_y, &MouseRect);
    SafetySwitch = FALSE;
  }

  SafetySwitch2 = FALSE;

  return 1;
}

VOID MouseGDICallBack(PMOUSE_INPUT_DATA Data, ULONG InputCount)
{
  ULONG i;
  LONG mouse_cx = 0, mouse_cy = 0;
  HDC hDC = W32kGetScreenDC();
  PDC dc;
  PSURFOBJ SurfObj;
  PSURFGDI SurfGDI;
  RECTL MouseRect; 

  PDEVICE_OBJECT ClassDeviceObject = NULL;
  PFILE_OBJECT FileObject = NULL;
  NTSTATUS status;
  UNICODE_STRING ClassName;
  IO_STATUS_BLOCK ioStatus;
  KEVENT event;
  PIRP irp;

  if (hDC == 0)
    {
      return;
    }

  dc = DC_HandleToPtr(hDC);
  SurfObj = (PSURFOBJ)AccessUserObject(dc->Surface);
  SurfGDI = (PSURFGDI)AccessInternalObject(dc->Surface);

  // Compile the total mouse movement change
  for (i=0; i<InputCount; i++)
  {
    mouse_cx += Data[i].LastX;
    mouse_cy += Data[i].LastY;
  }

  if((mouse_cx != 0) || (mouse_cy != 0))
  {
    mouse_x += mouse_cx;
    mouse_y += mouse_cy;

    if(mouse_x < 0) mouse_x = 0;
    if(mouse_y < 0) mouse_y = 0;
    if(mouse_x > 620) mouse_x = 620;
    if(mouse_y > 460) mouse_y = 460;

    if((SafetySwitch == FALSE) && (SafetySwitch2 == FALSE)) ;
    SurfGDI->MovePointer(SurfObj, mouse_x, mouse_y, &MouseRect); 
  }
}

VOID EnableMouse(HDC hDisplayDC)
{
  PDC dc = DC_HandleToPtr(hDisplayDC);
  PSURFOBJ SurfObj = (PSURFOBJ)AccessUserObject(dc->Surface);
  PSURFGDI SurfGDI = (PSURFGDI)AccessInternalObject(dc->Surface);
  BOOL txt;
  int i;

  BRUSHOBJ Brush;
  HBITMAP hMouseSurf;
  PSURFOBJ MouseSurf;
  SIZEL MouseSize;
  POINTL ZeroPoint;
  RECTL MouseRect;

  // Draw a test mouse cursor
  mouse_width  = 16;
  mouse_height = 16;

  // Draw transparent colored rectangle
  Brush.iSolidColor = 5;
  for (i = 0; i < 17; i++)
    EngLineTo(SurfObj, NULL, &Brush, 0, i, 17, i, NULL, 0);

  // Draw white interior
  Brush.iSolidColor = 15;
  for (i = 1; i < 16; i++)
    EngLineTo(SurfObj, NULL, &Brush, 0, i-1, 16-i, i-1, NULL, 0);

  // Draw black outline
  Brush.iSolidColor = 0;
  EngLineTo(SurfObj, NULL, &Brush, 0, 0, 15, 0, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 0, 16, 15, 0, NULL, 0);
  EngLineTo(SurfObj, NULL, &Brush, 0, 15, 0, 0, NULL, 0);

  // Create the bitmap for the mouse cursor data
  MouseSize.cx = 16;
  MouseSize.cy = 16;
  hMouseSurf = EngCreateBitmap(MouseSize, 16, BMF_4BPP, 0, NULL);
  MouseSurf = (PSURFOBJ)AccessUserObject(hMouseSurf);

  // Capture the cursor we drew in the mouse cursor buffer
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;
  MouseRect.top = 0;
  MouseRect.left = 0;
  MouseRect.bottom = 16;
  MouseRect.right = 16;
  EngBitBlt(MouseSurf, SurfObj, NULL, NULL, NULL, &MouseRect, &ZeroPoint, NULL, NULL, NULL, 0);
  SurfGDI->SetPointerShape(SurfObj, MouseSurf, NULL, NULL, 0, 0, 50, 50, &MouseRect, 0);

  mouse_x = 320;
  mouse_y = 240;
  MouseEnabled = TRUE;
}

