/* $Id: bootvid.c,v 1.6 2004/02/10 16:22:55 navaraf Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/inbv/bootvid.c
 * PURPOSE:        Boot video support
 * PROGRAMMER:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *  12-07-2003 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntbootvid.h>
#include <reactos/resource.h>
#include <rosrtl/string.h>

#include "../../../ntoskrnl/include/internal/v86m.h"

/*#define NDEBUG*/
#include <debug.h>

#define RT_BITMAP   2

typedef struct tagRGBQUAD {
  unsigned char    rgbBlue;
  unsigned char    rgbGreen;
  unsigned char    rgbRed;
  unsigned char    rgbReserved;
} RGBQUAD, *PRGBQUAD;

typedef long FXPT2DOT30;

typedef struct tagCIEXYZ {
  FXPT2DOT30 ciexyzX; 
  FXPT2DOT30 ciexyzY; 
  FXPT2DOT30 ciexyzZ; 
} CIEXYZ;
typedef CIEXYZ * LPCIEXYZ; 

typedef struct tagCIEXYZTRIPLE {
  CIEXYZ  ciexyzRed; 
  CIEXYZ  ciexyzGreen; 
  CIEXYZ  ciexyzBlue; 
} CIEXYZTRIPLE;
typedef CIEXYZTRIPLE *LPCIEXYZTRIPLE;

typedef struct { 
  DWORD        bV5Size; 
  LONG         bV5Width; 
  LONG         bV5Height; 
  WORD         bV5Planes; 
  WORD         bV5BitCount; 
  DWORD        bV5Compression; 
  DWORD        bV5SizeImage; 
  LONG         bV5XPelsPerMeter; 
  LONG         bV5YPelsPerMeter; 
  DWORD        bV5ClrUsed; 
  DWORD        bV5ClrImportant; 
  DWORD        bV5RedMask; 
  DWORD        bV5GreenMask; 
  DWORD        bV5BlueMask; 
  DWORD        bV5AlphaMask; 
  DWORD        bV5CSType; 
  CIEXYZTRIPLE bV5Endpoints; 
  DWORD        bV5GammaRed; 
  DWORD        bV5GammaGreen; 
  DWORD        bV5GammaBlue; 
  DWORD        bV5Intent; 
  DWORD        bV5ProfileData; 
  DWORD        bV5ProfileSize; 
  DWORD        bV5Reserved; 
} BITMAPV5HEADER, *PBITMAPV5HEADER; 


#define MISC     0x3c2
#define SEQ      0x3c4
#define CRTC     0x3d4
#define GRAPHICS 0x3ce
#define FEATURE  0x3da
#define ATTRIB   0x3c0
#define STATUS   0x3da

typedef struct {
  ULONG r;
  ULONG g;
  ULONG b;
} FADER_PALETTE_ENTRY;

/* In pixelsups.S */
extern VOID
InbvPutPixels(int x, int y, unsigned long c);

/* GLOBALS *******************************************************************/

char *vidmem;

/* Must be 4 bytes per entry */
long maskbit[640];
long y80[480];

static HANDLE BitmapThreadHandle;
static CLIENT_ID BitmapThreadId;
static BOOLEAN BitmapIsDrawn;
static PUCHAR BootimageBitmap;
static BOOLEAN InGraphicsMode = FALSE;

/* DATA **********************************************************************/

static BOOLEAN VideoAddressSpaceInitialized = FALSE;
static PVOID NonBiosBaseAddress;
static PDRIVER_OBJECT BootVidDriverObject = NULL;

/* FUNCTIONS *****************************************************************/

