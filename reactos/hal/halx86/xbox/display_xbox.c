/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            hal/halx86/xbox/display_xbox.c
 * PURPOSE:         Blue screen display
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 08/10/99
 *                  Modified for Xbox 2004/12/02 GvG
 */

/* For an explanation about display ownership see generic/display.c */

#include <halxbox.h>

#define NDEBUG
#include <debug.h>

#define I2C_IO_BASE 0xc000

#define CONTROL_FRAMEBUFFER_ADDRESS_OFFSET 0x600800

#define MAKE_COLOR(Red, Green, Blue) (0xff000000 | (((Red) & 0xff) << 16) | (((Green) & 0xff) << 8) | ((Blue) & 0xff))

/* Default to grey on blue */
#define DEFAULT_FG_COLOR MAKE_COLOR(127, 127, 127)
#define DEFAULT_BG_COLOR MAKE_COLOR(0, 0, 127)

#define TAG_HALX     TAG('H', 'A', 'L', 'X')

/* VARIABLES ****************************************************************/

static ULONG CursorX = 0;      /* Cursor Position */
static ULONG CursorY = 0;
static ULONG SizeX;            /* Display size (characters) */
static ULONG SizeY;

static BOOLEAN DisplayInitialized = FALSE;
static BOOLEAN HalOwnsDisplay = TRUE;

static PHAL_RESET_DISPLAY_PARAMETERS HalResetDisplayParameters = NULL;

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16

static PVOID FrameBuffer;
static ULONG BytesPerPixel;
static ULONG Delta;

/*
 * It turns out that reading from the frame buffer is a pretty expensive
 * operation. So, we're keeping shadow arrays of the contents and use
 * those when needed (only for scrolling) instead of reading from the fb.
 * This cuts down boot time from about 45 sec to about 6 sec.
 */
static PUCHAR CellContents;
static PULONG CellFgColor;
static PULONG CellBgColor;

/* PRIVATE FUNCTIONS *********************************************************/

static VOID FASTCALL
HalpXboxOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
  PUCHAR FontPtr;
  PULONG Pixel;
  UCHAR Mask;
  unsigned Line;
  unsigned Col;

  FontPtr = XboxFont8x16 + Char * 16;
  Pixel = (PULONG) ((char *) FrameBuffer + Y * CHAR_HEIGHT * Delta
                  + X * CHAR_WIDTH * BytesPerPixel);
  for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
      Mask = 0x80;
      for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
          Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
          Mask = Mask >> 1;
        }
      Pixel = (PULONG) ((char *) Pixel + Delta);
    }

  if (NULL != CellContents)
    {
      CellContents[Y * SizeX + X] = Char;
      CellFgColor[Y * SizeX + X] = FgColor;
      CellBgColor[Y * SizeX + X] = BgColor;
    }
}

