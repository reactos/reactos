#include "../vgaddi.h"

static WORD PaletteBuffer[] = {
   16, 0, // 16 entries, start with 0
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static BYTE ColorBuffer[] = {
   16, // 16 entries
   0, 0,
   0,  // start with 0
   0x00, 0x00, 0x00, 0x00, // black
   0x2A, 0x00, 0x15, 0x00, // red
   0x00, 0x2A, 0x15, 0x00, // green
   0x2A, 0x2A, 0x15, 0x00, // brown
   0x00, 0x00, 0x2A, 0x00, // blue
   0x2A, 0x15, 0x2A, 0x00, // magenta
   0x15, 0x2A, 0x2A, 0x00, // cyan
   0x21, 0x22, 0x23, 0x00, // dark gray
   0x30, 0x31, 0x32, 0x00, // light gray
   0x3F, 0x00, 0x00, 0x00, // bright red
   0x00, 0x3F, 0x00, 0x00, // bright green
   0x3F, 0x3F, 0x00, 0x00, // bright yellow
   0x00, 0x00, 0x3F, 0x00, // bright blue
   0x3F, 0x00, 0x3F, 0x00, // bright magenta
   0x00, 0x3F, 0x3F, 0x00, // bright cyan
   0x3F, 0x3F, 0x3F, 0x00  // bright white
};

DWORD getAvailableModes(HANDLE Driver,
                        PVIDEO_MODE_INFORMATION *modeInformation,
                        DWORD *ModeSize)
{
   ULONG Temp;
   VIDEO_NUM_MODES modes;
   PVIDEO_MODE_INFORMATION VideoTemp;

   // get number of modes supported
   if (EngDeviceIoControl(Driver,
                          IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                          NULL,
                          0,
                          &modes,
                          sizeof(VIDEO_NUM_MODES),
                          &Temp))
   {
      // get modes failed
      return(0);
   }

   *ModeSize = modes.ModeInformationLength;

   // allocate buffer for the mini-port to write the modes in
   *modeInformation = (PVIDEO_MODE_INFORMATION)
                       EngAllocMem(0, modes.NumModes *
                                   modes.ModeInformationLength, ALLOC_TAG);

   if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
   {
      // couldn't allocate buffer
      return 0;
   }

   // Ask the mini-port to fill in the available modes.
   if (EngDeviceIoControl(Driver,
                          IOCTL_VIDEO_QUERY_AVAIL_MODES,
                          NULL,
                          0,
                          *modeInformation,
                          modes.NumModes * modes.ModeInformationLength,
                          &Temp))
   {
      // failed to query modes
      EngFreeMem(*modeInformation);
      *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

      return(0);
   }

   // Which modes supported by miniport driver are also suppoted by us, the
   // display driver

   Temp = modes.NumModes;
   VideoTemp = *modeInformation;

   // Reject mode if it's not 4 planes or not graphic or not 1 bits per pel
   while (Temp--)
   {
      if ((VideoTemp->NumberOfPlanes != 4 ) ||
          !(VideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
          (VideoTemp->BitsPerPlane != 1) ||
          BROKEN_RASTERS(VideoTemp->ScreenStride,
                         VideoTemp->VisScreenHeight))

      {
         VideoTemp->Length = 0;
      }

      VideoTemp = (PVIDEO_MODE_INFORMATION)
          (((PUCHAR)VideoTemp) + modes.ModeInformationLength);
   }

   return modes.NumModes;
}

BOOL InitVGA(PPDEV ppdev, BOOL bFirst)
{
   UINT ReturnedDataLength;
   VIDEO_MEMORY VideoMemory;
   VIDEO_MEMORY_INFORMATION VideoMemoryInfo;

char* vidmem;

   ppdev->ModeNum = 12;

   // Set the mode that was requested
   if (EngDeviceIoControl(ppdev->KMDriver,
                          IOCTL_VIDEO_SET_CURRENT_MODE,
                          &ppdev->ModeNum,
                          sizeof(VIDEO_MODE),
                          NULL,
                          0,
                          &ReturnedDataLength)) {
      return(FALSE);
   }

   // set up internal palette
   if (EngDeviceIoControl(ppdev->KMDriver,
                          IOCTL_VIDEO_SET_PALETTE_REGISTERS,
                          (PVOID) PaletteBuffer,
                          sizeof (PaletteBuffer),
                          NULL,
                          0,
                          &ReturnedDataLength)) {
      return(FALSE);
   }

   // set up the DAC
   if (EngDeviceIoControl(ppdev->KMDriver,
                           IOCTL_VIDEO_SET_COLOR_REGISTERS,
                           (PVOID) ColorBuffer,
                           sizeof (ColorBuffer),
                           NULL,
                           0,
                           &ReturnedDataLength)) {
      return(FALSE);
   }

/*

gotta fix this up.. it prevents drawing to vidmem right now

    if (bFirst) {
      // map video memory into virtual memory
      VideoMemory.RequestedVirtualAddress = NULL;

      if (EngDeviceIoControl(ppdev->KMDriver,
                             IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                             (PVOID) &VideoMemory,
                             sizeof (VIDEO_MEMORY),
                             (PVOID) &VideoMemoryInfo,
                             sizeof (VideoMemoryInfo),
                             &ReturnedDataLength)) {
         // Failed to map to virtual memory
         return (FALSE);
      }

      ppdev->fbScreen = VideoMemoryInfo.FrameBufferBase;
   }
*/
   return TRUE;
}
