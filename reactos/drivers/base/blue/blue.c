/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 services/dd/blue/blue.c
 * PURPOSE:              Console (blue screen) device driver
 * PROGRAMMER:           Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                       ??? Created
 */

/* INCLUDES ******************************************************************/

#include <ntddk.h>
#include <windef.h>
#define WINBASEAPI
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;

#include <wincon.h>
#include <blue/ntddblue.h>
#include <ndk/inbvfuncs.h>
#include <intrin.h>

#define NDEBUG
#include <debug.h>

// ROS Internal. Please deprecate.
NTHALAPI
BOOLEAN
NTAPI
HalQueryDisplayOwnership(
    VOID
);

/* DEFINITIONS ***************************************************************/

#define VIDMEM_BASE        0xb8000

#define CRTC_COMMAND       ((PUCHAR)0x3d4)
#define CRTC_DATA          ((PUCHAR)0x3d5)

#define CRTC_COLUMNS       0x01
#define CRTC_OVERFLOW      0x07
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09
#define CRTC_CURSORSTART   0x0a
#define CRTC_CURSOREND     0x0b
#define CRTC_CURSORPOSHI   0x0e
#define CRTC_CURSORPOSLO   0x0f

#define ATTRC_WRITEREG     ((PUCHAR)0x3c0)
#define ATTRC_READREG      ((PUCHAR)0x3c1)
#define ATTRC_INPST1       ((PUCHAR)0x3da)

#define TAB_WIDTH          8

#define MISC         (PUCHAR)0x3c2
#define SEQ          (PUCHAR)0x3c4
#define SEQDATA      (PUCHAR)0x3c5
#define CRTC         (PUCHAR)0x3d4
#define CRTCDATA     (PUCHAR)0x3d5
#define GRAPHICS     (PUCHAR)0x3ce
#define GRAPHICSDATA (PUCHAR)0x3cf
#define ATTRIB       (PUCHAR)0x3c0
#define STATUS       (PUCHAR)0x3da
#define PELMASK      (PUCHAR)0x3c6
#define PELINDEX     (PUCHAR)0x3c8
#define PELDATA      (PUCHAR)0x3c9

/* NOTES ******************************************************************/
/*
 *  [[character][attribute]][[character][attribute]]....
 */


/* TYPEDEFS ***************************************************************/