static ULONG FASTCALL
HalpXboxAttrToSingleColor(UCHAR Attr)
{
  UCHAR Intensity;

  Intensity = (0 == (Attr & 0x08) ? 127 : 255);

  return 0xff000000 |
         (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
         (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
         (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID FASTCALL
HalpXboxAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
  *FgColor = HalpXboxAttrToSingleColor(Attr & 0xf);
  *BgColor = HalpXboxAttrToSingleColor((Attr >> 4) & 0xf);
}

static VOID FASTCALL
HalpXboxClearScreenColor(ULONG Color)
{
  ULONG Line, Col;
  PULONG p;

  for (Line = 0; Line < SizeY * CHAR_HEIGHT; Line++)
    {
      p = (PULONG) ((char *) FrameBuffer + Line * Delta);
      for (Col = 0; Col < SizeX * CHAR_WIDTH; Col++)
        {
          *p++ = Color;
        }
    }

  if (NULL != CellContents)
    {
      for (Line = 0; Line < SizeY; Line++)
        {
          for (Col = 0; Col < SizeX; Col++)
            {
              CellContents[Line * SizeX + Col] = ' ';
              CellFgColor[Line * SizeX + Col] = Color;
              CellBgColor[Line * SizeX + Col] = Color;
            }
        }
    }
}

VOID FASTCALL
HalClearDisplay(UCHAR CharAttribute)
{
  ULONG FgColor, BgColor;

  HalpXboxAttrToColors(CharAttribute, &FgColor, &BgColor);

  HalpXboxClearScreenColor(BgColor);

  CursorX = 0;
  CursorY = 0;
}

VOID static
HalScrollDisplay (VOID)
{
  ULONG Line, Col;
  PULONG p;

  if (NULL == CellContents)
    {
      p = (PULONG) ((char *) FrameBuffer + (Delta * CHAR_HEIGHT));
      RtlMoveMemory(FrameBuffer,
                    p,
                    (Delta * CHAR_HEIGHT) * (SizeY - 1));

      for (Line = 0; Line < CHAR_HEIGHT; Line++)
        {
          p = (PULONG) ((char *) FrameBuffer + (CHAR_HEIGHT * (SizeY - 1 ) + Line) * Delta);
          for (Col = 0; Col < SizeX * CHAR_WIDTH; Col++)
            {
              *p++ = DEFAULT_BG_COLOR;
            }
        }
    }
  else
    {
      for (Line = 0; Line < SizeY - 1; Line++)
        {
          for (Col = 0; Col < SizeX; Col++)
            {
              HalpXboxOutputChar(CellContents[(Line + 1) * SizeX + Col], Col, Line,
                                 CellFgColor[(Line + 1) * SizeX + Col],
                                 CellBgColor[(Line + 1) * SizeX + Col]);
            }
        }
      for (Col = 0; Col < SizeX; Col++)
        {
          HalpXboxOutputChar(' ', Col, SizeY - 1, DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
        }
    }
}

static VOID FASTCALL
HalPutCharacter(UCHAR Character)
{
  HalpXboxOutputChar(Character, CursorX, CursorY, DEFAULT_FG_COLOR, DEFAULT_BG_COLOR);
}

static BOOLEAN
ReadfromSMBus(UCHAR Address, UCHAR bRegister, UCHAR Size, ULONG *Data_to_smbus)
{
  int nRetriesToLive=50;

  while (0 != (READ_PORT_USHORT((PUSHORT) (I2C_IO_BASE + 0)) & 0x0800))
    {
      ;  /* Franz's spin while bus busy with any master traffic */
    }

  while (0 != nRetriesToLive--)
    {
      UCHAR b;
      int temp;

      WRITE_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 4), (Address << 1) | 1);
      WRITE_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 8), bRegister);

      temp = READ_PORT_USHORT((PUSHORT) (I2C_IO_BASE + 0));
      WRITE_PORT_USHORT((PUSHORT) (I2C_IO_BASE + 0), temp);  /* clear down all preexisting errors */

      switch (Size)
        {
          case 4:
            WRITE_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 2), 0x0d);      /* DWORD modus ? */
            break;
          case 2:
            WRITE_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 2), 0x0b);      /* WORD modus */
            break;
          default:
            WRITE_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 2), 0x0a);      // BYTE
            break;
        }

      b = 0;

      while (0 == (b & 0x36))
        {
          b = READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 0));
        }

      if (0 != (b & 0x24))
        {
          /* printf("I2CTransmitByteGetReturn error %x\n", b); */
        }

      if(0 == (b & 0x10))
        {
          /* printf("I2CTransmitByteGetReturn no complete, retry\n"); */
        }
      else
        {
          switch (Size)
            {
              case 4:
                READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 6));
                READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 9));
                READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 9));
                READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 9));
                READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 9));
                break;
              case 2:
                *Data_to_smbus = READ_PORT_USHORT((PUSHORT) (I2C_IO_BASE + 6));
                break;
              default:
                *Data_to_smbus = READ_PORT_UCHAR((PUCHAR) (I2C_IO_BASE + 6));
                break;
            }


          return TRUE;
        }
    }

  return FALSE;
}


static BOOLEAN
I2CTransmitByteGetReturn(UCHAR bPicAddressI2cFormat, UCHAR bDataToWrite, ULONG *Return)
{
  return ReadfromSMBus(bPicAddressI2cFormat, bDataToWrite, 1, Return);
}

VOID FASTCALL
HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Initalize the display
 * ARGUMENTS:
 *         InitParameters = Parameters setup by the boot loader
 */
{
  ULONG ScreenWidthPixels;
  ULONG ScreenHeightPixels;
  PHYSICAL_ADDRESS PhysControl;
  PHYSICAL_ADDRESS PhysBuffer;
  ULONG AvMode = 0;
  PVOID ControlBuffer;

  if (! DisplayInitialized)
    {
      PhysBuffer.u.HighPart = 0;
      //FIXME: We always assume 64Mb Xbox for now, until switch to
      //       NT-style LPB is finished
      /*if (0 != (LoaderBlock->Flags & MB_FLAGS_MEM_INFO))
        {
          PhysBuffer.u.LowPart = (LoaderBlock->MemHigher + 1024) * 1024;
        }
      else*/
        {
          /* Assume a 64Mb Xbox, last 4MB for video buf */
          PhysBuffer.u.LowPart = 60 * 1024 * 1024;
        }
      PhysBuffer.u.LowPart |= 0xf0000000;

      /* Tell the nVidia controller about the framebuffer */
      PhysControl.u.HighPart = 0;
      PhysControl.u.LowPart = 0xfd000000;
      ControlBuffer = MmMapIoSpace(PhysControl, 0x1000000, MmNonCached);
      if (NULL == ControlBuffer)
        {
          return;
        }
      *((PULONG) ((char *) ControlBuffer + CONTROL_FRAMEBUFFER_ADDRESS_OFFSET)) = (ULONG) PhysBuffer.u.LowPart;
      MmUnmapIoSpace(ControlBuffer, 0x1000000);

      FrameBuffer = MmMapIoSpace(PhysBuffer, 4 * 1024 * 1024, MmNonCached);
      if (NULL == FrameBuffer)
        {
          return;
        }

      if (I2CTransmitByteGetReturn(0x10, 0x04, &AvMode))
        {
          if (1 == AvMode) /* HDTV */
            {
              ScreenWidthPixels = 720;
            }
          else
            {
              /* FIXME Other possible values of AvMode:
               * 0 - AV_SCART_RGB
               * 2 - AV_VGA_SOG
               * 4 - AV_SVIDEO
               * 6 - AV_COMPOSITE
               * 7 - AV_VGA
               * other AV_COMPOSITE
               */
              ScreenWidthPixels = 640;
            }
        }
      else
        {
          ScreenWidthPixels = 640;
        }
      ScreenHeightPixels = 480;
      BytesPerPixel = 4;

      SizeX = ScreenWidthPixels / CHAR_WIDTH;
      SizeY = ScreenHeightPixels / CHAR_HEIGHT;
      Delta = (ScreenWidthPixels * BytesPerPixel + 3) & ~ 0x3;

      CellFgColor = (PULONG) ExAllocatePoolWithTag(PagedPool,
                                                   SizeX * SizeY * (sizeof(ULONG) + sizeof(ULONG) + sizeof(UCHAR)),
                                                   TAG_HALX);
      if (NULL != CellFgColor)
        {
          CellBgColor = CellFgColor + SizeX * SizeY;
          CellContents = (PUCHAR) (CellBgColor + SizeX * SizeY);
        }
      else
        {
          CellBgColor = NULL;
          CellContents = NULL;
        }

      HalpXboxClearScreenColor(MAKE_COLOR(0, 0, 0));

      DisplayInitialized = TRUE;
    }
}