static BOOLEAN
InbvFindBootimage()
{
  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
  LDR_RESOURCE_INFO ResourceInfo;
  NTSTATUS Status;
  PVOID BaseAddress = BootVidDriverObject->DriverStart;
  ULONG Size;

  ResourceInfo.Type = RT_BITMAP;
  ResourceInfo.Name = IDB_BOOTIMAGE;
  ResourceInfo.Language = 0x09;

  Status = LdrFindResource_U(BaseAddress,
    &ResourceInfo,
    RESOURCE_DATA_LEVEL,
    &ResourceDataEntry);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrFindResource_U() failed with status 0x%.08x\n", Status);
      return FALSE;
    }

  Status = LdrAccessResource(BaseAddress,
    ResourceDataEntry,
    (PVOID*)&BootimageBitmap,
    &Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("LdrAccessResource() failed with status 0x%.08x\n", Status);
      return FALSE;
    }

  return TRUE;
}


static BOOLEAN
InbvInitializeVideoAddressSpace(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PhysMemName;
   NTSTATUS Status;
   HANDLE PhysMemHandle;
   PVOID BaseAddress;
   LARGE_INTEGER Offset;
   ULONG ViewSize;
   CHAR IVT[1024];
   CHAR BDA[256];
   PVOID start = (PVOID)0x0;

   /*
    * Open the physical memory section
    */
   RtlRosInitUnicodeStringFromLiteral(&PhysMemName, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PhysMemName,
			      0,
			      NULL,
			      NULL);
   Status = ZwOpenSection(&PhysMemHandle, SECTION_ALL_ACCESS, 
			  &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Couldn't open \\Device\\PhysicalMemory\n");
	return FALSE;
     }

   /*
    * Map the BIOS and device registers into the address space
    */
   Offset.QuadPart = 0xa0000;
   ViewSize = 0x100000 - 0xa0000;
   BaseAddress = (PVOID)0xa0000;
   Status = NtMapViewOfSection(PhysMemHandle,
			       NtCurrentProcess(),
			       &BaseAddress,
			       0,
			       8192,
			       &Offset,
			       &ViewSize,
			       ViewUnmap,
			       0,
			       PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Couldn't map physical memory (%x)\n", Status);
	NtClose(PhysMemHandle);
	return FALSE;
     }
   NtClose(PhysMemHandle);
   if (BaseAddress != (PVOID)0xa0000)
     {
       DPRINT("Couldn't map physical memory at the right address "
		"(was %x)\n", BaseAddress);
       return FALSE;
     }

   /*
    * Map some memory to use for the non-BIOS parts of the v86 mode address
    * space
    */
   NonBiosBaseAddress = (PVOID)0x1;
   ViewSize = 0xa0000 - 0x1000;
   Status = NtAllocateVirtualMemory(NtCurrentProcess(),
				    &NonBiosBaseAddress,
				    0,
				    &ViewSize,
				    MEM_COMMIT,
				    PAGE_EXECUTE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("Failed to allocate virtual memory (Status %x)\n", Status);
       return FALSE;
     }
   if (NonBiosBaseAddress != (PVOID)0x0)
     {
       DPRINT("Failed to allocate virtual memory at right address "
		"(was %x)\n", NonBiosBaseAddress);
       return FALSE;
     }

   /*
    * Get the real mode IVT from the kernel
    */
   Status = NtVdmControl(0, IVT);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("NtVdmControl failed (status %x)\n", Status);
       return FALSE;
     }
   
   /*
    * Copy the real mode IVT into the right place
    */
   memcpy(start, IVT, 1024);
   
   /*
    * Get the BDA from the kernel
    */
   Status = NtVdmControl(1, BDA);
   if (!NT_SUCCESS(Status))
     {
       DPRINT("NtVdmControl failed (status %x)\n", Status);
       return FALSE;
     }
   
   /*
    * Copy the BDA into the right place
    */
   memcpy((PVOID)0x400, BDA, 256);

   return TRUE;
}


static BOOLEAN
InbvDeinitializeVideoAddressSpace(VOID)
{
  ULONG RegionSize;
  PUCHAR ViewBase;

  RegionSize = 0xa0000 - 0x1000;
  NtFreeVirtualMemory(NtCurrentProcess(),
    &NonBiosBaseAddress,
    &RegionSize,
    MEM_RELEASE);

  ViewBase = (PUCHAR) 0xa0000;
  ZwUnmapViewOfSection(NtCurrentProcess(), ViewBase);

 return TRUE;
}