typedef struct _DEVICE_EXTENSION
{
    PUCHAR VideoMemory;    /* Pointer to video memory */
    ULONG CursorSize;
    INT  CursorVisible;
    USHORT  CharAttribute;
    ULONG Mode;
    UCHAR  ScanLines;      /* Height of a text line */
    USHORT  Rows;           /* Number of rows        */
    USHORT  Columns;        /* Number of columns     */
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _VGA_REGISTERS
{
   UCHAR CRT[24];
   UCHAR Attribute[21];
   UCHAR Graphics[9];
   UCHAR Sequencer[5];
   UCHAR Misc;
} VGA_REGISTERS, *PVGA_REGISTERS;

static const VGA_REGISTERS VidpMode3Regs =
{
   /* CRT Controller Registers */
   {0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x47, 0x1E, 0x00,
    0x00, 0x00, 0x05, 0xF0, 0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3},
   /* Attribute Controller Registers */
   {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
    0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00},
   /* Graphics Controller Registers */
   {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x00, 0xFF},
   /* Sequencer Registers */
   {0x03, 0x00, 0x03, 0x00, 0x02},
   /* Misc Output Register */
   0x67
};

static const UCHAR DefaultPalette[] =
{
   0, 0, 0,
   0, 0, 0xC0,
   0, 0xC0, 0,
   0, 0xC0, 0xC0,
   0xC0, 0, 0,
   0xC0, 0, 0xC0,
   0xC0, 0xC0, 0,
   0xC0, 0xC0, 0xC0,
   0x80, 0x80, 0x80,
   0, 0, 0xFF,
   0, 0xFF, 0,
   0, 0xFF, 0xFF,
   0xFF, 0, 0,
   0xFF, 0, 0xFF,
   0xFF, 0xFF, 0,
   0xFF, 0xFF, 0xFF
};

/* FUNCTIONS **************************************************************/

static VOID FASTCALL
ScrSetRegisters(const VGA_REGISTERS *Registers)
{
    UINT i;

    /* Update misc output register */
    WRITE_PORT_UCHAR(MISC, Registers->Misc);

    /* Synchronous reset on */
    WRITE_PORT_UCHAR(SEQ, 0x00);
    WRITE_PORT_UCHAR(SEQDATA, 0x01);

    /* Write sequencer registers */
    for (i = 1; i < sizeof(Registers->Sequencer); i++)
{
        WRITE_PORT_UCHAR(SEQ, i);
        WRITE_PORT_UCHAR(SEQDATA, Registers->Sequencer[i]);
    }

    /* Synchronous reset off */
    WRITE_PORT_UCHAR(SEQ, 0x00);
    WRITE_PORT_UCHAR(SEQDATA, 0x03);

    /* Deprotect CRT registers 0-7 */
    WRITE_PORT_UCHAR(CRTC, 0x11);
    WRITE_PORT_UCHAR(CRTCDATA, Registers->CRT[0x11] & 0x7f);

    /* Write CRT registers */
    for (i = 0; i < sizeof(Registers->CRT); i++)
    {
        WRITE_PORT_UCHAR(CRTC, i);
        WRITE_PORT_UCHAR(CRTCDATA, Registers->CRT[i]);
    }

    /* Write graphics controller registers */
    for (i = 0; i < sizeof(Registers->Graphics); i++)
    {
        WRITE_PORT_UCHAR(GRAPHICS, i);
        WRITE_PORT_UCHAR(GRAPHICSDATA, Registers->Graphics[i]);
    }

    /* Write attribute controller registers */
    for (i = 0; i < sizeof(Registers->Attribute); i++)
    {
        READ_PORT_UCHAR(STATUS);
        WRITE_PORT_UCHAR(ATTRIB, i);
        WRITE_PORT_UCHAR(ATTRIB, Registers->Attribute[i]);
    }

    /* Set the PEL mask. */
    WRITE_PORT_UCHAR(PELMASK, 0xff);
}

static VOID FASTCALL
ScrAcquireOwnership(PDEVICE_EXTENSION DeviceExtension)
{
    unsigned int offset;
    UCHAR data, value;
    ULONG Index;

    ScrSetRegisters(&VidpMode3Regs);

    /* Disable screen and enable palette access. */
    READ_PORT_UCHAR(STATUS);
    WRITE_PORT_UCHAR(ATTRIB, 0x00);

    for (Index = 0; Index < sizeof(DefaultPalette) / 3; Index++)
    {
       WRITE_PORT_UCHAR(PELINDEX, Index);
       WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3] >> 2);
       WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3 + 1] >> 2);
       WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3 + 2] >> 2);
    }

    /* Enable screen and disable palette access. */
    READ_PORT_UCHAR(STATUS);
    WRITE_PORT_UCHAR(ATTRIB, 0x20);

    /* get current output position */
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    offset = READ_PORT_UCHAR (CRTC_DATA);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset += (READ_PORT_UCHAR (CRTC_DATA) << 8);

    /* switch blinking characters off */
    READ_PORT_UCHAR (ATTRC_INPST1);
    value = READ_PORT_UCHAR (ATTRC_WRITEREG);
    WRITE_PORT_UCHAR (ATTRC_WRITEREG, 0x10);
    data  = READ_PORT_UCHAR (ATTRC_READREG);
    data  = data & ~0x08;
    WRITE_PORT_UCHAR (ATTRC_WRITEREG, data);
    WRITE_PORT_UCHAR (ATTRC_WRITEREG, value);
    READ_PORT_UCHAR (ATTRC_INPST1);

    /* read screen information from crt controller */
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_COLUMNS);
    DeviceExtension->Columns = READ_PORT_UCHAR (CRTC_DATA) + 1;
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_ROWS);
    DeviceExtension->Rows = READ_PORT_UCHAR (CRTC_DATA);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_OVERFLOW);
    data = READ_PORT_UCHAR (CRTC_DATA);
    DeviceExtension->Rows |= (((data & 0x02) << 7) | ((data & 0x40) << 3));
    DeviceExtension->Rows++;
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_SCANLINES);
    DeviceExtension->ScanLines = (READ_PORT_UCHAR (CRTC_DATA) & 0x1F) + 1;

    /* show blinking cursor */
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORSTART);
    WRITE_PORT_UCHAR (CRTC_DATA, (DeviceExtension->ScanLines - 1) & 0x1F);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSOREND);
    data = READ_PORT_UCHAR (CRTC_DATA) & 0xE0;
    WRITE_PORT_UCHAR (CRTC_DATA,
                      data | ((DeviceExtension->ScanLines - 1) & 0x1F));

    /* calculate number of text rows */
    DeviceExtension->Rows =
        DeviceExtension->Rows / DeviceExtension->ScanLines;