/* PUBLIC FUNCTIONS *********************************************************/

VOID STDCALL
HalReleaseDisplayOwnership(VOID)
/*
 * FUNCTION: Release ownership of display back to HAL
 */
{
  if (HalOwnsDisplay || NULL == HalResetDisplayParameters)
    {
      return;
    }

  HalResetDisplayParameters(SizeX, SizeY);

  HalOwnsDisplay = TRUE;
  HalpXboxClearScreenColor(DEFAULT_BG_COLOR);

  CursorX = 0;
  CursorY = 0;
}


VOID STDCALL
HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters)
/*
 * FUNCTION: 
 * ARGUMENTS:
 *         ResetDisplayParameters = Pointer to a driver specific
 *         reset routine.
 */
{
  HalOwnsDisplay = FALSE;
  HalResetDisplayParameters = ResetDisplayParameters;
}

VOID STDCALL
HalDisplayString(IN PCH String)
/*
 * FUNCTION: Switches the screen to HAL console mode (BSOD) if not there
 * already and displays a string
 * ARGUMENT:
 *        string = ASCII string to display
 * NOTE: Use with care because there is no support for returning from BSOD
 * mode
 */
{
  PCH pch;
  static KSPIN_LOCK Lock;
  KIRQL OldIrql;
  ULONG Flags;

  if (! HalOwnsDisplay || ! DisplayInitialized)
    {
      return;
    }

  pch = String;

  OldIrql = KfRaiseIrql(HIGH_LEVEL);
  KiAcquireSpinLock(&Lock);

  Ke386SaveFlags(Flags);
  _disable();

  while (*pch != 0)
    {
      if (*pch == '\n')
	{
	  CursorY++;
	  CursorX = 0;
	}
      else if (*pch == '\b')
	{
	  if (CursorX > 0)
	    {
	      CursorX--;
	    }
	}
      else if (*pch != '\r')
	{
	  HalPutCharacter(*pch);
	  CursorX++;
	  
	  if (SizeX <= CursorX)
	    {
	      CursorY++;
	      CursorX = 0;
	    }
	}

      if (SizeY <= CursorY)
	{
	  HalScrollDisplay ();
	  CursorY = SizeY - 1;
	}
  
      pch++;
    }
  
  Ke386RestoreFlags(Flags);

  KiReleaseSpinLock(&Lock);
  KfLowerIrql(OldIrql);
}

VOID STDCALL
HalQueryDisplayParameters(OUT PULONG DispSizeX,
			  OUT PULONG DispSizeY,
			  OUT PULONG CursorPosX,
			  OUT PULONG CursorPosY)
{
  if (DispSizeX)
    *DispSizeX = SizeX;
  if (DispSizeY)
    *DispSizeY = SizeY;
  if (CursorPosX)
    *CursorPosX = CursorX;
  if (CursorPosY)
    *CursorPosY = CursorY;
}


VOID STDCALL
HalSetDisplayParameters(IN ULONG CursorPosX,
			IN ULONG CursorPosY)
{
  CursorX = (CursorPosX < SizeX) ? CursorPosX : SizeX - 1;
  CursorY = (CursorPosY < SizeY) ? CursorPosY : SizeY - 1;
}


BOOLEAN STDCALL
HalQueryDisplayOwnership(VOID)
{
  return ! HalOwnsDisplay;
}

/* EOF */