static VOID
vgaPreCalc()
{
  ULONG j;

  for(j = 0; j < 80; j++)
  {
    maskbit[j * 8 + 0] = 128;
    maskbit[j * 8 + 1] = 64;
    maskbit[j * 8 + 2] = 32;
    maskbit[j * 8 + 3] = 16;
    maskbit[j * 8 + 4] = 8;
    maskbit[j * 8 + 5] = 4;
    maskbit[j * 8 + 6] = 2;
    maskbit[j * 8 + 7] = 1;
  }
  for(j = 0; j < 480; j++)
  {
    y80[j] = j * 80; /* 80 = 640 / 8 = Number of bytes per scanline */
  }
}


static VOID
InbvInitVGAMode(VOID)
{
  KV86M_REGISTERS Regs;
  NTSTATUS Status;
  ULONG i;

  vidmem = (char *)(0xd0000000 + 0xa0000);
  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = 0x0012;
  Status = Ke386CallBios(0x10, &Regs);
  assert(NT_SUCCESS(Status));

  /* Get VGA registers into the correct state */
  /* Reset the internal flip-flop. */
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  /* Write the 16 palette registers. */
  for (i = 0; i < 16; i++)
    {
      WRITE_PORT_UCHAR((PUCHAR)ATTRIB, i);
      WRITE_PORT_UCHAR((PUCHAR)ATTRIB, i);
    }
  /* Write the mode control register - graphics mode; 16 color DAC. */
  WRITE_PORT_UCHAR((PUCHAR)ATTRIB, 0x10);
  WRITE_PORT_UCHAR((PUCHAR)ATTRIB, 0x81);
  /* Write the color select register - select first 16 DAC registers. */
  WRITE_PORT_UCHAR((PUCHAR)ATTRIB, 0x14);
  WRITE_PORT_UCHAR((PUCHAR)ATTRIB, 0x00);

  /* Get the VGA into the mode we want to work with */
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);     /* Set */
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0);        /* the MASK */
  WRITE_PORT_USHORT((PUSHORT)0x3ce,0x0205); /* write mode = 2 (bits 0,1) read mode = 0  (bit 3) */
  (UCHAR) READ_REGISTER_UCHAR(vidmem);      /* Update bit buffer */
  WRITE_REGISTER_UCHAR(vidmem, 0);          /* Write the pixel */
  WRITE_PORT_UCHAR((PUCHAR)0x3ce,0x08);
  WRITE_PORT_UCHAR((PUCHAR)0x3cf,0xff);

  /* Set the PEL mask. */
  WRITE_PORT_UCHAR((PUCHAR)0x3c6, 0xff);

  /* Zero out video memory (clear a possibly trashed screen) */
  RtlZeroMemory(vidmem, 64000);

  vgaPreCalc();
}


BOOL
STDCALL
VidResetDisplay(VOID)
{
  /* 
     We are only using standard VGA facilities so we can rely on the HAL 'int10mode3'
     reset to cleanup the hardware state.
  */
  InGraphicsMode = FALSE;

  return FALSE;
}


VOID
STDCALL
VidCleanUp(VOID)
{
  /* 
     We are only using standard VGA facilities so we can rely on the HAL 'int10mode3'
     reset to cleanup the hardware state.
  */
  InGraphicsMode = FALSE;
}


static __inline__ VOID
InbvSetColor(int cindex, unsigned char red, unsigned char green, unsigned char blue)
{
  red = (red * 63) / 255;
  green = (green * 63) / 255;
  blue = (blue * 63) / 255;

  WRITE_PORT_UCHAR((PUCHAR)0x03c8, cindex);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, red);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, green);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  WRITE_PORT_UCHAR((PUCHAR)0x03c9, blue);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
}