#ifdef BOCHS_30ROWS
    DeviceExtension->Rows = 30;
#endif

    DPRINT1 ("%d Columns  %d Rows %d Scanlines\n",
            DeviceExtension->Columns,
            DeviceExtension->Rows,
            DeviceExtension->ScanLines);
}

NTSTATUS STDCALL
DriverEntry (PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

static NTSTATUS STDCALL
ScrCreate(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    PHYSICAL_ADDRESS BaseAddress;
    NTSTATUS Status;

    DeviceExtension = DeviceObject->DeviceExtension;
    
    ScrAcquireOwnership(DeviceExtension);

    /* get pointer to video memory */
    BaseAddress.QuadPart = VIDMEM_BASE;
    DeviceExtension->VideoMemory =
        (PUCHAR)MmMapIoSpace (BaseAddress, DeviceExtension->Rows * DeviceExtension->Columns * 2, MmNonCached);

    DeviceExtension->CursorSize    = 5; /* FIXME: value correct?? */
    DeviceExtension->CursorVisible = TRUE;

    /* more initialization */
    DeviceExtension->CharAttribute = 0x17;  /* light grey on blue */
    DeviceExtension->Mode = ENABLE_PROCESSED_OUTPUT |
                            ENABLE_WRAP_AT_EOL_OUTPUT;

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


static NTSTATUS STDCALL
ScrWrite(PDEVICE_OBJECT DeviceObject,
	 PIRP Irp)
{
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS Status;
    char *pch = Irp->UserBuffer;
    PUCHAR vidmem;
    unsigned int i;
    int j, offset;
    int cursorx, cursory;
    int rows, columns;
    int processed = DeviceExtension->Mode & ENABLE_PROCESSED_OUTPUT;

    if (0 && InbvCheckDisplayOwnership())
       {
	  /* Display is in graphics mode, we're not allowed to touch it */
	  Status = STATUS_SUCCESS;

	  Irp->IoStatus.Status = Status;
	  IoCompleteRequest (Irp, IO_NO_INCREMENT);

	  return Status;
       }

    vidmem  = DeviceExtension->VideoMemory;
    rows = DeviceExtension->Rows;
    columns = DeviceExtension->Columns;

    _disable();
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset = READ_PORT_UCHAR (CRTC_DATA)<<8;
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    offset += READ_PORT_UCHAR (CRTC_DATA);
    _enable();

    cursory = offset / columns;
    cursorx = offset % columns;
    if( processed == 0 )
       {
	  /* raw output mode */
	  memcpy( &vidmem[(cursorx * 2) + (cursory * columns * 2)], pch, stk->Parameters.Write.Length );
	  offset += (stk->Parameters.Write.Length / 2);
       }
    else {
       for (i = 0; i < stk->Parameters.Write.Length; i++, pch++)
	  {
	     switch (*pch)
		{
		case '\b':
		   if (cursorx > 0)
		      {
			 cursorx--;
		      }
		   else if (cursory > 0)
		      {
			 cursorx = columns - 1;
			 cursory--;
		      }
		   vidmem[(cursorx * 2) + (cursory * columns * 2)] = ' ';
		   vidmem[(cursorx * 2) + (cursory * columns * 2) + 1] = (char) DeviceExtension->CharAttribute;
		   break;

		case '\n':
		   cursory++;
		   cursorx = 0;
		   break;

		case '\r':
		   cursorx = 0;
		   break;

		case '\t':
		   offset = TAB_WIDTH - (cursorx % TAB_WIDTH);
		   for (j = 0; j < offset; j++)
		      {
			 vidmem[(cursorx * 2) + (cursory * columns * 2)] = ' ';
			 cursorx++;

			 if (cursorx >= columns)
			    {
			       cursory++;
			       cursorx = 0;
			    }
		      }
		   break;

		default:
		   vidmem[(cursorx * 2) + (cursory * columns * 2)] = *pch;
		   vidmem[(cursorx * 2) + (cursory * columns * 2) + 1] = (char) DeviceExtension->CharAttribute;
		   cursorx++;
		   if (cursorx >= columns)
		      {
			 cursory++;
			 cursorx = 0;
		      }
		   break;
		}
	     if (cursory >= rows)
		{
		   unsigned short *LinePtr;

		   memcpy (vidmem,
			   &vidmem[columns * 2],
			   columns * (rows - 1) * 2);

		   LinePtr = (unsigned short *) &vidmem[columns * (rows - 1) * 2];

		   for (j = 0; j < columns; j++)
		      {
			 LinePtr[j] = DeviceExtension->CharAttribute << 8;
		      }
		   cursory = rows - 1;
		   for (j = 0; j < columns; j++)
		      {
			 vidmem[(j * 2) + (cursory * columns * 2)] = ' ';
			 vidmem[(j * 2) + (cursory * columns * 2) + 1] = (char)DeviceExtension->CharAttribute;
		      }
		}
	  }

       /* Set the cursor position */
       offset = (cursory * columns) + cursorx;
    }
    _disable();
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    WRITE_PORT_UCHAR (CRTC_DATA, offset);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset >>= 8;
    WRITE_PORT_UCHAR (CRTC_DATA, offset);
    _enable();

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


static NTSTATUS STDCALL
ScrIoControl(PDEVICE_OBJECT DeviceObject,
	     PIRP Irp)
{
  PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation (Irp);
  PDEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = DeviceObject->DeviceExtension;
  switch (stk->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO:
        {
          PCONSOLE_SCREEN_BUFFER_INFO pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;
          int rows = DeviceExtension->Rows;
          int columns = DeviceExtension->Columns;
          unsigned int offset;

          /* read cursor position from crtc */
          _disable();
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
          offset = READ_PORT_UCHAR (CRTC_DATA);
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
          offset += (READ_PORT_UCHAR (CRTC_DATA) << 8);
          _enable();

          pcsbi->dwSize.X = columns;
          pcsbi->dwSize.Y = rows;

          pcsbi->dwCursorPosition.X = (SHORT)(offset % columns);
          pcsbi->dwCursorPosition.Y = (SHORT)(offset / columns);

          pcsbi->wAttributes = DeviceExtension->CharAttribute;

          pcsbi->srWindow.Left   = 0;
          pcsbi->srWindow.Right  = columns - 1;
          pcsbi->srWindow.Top    = 0;
          pcsbi->srWindow.Bottom = rows - 1;

          pcsbi->dwMaximumWindowSize.X = columns;
          pcsbi->dwMaximumWindowSize.Y = rows;

          Irp->IoStatus.Information = sizeof (CONSOLE_SCREEN_BUFFER_INFO);
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO:
        {
          PCONSOLE_SCREEN_BUFFER_INFO pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;
          unsigned int offset;

          DeviceExtension->CharAttribute = pcsbi->wAttributes;
          offset = (pcsbi->dwCursorPosition.Y * DeviceExtension->Columns) +
                    pcsbi->dwCursorPosition.X;

          _disable();
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
          WRITE_PORT_UCHAR (CRTC_DATA, offset);
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
          WRITE_PORT_UCHAR (CRTC_DATA, offset>>8);
          _enable();

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_GET_CURSOR_INFO:
        {
          PCONSOLE_CURSOR_INFO pcci = (PCONSOLE_CURSOR_INFO)Irp->AssociatedIrp.SystemBuffer;

          pcci->dwSize = DeviceExtension->CursorSize;
          pcci->bVisible = DeviceExtension->CursorVisible;

          Irp->IoStatus.Information = sizeof (CONSOLE_CURSOR_INFO);
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_SET_CURSOR_INFO:
        {
          PCONSOLE_CURSOR_INFO pcci = (PCONSOLE_CURSOR_INFO)Irp->AssociatedIrp.SystemBuffer;
          UCHAR data, value;
          ULONG size, height;

          DeviceExtension->CursorSize = pcci->dwSize;
          DeviceExtension->CursorVisible = pcci->bVisible;
          height = DeviceExtension->ScanLines;
          data = (pcci->bVisible) ? 0x00 : 0x20;

          size = (pcci->dwSize * height) / 100;
          if (size < 1)
            {
              size = 1;
            }

          data |= (UCHAR)(height - size);

          _disable();
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORSTART);
          WRITE_PORT_UCHAR (CRTC_DATA, data);
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSOREND);
          value = READ_PORT_UCHAR (CRTC_DATA) & 0xE0;
          WRITE_PORT_UCHAR (CRTC_DATA, value | (height - 1));

          _enable();

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_GET_MODE:
        {
          PCONSOLE_MODE pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;

          pcm->dwMode = DeviceExtension->Mode;

          Irp->IoStatus.Information = sizeof(CONSOLE_MODE);
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_SET_MODE:
        {
          PCONSOLE_MODE pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;

          DeviceExtension->Mode = pcm->dwMode;

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE:
        {
          POUTPUT_ATTRIBUTE Buf = (POUTPUT_ATTRIBUTE)Irp->AssociatedIrp.SystemBuffer;
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          vidmem = DeviceExtension->VideoMemory;
          offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                    (Buf->dwCoord.X * 2) + 1;

          for (dwCount = 0; dwCount < Buf->nLength; dwCount++)
            {
              vidmem[offset + (dwCount * 2)] = (char) Buf->wAttribute;
            }

          Buf->dwTransfered = Buf->nLength;

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE:
        {
          POUTPUT_ATTRIBUTE Buf = (POUTPUT_ATTRIBUTE)Irp->AssociatedIrp.SystemBuffer;
          PUSHORT pAttr = (PUSHORT)MmGetSystemAddressForMdl(Irp->MdlAddress);
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          vidmem = DeviceExtension->VideoMemory;
          offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                   (Buf->dwCoord.X * 2) + 1;

          for (dwCount = 0; dwCount < stk->Parameters.DeviceIoControl.OutputBufferLength; dwCount++, pAttr++)
            {
              *((char *) pAttr) = vidmem[offset + (dwCount * 2)];
            }

          Buf->dwTransfered = dwCount;

          Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE:
        {
          COORD *pCoord = (COORD *)MmGetSystemAddressForMdl(Irp->MdlAddress);
          CHAR *pAttr = (CHAR *)(pCoord + 1);
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          vidmem = DeviceExtension->VideoMemory;
          offset = (pCoord->Y * DeviceExtension->Columns * 2) +
                   (pCoord->X * 2) + 1;

          for (dwCount = 0; dwCount < (stk->Parameters.DeviceIoControl.OutputBufferLength - sizeof( COORD )); dwCount++, pAttr++)
            {
              vidmem[offset + (dwCount * 2)] = *pAttr;
            }
          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE:
        DeviceExtension->CharAttribute = (USHORT)*(PUSHORT)Irp->AssociatedIrp.SystemBuffer;
        Irp->IoStatus.Information = 0;
        Status = STATUS_SUCCESS;
        break;

      case IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER:
        {
          POUTPUT_CHARACTER Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          vidmem = DeviceExtension->VideoMemory;
          offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                   (Buf->dwCoord.X * 2);

          CHECKPOINT

          for (dwCount = 0; dwCount < Buf->nLength; dwCount++)
            {
              vidmem[offset + (dwCount * 2)] = (char) Buf->cCharacter;
            }

          Buf->dwTransfered = Buf->nLength;

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_READ_OUTPUT_CHARACTER:
        {
          POUTPUT_CHARACTER Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
          LPSTR pChar = (LPSTR)MmGetSystemAddressForMdl(Irp->MdlAddress);
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          vidmem = DeviceExtension->VideoMemory;
          offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                   (Buf->dwCoord.X * 2);

          for (dwCount = 0; dwCount < stk->Parameters.DeviceIoControl.OutputBufferLength; dwCount++, pChar++)
            {
              *pChar = vidmem[offset + (dwCount * 2)];
            }

          Buf->dwTransfered = dwCount;

          Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER:
        {
          COORD *pCoord;
          LPSTR pChar;
          PUCHAR vidmem;
          int offset;
          ULONG dwCount;

          pCoord = (COORD *)MmGetSystemAddressForMdl(Irp->MdlAddress);
          pChar = (CHAR *)(pCoord + 1);
          vidmem = DeviceExtension->VideoMemory;
          offset = (pCoord->Y * DeviceExtension->Columns * 2) +
                   (pCoord->X * 2);

          for (dwCount = 0; dwCount < (stk->Parameters.DeviceIoControl.OutputBufferLength - sizeof( COORD )); dwCount++, pChar++)
            {
              vidmem[offset + (dwCount * 2)] = *pChar;
            }

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      case IOCTL_CONSOLE_DRAW:
        {
          PCONSOLE_DRAW ConsoleDraw;
          PUCHAR Src, Dest;
          UINT SrcDelta, DestDelta, i, Offset;

          ConsoleDraw = (PCONSOLE_DRAW) MmGetSystemAddressForMdl(Irp->MdlAddress);
          Src = (PUCHAR) (ConsoleDraw + 1);
          SrcDelta = ConsoleDraw->SizeX * 2;
          Dest = DeviceExtension->VideoMemory +
                 (ConsoleDraw->Y * DeviceExtension->Columns + ConsoleDraw->X) * 2;
          DestDelta = DeviceExtension->Columns * 2;

          for (i = 0; i < ConsoleDraw->SizeY; i++)
            {
              RtlCopyMemory(Dest, Src, SrcDelta);
              Src += SrcDelta;
              Dest += DestDelta;
            }

          Offset = (ConsoleDraw->CursorY * DeviceExtension->Columns) +
                   ConsoleDraw->CursorX;

          _disable();
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
          WRITE_PORT_UCHAR (CRTC_DATA, Offset);
          WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
          WRITE_PORT_UCHAR (CRTC_DATA, Offset >> 8);
          _enable();

          Irp->IoStatus.Information = 0;
          Status = STATUS_SUCCESS;
        }
        break;

      default:
        Status = STATUS_NOT_IMPLEMENTED;
    }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return Status;
}


static NTSTATUS STDCALL
ScrDispatch(PDEVICE_OBJECT DeviceObject,
	    PIRP Irp)
{
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    switch (stk->MajorFunction)
    {
        case IRP_MJ_CLOSE:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }


    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


/*
 * Module entry point
 */
NTSTATUS STDCALL
DriverEntry (PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\BlueScreen");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\BlueScreen");

    DPRINT ("Screen Driver 0.0.6\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScrCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = ScrDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ]   = ScrDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = ScrWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL ] = ScrIoControl;

    Status = IoCreateDevice (DriverObject,
                             sizeof(DEVICE_EXTENSION),
                             &DeviceName,
                             FILE_DEVICE_SCREEN,
                             FILE_DEVICE_SECURE_OPEN,
                             TRUE,
                             &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = IoCreateSymbolicLink (&SymlinkName, &DeviceName);
    if (NT_SUCCESS(Status))
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    else
        IoDeleteDevice (DeviceObject);
    return Status;
}

/* EOF */