static __inline__ VOID
InbvSetBlackPalette()
{
  register ULONG r = 0;

  /* Disable screen and enable palette access. */
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  WRITE_PORT_UCHAR((PUCHAR)0x3c0, 0x00);
  for (r = 0; r < 16; r++)
    {
      InbvSetColor(r, 0, 0, 0);
    }
  /* Enable screen and enable palette access. */
  (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
  WRITE_PORT_UCHAR((PUCHAR)0x3c0, 0x20);
}


static VOID
InbvDisplayBitmap(ULONG Width, ULONG Height, PCHAR ImageData)
{
  ULONG j,k,y;
  register ULONG i;
  register ULONG x;
  register ULONG c;

  k = 0;
  for (y = 0; y < Height; y++)
    {
      for (j = 0; j < 8; j++)
        {
          x = j;

          /*
           * Loop through the line and process every 8th pixel.
           * This way we can get a way with using the same bit mask
           * for several pixels and thus not need to do as much I/O
           * communication.
           */
          while (x < 640)
            {
              c = 0;

              if (x < Width)
                {
                  c = ImageData[k + x];
                  for (i = 1; i < 4; i++)
                    {
                      if (x + i*8 < Width)
                        {
                          c |= (ImageData[k + x + i * 8] << i * 8);
                        }
                    }
                }

	      InbvPutPixels(x, 479 - y, c);
              x += 8*4;
            }
        }
      k += Width;
    }
}


static VOID
InbvDisplayCompressedBitmap()
{
  PBITMAPV5HEADER bminfo;
  ULONG i,j,k;
  ULONG x,y;
  ULONG curx,cury;
  ULONG bfOffBits;
  ULONG clen;
  PCHAR ImageData;

  bminfo = (PBITMAPV5HEADER) &BootimageBitmap[0];
  DPRINT("bV5Size = %d\n", bminfo->bV5Size);
  DPRINT("bV5Width = %d\n", bminfo->bV5Width);
  DPRINT("bV5Height = %d\n", bminfo->bV5Height);
  DPRINT("bV5Planes = %d\n", bminfo->bV5Planes);
  DPRINT("bV5BitCount = %d\n", bminfo->bV5BitCount);
  DPRINT("bV5Compression = %d\n", bminfo->bV5Compression);
  DPRINT("bV5SizeImage = %d\n", bminfo->bV5SizeImage);
  DPRINT("bV5XPelsPerMeter = %d\n", bminfo->bV5XPelsPerMeter);
  DPRINT("bV5YPelsPerMeter = %d\n", bminfo->bV5YPelsPerMeter);
  DPRINT("bV5ClrUsed = %d\n", bminfo->bV5ClrUsed);
  DPRINT("bV5ClrImportant = %d\n", bminfo->bV5ClrImportant);

  bfOffBits = bminfo->bV5Size + bminfo->bV5ClrUsed * sizeof(RGBQUAD);
  DPRINT("bfOffBits = %d\n", bfOffBits);
  DPRINT("size of color indices = %d\n", bminfo->bV5ClrUsed * sizeof(RGBQUAD));
  DPRINT("first byte of data = %d\n", BootimageBitmap[bfOffBits]);

  InbvSetBlackPalette();

  ImageData = ExAllocatePool(NonPagedPool, bminfo->bV5Width * bminfo->bV5Height);
  RtlZeroMemory(ImageData, bminfo->bV5Width * bminfo->bV5Height);

  /*
   * ImageData has 1 pixel per byte.
   * bootimage has 2 pixels per byte.
   */

  if (bminfo->bV5Compression == 2)
    {
      k = 0;
      j = 0;
      while ((j < bminfo->bV5SizeImage) && (k < (ULONG) (bminfo->bV5Width * bminfo->bV5Height)))
        {
          unsigned char b;
    
          clen = BootimageBitmap[bfOffBits + j];
          j++;
    
          if (clen > 0)
            {
              /* Encoded mode */
    
              b = BootimageBitmap[bfOffBits + j];
              j++;
    
              for (i = 0; i < (clen / 2); i++)
                {
                  ImageData[k] = (b & 0xf0) >> 4;
                  k++;
                  ImageData[k] = b & 0xf;
                  k++;
                }
              if ((clen & 1) > 0)
              {
                ImageData[k] = (b & 0xf0) >> 4;
                k++;
              }
            }
          else
            {
              /* Absolute mode */
              b = BootimageBitmap[bfOffBits + j];
              j++;
    
              if (b == 0)
                {
                  /* End of line */
                }
              else if (b == 1)
                {
                  /* End of image */
                  break;
                }
              else if (b == 2)
                {
                  x = BootimageBitmap[bfOffBits + j];
                  j++;
                  y = BootimageBitmap[bfOffBits + j];
                  j++;
                  curx = k % bminfo->bV5Width;
                  cury = k / bminfo->bV5Width;
                  k = (cury + y) * bminfo->bV5Width + (curx + x);
                }
              else
                {
                  if ((j & 1) > 0)
                    {
                      DPRINT("Unaligned copy!\n");
                    }
    
                  clen = b;
                  for (i = 0; i < (clen / 2); i++)
                    {
                      b = BootimageBitmap[bfOffBits + j];
                      j++;
        
                      ImageData[k] = (b & 0xf0) >> 4;
                      k++;
                      ImageData[k] = b & 0xf;
                      k++;
                    }
                  if ((clen & 1) > 0)
                  {
                    b = BootimageBitmap[bfOffBits + j];
                    j++;
                    ImageData[k] = (b & 0xf0) >> 4;
                    k++;
                  }
                  /* Word align */
                  j += (j & 1);
                }
            }
        }

      InbvDisplayBitmap(bminfo->bV5Width, bminfo->bV5Height, ImageData);
    }
  else
    {
      DbgPrint("Warning boot image need to be compressed using RLE4\n");
    }

  ExFreePool(ImageData);
}


#define PALETTE_FADE_STEPS  20
#define PALETTE_FADE_TIME   20 * 10000 /* 20ms */

static VOID
InbvFadeUpPalette()
{
  PBITMAPV5HEADER bminfo;
  PRGBQUAD Palette;
  ULONG i;
  unsigned char r,g,b;
  register ULONG c;
  LARGE_INTEGER	Interval;
  FADER_PALETTE_ENTRY FaderPalette[16];
  FADER_PALETTE_ENTRY FaderPaletteDelta[16];

  RtlZeroMemory(&FaderPalette, sizeof(FaderPalette));
  RtlZeroMemory(&FaderPaletteDelta, sizeof(FaderPaletteDelta));

  bminfo = (PBITMAPV5HEADER) &BootimageBitmap[0]; //sizeof(BITMAPFILEHEADER)];
  Palette = (PRGBQUAD) &BootimageBitmap[/* sizeof(BITMAPFILEHEADER) + */ bminfo->bV5Size];

  for (i = 0; i < 16; i++)
    {
      if (i < bminfo->bV5ClrUsed)
        {
          FaderPaletteDelta[i].r = ((Palette[i].rgbRed << 8) / PALETTE_FADE_STEPS);
          FaderPaletteDelta[i].g = ((Palette[i].rgbGreen << 8) / PALETTE_FADE_STEPS);
          FaderPaletteDelta[i].b = ((Palette[i].rgbBlue << 8) / PALETTE_FADE_STEPS);
        }
    }

  for (i = 0; i < PALETTE_FADE_STEPS; i++)
    {
      /* Disable screen and enable palette access. */
      (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
      WRITE_PORT_UCHAR((PUCHAR)0x3c0, 0x00);
      for (c = 0; c < bminfo->bV5ClrUsed; c++)
        {
          /* Add the delta */
          FaderPalette[c].r += FaderPaletteDelta[c].r;
          FaderPalette[c].g += FaderPaletteDelta[c].g;
          FaderPalette[c].b += FaderPaletteDelta[c].b;

          /* Get the integer values */
          r = FaderPalette[c].r >> 8;
          g = FaderPalette[c].g >> 8;
          b = FaderPalette[c].b >> 8;

          /* Don't go too far */
          if (r > Palette[c].rgbRed)
            r = Palette[c].rgbRed;
          if (g > Palette[c].rgbGreen)
            g = Palette[c].rgbGreen;
          if (b > Palette[c].rgbBlue)
            b = Palette[c].rgbBlue;

          /* Update the hardware */
          InbvSetColor(c, r, g, b);
        }
      /* Enable screen and disable palette access. */
      (VOID)READ_PORT_UCHAR((PUCHAR)FEATURE);
      WRITE_PORT_UCHAR((PUCHAR)0x3c0, 0x20);
      /* Wait for a bit. */
      Interval.QuadPart = -PALETTE_FADE_TIME;
      KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }
}

static VOID STDCALL
InbvBitmapThreadMain(PVOID Ignored)
{
  if (InbvFindBootimage())
    {
      InbvDisplayCompressedBitmap();
      InbvFadeUpPalette();
    }
  else
    {
      DbgPrint("Warning: Cannot find boot image\n");
    }

  BitmapIsDrawn = TRUE;
}


BOOLEAN
STDCALL
VidIsBootDriverInstalled(VOID)
{
  return InGraphicsMode;
}


BOOLEAN
STDCALL
VidInitialize(VOID)
{
  NTSTATUS Status;

  if (!VideoAddressSpaceInitialized)
    {
      InbvInitializeVideoAddressSpace();
    }

  InbvInitVGAMode();

  InGraphicsMode = TRUE;

  BitmapIsDrawn = FALSE;
  
  Status = PsCreateSystemThread(&BitmapThreadHandle,
    THREAD_ALL_ACCESS,
    NULL,
    NULL,
    &BitmapThreadId,
    InbvBitmapThreadMain,
    NULL);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
  NtClose(BitmapThreadHandle);

  InbvDeinitializeVideoAddressSpace();
  VideoAddressSpaceInitialized = FALSE;

  return TRUE;
}

NTSTATUS STDCALL
VidDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS nErrCode;
  NTBOOTVID_FUNCTION_TABLE* FunctionTable;
 
  nErrCode = STATUS_SUCCESS;

  switch(piosStack->MajorFunction)
    {
      /* opening and closing handles to the device */
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
        break;

    case IRP_MJ_DEVICE_CONTROL:
      switch (piosStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_BOOTVID_INITIALIZE:
	  VidInitialize();
	  FunctionTable = (NTBOOTVID_FUNCTION_TABLE*)
	    Irp->AssociatedIrp.SystemBuffer;
	  FunctionTable->ResetDisplay = VidResetDisplay;
	  break;
	case IOCTL_BOOTVID_CLEANUP:
	  VidCleanUp();	  
	  break;
	default:
	  nErrCode = STATUS_NOT_IMPLEMENTED;
	  break;
	}
        break;

      /* unsupported operations */
      default:
        nErrCode = STATUS_NOT_IMPLEMENTED;
    }

  Irp->IoStatus.Status = nErrCode;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return nErrCode;
}


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
  PDEVICE_OBJECT BootVidDevice;
  UNICODE_STRING DeviceName;
  NTSTATUS Status;

  BootVidDriverObject = DriverObject;

  /* register driver routines */
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatch;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatch;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatch;
  DriverObject->DriverUnload = NULL;

  /* create device */
  RtlRosInitUnicodeStringFromLiteral(&DeviceName, L"\\Device\\BootVid");

  Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_BOOTVID,
                            0, FALSE, &BootVidDevice);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  BootVidDevice->Flags |= DO_BUFFERED_IO;

  return Status;
}
