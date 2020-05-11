/*
 * PROJECT:     ReactOS Console Text-Mode Device Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Driver Management Functions.
 * COPYRIGHT:   Copyright 1999 Boudewijn Dekker
 *              Copyright 1999-2019 Eric Kohl
 *              Copyright 2006 Filip Navara
 *              Copyright 2019 Hermes Belusca-Maito
 */

/* INCLUDES ******************************************************************/

#include "blue.h"
#include <ndk/inbvfuncs.h>

#define NDEBUG
#include <debug.h>

/* NOTES ******************************************************************/
/*
 *  [[character][attribute]][[character][attribute]]....
 */

/* TYPEDEFS ***************************************************************/

typedef struct _DEVICE_EXTENSION
{
    PUCHAR  VideoMemory;    /* Pointer to video memory */
    SIZE_T  VideoMemorySize;
    BOOLEAN Enabled;
    PUCHAR  ScreenBuffer;   /* Pointer to screenbuffer */
    SIZE_T  ScreenBufferSize;
    ULONG   CursorSize;
    INT     CursorVisible;
    USHORT  CharAttribute;
    ULONG   Mode;
    UCHAR   ScanLines;  /* Height of a text line */
    USHORT  Rows;       /* Number of rows        */
    USHORT  Columns;    /* Number of columns     */
    USHORT  CursorX, CursorY; /* Cursor position */
    PUCHAR  FontBitfield; /* Specifies the font. If NULL, use CodePage */
    ULONG   CodePage;   /* Specifies the font associated to this code page */
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

/* INBV MANAGEMENT FUNCTIONS **************************************************/

static BOOLEAN
ScrResetScreen(
    _In_ PDEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN FullReset,
    _In_ BOOLEAN Enable);

static PDEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;
static HANDLE InbvThreadHandle = NULL;
static BOOLEAN InbvMonitoring = FALSE;

/*
 * Reinitialize the display to base VGA mode.
 *
 * Returns TRUE if it completely resets the adapter to the given character mode.
 * Returns FALSE otherwise, indicating that the HAL should perform the VGA mode
 * reset itself after HwVidResetHw() returns control.
 *
 * This callback has been registered with InbvNotifyDisplayOwnershipLost()
 * and is called by InbvAcquireDisplayOwnership(), typically when the bugcheck
 * code regains display access. Therefore this routine can be called at any
 * IRQL, and in particular at IRQL = HIGH_LEVEL. This routine must also reside
 * completely in non-paged pool, and cannot perform the following actions:
 * Allocate memory, access pageable memory, use any synchronization mechanisms
 * or call any routine that must execute at IRQL = DISPATCH_LEVEL or below.
 */
static BOOLEAN
NTAPI
ScrResetDisplayParametersEx(
    _In_ ULONG Columns,
    _In_ ULONG Rows,
    _In_ BOOLEAN CalledByInbv)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* Bail out early if we don't have any resettable adapter */
    if (!ResetDisplayParametersDeviceExtension)
        return FALSE; // No adapter found: request HAL to perform a full reset.

    /*
     * If we have been unexpectedly called via a callback from
     * InbvAcquireDisplayOwnership(), start monitoring INBV.
     */
    if (CalledByInbv)
        InbvMonitoring = TRUE;

    DeviceExtension = ResetDisplayParametersDeviceExtension;
    ASSERT(DeviceExtension);

    /* Disable the screen but don't reset all screen settings (OK at high IRQL) */
    return ScrResetScreen(DeviceExtension, FALSE, FALSE);
}

/* This callback is registered with InbvNotifyDisplayOwnershipLost() */
static BOOLEAN
NTAPI
ScrResetDisplayParameters(
    _In_ ULONG Columns,
    _In_ ULONG Rows)
{
    /* Call the extended function, specifying we were called by INBV */
    return ScrResetDisplayParametersEx(Columns, Rows, TRUE);
}

/*
 * (Adapted for ReactOS/Win2k3 from an original comment
 *  by Gé van Geldorp, June 2003, r4937)
 *
 * DISPLAY OWNERSHIP
 *
 * So, who owns the physical display and is allowed to write to it?
 *
 * In NT 5.x (Win2k/Win2k3), upon boot INBV/BootVid owns the display, unless
 * /NOGUIBOOT has been specified in the boot command line. Later in the boot
 * sequence, WIN32K.SYS opens the DISPLAY device. This open call ends up in
 * VIDEOPRT.SYS. This component takes ownership of the display by calling
 * InbvNotifyDisplayOwnershipLost() -- effectively telling INBV to release
 * ownership of the display it previously had. From that moment on, the display
 * is owned by that component and can be switched to graphics mode. The display
 * is not supposed to return to text mode, except in case of a bugcheck.
 * The bugcheck code calls InbvAcquireDisplayOwnership() so as to make INBV
 * re-take display ownership, and calls back the function previously registered
 * by VIDEOPRT.SYS with InbvNotifyDisplayOwnershipLost(). After the bugcheck,
 * execution is halted. So, under NT, the only possible sequence of display
 * modes is text mode -> graphics mode -> text mode (the latter hopefully
 * happening very infrequently).
 *
 * In ReactOS things are a little bit different. We want to have a functional
 * interactive text mode. We should be able to switch back and forth from
 * text mode to graphics mode when a GUI app is started and then finished.
 * Also, when the system bugchecks in graphics mode we want to switch back to
 * text mode and show the bugcheck information. Last but not least, when using
 * KDBG in /DEBUGPORT=SCREEN mode, breaking into the debugger would trigger a
 * switch to text mode, and the user would expect that by continuing execution
 * a switch back to graphics mode is done.
 */
static VOID
NTAPI
InbvMonitorThread(
    _In_ PVOID Context)
{
    LARGE_INTEGER Delay;
    USHORT i;

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    while (TRUE)
    {
        /*
         * During one second, check the INBV status each 100 milliseconds,
         * then revert to 1 second delay.
         */
        i = 10;
        Delay.QuadPart = (LONGLONG)-100*1000*10; // 100 millisecond delay
        while (!InbvMonitoring)
        {
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);

            if ((i > 0) && (--i == 0))
                Delay.QuadPart = (LONGLONG)-1*1000*1000*10; // 1 second delay
        }

        /*
         * Loop while the display is owned by INBV. We cannot do anything else
         * than polling since INBV does not offer a proper notification system.
         *
         * During one second, check the INBV status each 100 milliseconds,
         * then revert to 1 second delay.
         */
        i = 10;
        Delay.QuadPart = (LONGLONG)-100*1000*10; // 100 millisecond delay
        while (InbvCheckDisplayOwnership())
        {
            KeDelayExecutionThread(KernelMode, FALSE, &Delay);

            if ((i > 0) && (--i == 0))
                Delay.QuadPart = (LONGLONG)-1*1000*1000*10; // 1 second delay
        }

        /* Reset the monitoring */
        InbvMonitoring = FALSE;

        /*
         * Somebody released INBV display ownership, usually by invoking
         * InbvNotifyDisplayOwnershipLost(). However the caller of this
         * function certainly specified a different callback than ours.
         * As we are going to be the only owner of the active display,
         * we need to re-register our own display reset callback.
         */
        InbvNotifyDisplayOwnershipLost(ScrResetDisplayParameters);

        /* Re-enable the screen, keeping the original screen settings */
        if (ResetDisplayParametersDeviceExtension)
            ScrResetScreen(ResetDisplayParametersDeviceExtension, FALSE, TRUE);
    }

    // FIXME: See ScrInbvCleanup().
    // PsTerminateSystemThread(STATUS_SUCCESS);
}

static NTSTATUS
ScrInbvInitialize(VOID)
{
    /* Create the INBV monitoring thread if needed */
    if (!InbvThreadHandle)
    {
        NTSTATUS Status;
        OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, OBJ_KERNEL_HANDLE);

        Status = PsCreateSystemThread(&InbvThreadHandle,
                                      0,
                                      &ObjectAttributes,
                                      NULL,
                                      NULL,
                                      InbvMonitorThread,
                                      NULL);
        if (!NT_SUCCESS(Status))
            InbvThreadHandle = NULL;
    }

    /* Re-register the display reset callback with INBV */
    InbvNotifyDisplayOwnershipLost(ScrResetDisplayParameters);

    return STATUS_SUCCESS;
}

static NTSTATUS
ScrInbvCleanup(VOID)
{
    // HANDLE ThreadHandle;

    // ResetDisplayParametersDeviceExtension = NULL;
    if (ResetDisplayParametersDeviceExtension)
    {
        InbvNotifyDisplayOwnershipLost(NULL);
        ScrResetDisplayParametersEx(80, 50, FALSE);
        // or InbvAcquireDisplayOwnership(); ?
    }

#if 0
    // TODO: Find the best way to communicate the request.
    /* Signal the INBV monitoring thread and wait for it to terminate */
    ThreadHandle = InterlockedExchangePointer((PVOID*)&InbvThreadHandle, NULL);
    if (ThreadHandle)
    {
        ZwWaitForSingleObject(ThreadHandle, Executive, KernelMode, FALSE, NULL);
        /* Close its handle */
        ObCloseHandle(ThreadHandle, KernelMode);
    }
#endif

    return STATUS_SUCCESS;
}

/* FUNCTIONS **************************************************************/

static VOID
FASTCALL
ScrSetRegisters(const VGA_REGISTERS *Registers)
{
    UINT32 i;

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

    /* Set the PEL mask */
    WRITE_PORT_UCHAR(PELMASK, 0xff);
}

static VOID
FASTCALL
ScrSetCursor(
    _In_ PDEVICE_EXTENSION DeviceExtension)
{
    ULONG Offset;

    if (!DeviceExtension->VideoMemory)
        return;

    Offset = (DeviceExtension->CursorY * DeviceExtension->Columns) + DeviceExtension->CursorX;

    _disable();
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORPOSLO);
    WRITE_PORT_UCHAR(CRTC_DATA, Offset);
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORPOSHI);
    WRITE_PORT_UCHAR(CRTC_DATA, Offset >> 8);
    _enable();
}

static VOID
FASTCALL
ScrSetCursorShape(
    _In_ PDEVICE_EXTENSION DeviceExtension)
{
    ULONG size, height;
    UCHAR data, value;

    if (!DeviceExtension->VideoMemory)
        return;

    height = DeviceExtension->ScanLines;
    data = (DeviceExtension->CursorVisible) ? 0x00 : 0x20;

    size = (DeviceExtension->CursorSize * height) / 100;
    if (size < 1)
        size = 1;

    data |= (UCHAR)(height - size);

    _disable();
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORSTART);
    WRITE_PORT_UCHAR(CRTC_DATA, data);
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSOREND);
    value = READ_PORT_UCHAR(CRTC_DATA) & 0xE0;
    WRITE_PORT_UCHAR(CRTC_DATA, value | (height - 1));
    _enable();
}

static VOID
FASTCALL
ScrAcquireOwnership(
    _In_ PDEVICE_EXTENSION DeviceExtension)
{
    UCHAR data, value;
    ULONG offset;
    ULONG Index;

    _disable();

    ScrSetRegisters(&VidpMode3Regs);

    /* Disable screen and enable palette access */
    READ_PORT_UCHAR(STATUS);
    WRITE_PORT_UCHAR(ATTRIB, 0x00);

    for (Index = 0; Index < sizeof(DefaultPalette) / 3; Index++)
    {
        WRITE_PORT_UCHAR(PELINDEX, Index);
        WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3] >> 2);
        WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3 + 1] >> 2);
        WRITE_PORT_UCHAR(PELDATA, DefaultPalette[Index * 3 + 2] >> 2);
    }

    /* Enable screen and disable palette access */
    READ_PORT_UCHAR(STATUS);
    WRITE_PORT_UCHAR(ATTRIB, 0x20);

    /* Switch blinking characters off */
    READ_PORT_UCHAR(ATTRC_INPST1);
    value = READ_PORT_UCHAR(ATTRC_WRITEREG);
    WRITE_PORT_UCHAR(ATTRC_WRITEREG, 0x10);
    data  = READ_PORT_UCHAR(ATTRC_READREG);
    data  = data & ~0x08;
    WRITE_PORT_UCHAR(ATTRC_WRITEREG, data);
    WRITE_PORT_UCHAR(ATTRC_WRITEREG, value);
    READ_PORT_UCHAR(ATTRC_INPST1);

    /* Read screen information from CRT controller */
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_COLUMNS);
    DeviceExtension->Columns = READ_PORT_UCHAR(CRTC_DATA) + 1;
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_ROWS);
    DeviceExtension->Rows = READ_PORT_UCHAR(CRTC_DATA);
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_OVERFLOW);
    data = READ_PORT_UCHAR(CRTC_DATA);
    DeviceExtension->Rows |= (((data & 0x02) << 7) | ((data & 0x40) << 3));
    DeviceExtension->Rows++;
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_SCANLINES);
    DeviceExtension->ScanLines = (READ_PORT_UCHAR(CRTC_DATA) & 0x1F) + 1;

    /* Retrieve the current output cursor position */
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORPOSLO);
    offset = READ_PORT_UCHAR(CRTC_DATA);
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset += (READ_PORT_UCHAR(CRTC_DATA) << 8);

    /* Show blinking cursor */
    // FIXME: cursor block? Call ScrSetCursorShape() instead?
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSORSTART);
    WRITE_PORT_UCHAR(CRTC_DATA, (DeviceExtension->ScanLines - 1) & 0x1F);
    WRITE_PORT_UCHAR(CRTC_COMMAND, CRTC_CURSOREND);
    data = READ_PORT_UCHAR(CRTC_DATA) & 0xE0;
    WRITE_PORT_UCHAR(CRTC_DATA,
                     data | ((DeviceExtension->ScanLines - 1) & 0x1F));

    _enable();

    /* Calculate number of text rows */
    DeviceExtension->Rows = DeviceExtension->Rows / DeviceExtension->ScanLines;

    /* Set the cursor position, clipping it to the screen */
    DeviceExtension->CursorX = (USHORT)(offset % DeviceExtension->Columns);
    DeviceExtension->CursorY = (USHORT)(offset / DeviceExtension->Columns);
    // DeviceExtension->CursorX = min(max(DeviceExtension->CursorX, 0), DeviceExtension->Columns - 1);
    DeviceExtension->CursorY = min(max(DeviceExtension->CursorY, 0), DeviceExtension->Rows - 1);

    if (DeviceExtension->FontBitfield)
    {
        ScrSetFont(DeviceExtension->FontBitfield);
    }
    else
    {
        /* Upload a default font for the current codepage */
        ScrLoadFontTable(DeviceExtension->CodePage);
    }

    DPRINT("%d Columns  %d Rows %d Scanlines\n",
           DeviceExtension->Columns,
           DeviceExtension->Rows,
           DeviceExtension->ScanLines);
}

static BOOLEAN
ScrResetScreen(
    _In_ PDEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN FullReset,
    _In_ BOOLEAN Enable)
{
#define FOREGROUND_LIGHTGRAY (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

    PHYSICAL_ADDRESS BaseAddress;

    /* Allow resets to the same state only for full resets */
    if (!FullReset && (Enable == DeviceExtension->Enabled))
        return FALSE; // STATUS_INVALID_PARAMETER; STATUS_INVALID_DEVICE_REQUEST;

    if (FullReset)
    {
        DeviceExtension->CursorSize    = 5; /* FIXME: value correct?? */
        DeviceExtension->CursorVisible = TRUE;

        if (DeviceExtension->FontBitfield)
        {
            ExFreePoolWithTag(DeviceExtension->FontBitfield, TAG_BLUE);
            DeviceExtension->FontBitfield = NULL;
        }

        /* More initialization */
        DeviceExtension->CharAttribute = BACKGROUND_BLUE | FOREGROUND_LIGHTGRAY;
        DeviceExtension->Mode = ENABLE_PROCESSED_OUTPUT |
                                ENABLE_WRAP_AT_EOL_OUTPUT;
        DeviceExtension->CodePage = 437; /* Use default codepage */
    }

    if (Enable)
    {
        ScrAcquireOwnership(DeviceExtension);

        if (FullReset)
        {
            /*
             * Fully reset the screen and all its settings.
             */

            /* Unmap any previously mapped video memory */
            if (DeviceExtension->VideoMemory)
            {
                ASSERT(DeviceExtension->VideoMemorySize != 0);
                MmUnmapIoSpace(DeviceExtension->VideoMemory, DeviceExtension->VideoMemorySize);
            }
            DeviceExtension->VideoMemory = NULL;
            DeviceExtension->VideoMemorySize = 0;

            /* Free any previously allocated backup screenbuffer */
            if (DeviceExtension->ScreenBuffer)
            {
                ASSERT(DeviceExtension->ScreenBufferSize != 0);
                ExFreePoolWithTag(DeviceExtension->ScreenBuffer, TAG_BLUE);
            }
            DeviceExtension->ScreenBuffer = NULL;
            DeviceExtension->ScreenBufferSize = 0;

            /* Get a pointer to the video memory */
            DeviceExtension->VideoMemorySize = DeviceExtension->Rows * DeviceExtension->Columns * 2;
            if (DeviceExtension->VideoMemorySize == 0)
                return FALSE; // STATUS_INVALID_VIEW_SIZE; STATUS_MAPPED_FILE_SIZE_ZERO;

            /* Map the video memory */
            BaseAddress.QuadPart = VIDMEM_BASE;
            DeviceExtension->VideoMemory =
                (PUCHAR)MmMapIoSpace(BaseAddress, DeviceExtension->VideoMemorySize, MmNonCached);
            if (!DeviceExtension->VideoMemory)
            {
                DeviceExtension->VideoMemorySize = 0;
                return FALSE; // STATUS_NONE_MAPPED; STATUS_NOT_MAPPED_VIEW; STATUS_CONFLICTING_ADDRESSES;
            }

            /* Initialize the backup screenbuffer in non-paged pool (must be accessible at high IRQL) */
            DeviceExtension->ScreenBufferSize = DeviceExtension->VideoMemorySize;
            DeviceExtension->ScreenBuffer =
                (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, DeviceExtension->ScreenBufferSize, TAG_BLUE);
            if (!DeviceExtension->ScreenBuffer)
            {
                DPRINT1("Could not allocate screenbuffer, ignore...\n");
                DeviceExtension->ScreenBufferSize = 0;
            }

            /* (Re-)initialize INBV */
            ScrInbvInitialize();
        }
        else
        {
            /*
             * Restore the previously disabled screen.
             */

            /* Restore the snapshot of the video memory from the backup screenbuffer */
            if (DeviceExtension->ScreenBuffer)
            {
                ASSERT(DeviceExtension->VideoMemory);
                ASSERT(DeviceExtension->ScreenBuffer);
                ASSERT(DeviceExtension->ScreenBufferSize != 0);
                ASSERT(DeviceExtension->VideoMemorySize == DeviceExtension->ScreenBufferSize);

                RtlCopyMemory(DeviceExtension->VideoMemory,
                              DeviceExtension->ScreenBuffer,
                              DeviceExtension->VideoMemorySize);
            }

            /* Restore the cursor state */
            ScrSetCursor(DeviceExtension);
            ScrSetCursorShape(DeviceExtension);
        }
        DeviceExtension->Enabled = TRUE;
    }
    else
    {
        DeviceExtension->Enabled = FALSE;
        if (FullReset)
        {
            /*
             * Fully disable the screen and reset all its settings.
             */

            /* Clean INBV up */
            ScrInbvCleanup();

            /* Unmap any previously mapped video memory */
            if (DeviceExtension->VideoMemory)
            {
                ASSERT(DeviceExtension->VideoMemorySize != 0);
                MmUnmapIoSpace(DeviceExtension->VideoMemory, DeviceExtension->VideoMemorySize);
            }
            DeviceExtension->VideoMemory = NULL;
            DeviceExtension->VideoMemorySize = 0;

            /* Free any previously allocated backup screenbuffer */
            if (DeviceExtension->ScreenBuffer)
            {
                ASSERT(DeviceExtension->ScreenBufferSize != 0);
                ExFreePoolWithTag(DeviceExtension->ScreenBuffer, TAG_BLUE);
            }
            DeviceExtension->ScreenBuffer = NULL;
            DeviceExtension->ScreenBufferSize = 0;

            /* Store dummy values */
            DeviceExtension->Columns = 1;
            DeviceExtension->Rows = 1;
            DeviceExtension->ScanLines = 1;
        }
        else
        {
            /*
             * Partially disable the screen such that it can be restored later.
             */

            /* Take a snapshot of the video memory into the backup screenbuffer */
            if (DeviceExtension->ScreenBuffer)
            {
                ASSERT(DeviceExtension->VideoMemory);
                ASSERT(DeviceExtension->ScreenBuffer);
                ASSERT(DeviceExtension->ScreenBufferSize != 0);
                ASSERT(DeviceExtension->VideoMemorySize == DeviceExtension->ScreenBufferSize);

                RtlCopyMemory(DeviceExtension->ScreenBuffer,
                              DeviceExtension->VideoMemory,
                              DeviceExtension->VideoMemorySize);
            }
        }
    }

    return TRUE; // STATUS_SUCCESS;
}

static DRIVER_DISPATCH ScrCreateClose;
static NTSTATUS
NTAPI
ScrCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    if (stk->MajorFunction == IRP_MJ_CREATE)
        Irp->IoStatus.Information = FILE_OPENED;
    // else: IRP_MJ_CLOSE

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static DRIVER_DISPATCH ScrWrite;
static NTSTATUS
NTAPI
ScrWrite(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PCHAR pch = Irp->UserBuffer;
    PUCHAR vidmem;
    ULONG i;
    ULONG j, offset;
    USHORT cursorx, cursory;
    USHORT rows, columns;
    BOOLEAN processed = !!(DeviceExtension->Mode & ENABLE_PROCESSED_OUTPUT);

    if (!DeviceExtension->Enabled || !DeviceExtension->VideoMemory)
    {
        /* Display is not enabled, we're not allowed to touch it */
        Status = STATUS_SUCCESS;

        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    vidmem  = DeviceExtension->VideoMemory;
    rows    = DeviceExtension->Rows;
    columns = DeviceExtension->Columns;
    cursorx = DeviceExtension->CursorX;
    cursory = DeviceExtension->CursorY;

    if (!processed)
    {
        /* Raw output mode */

        /* Calculate the offset from the cursor position */
        offset = cursorx + cursory * columns;

        // FIXME: Does the buffer only contains chars? or chars + attributes?
        // FIXME2: Fix buffer overflow.
        RtlCopyMemory(&vidmem[offset * 2], pch, stk->Parameters.Write.Length);
        offset += (stk->Parameters.Write.Length / 2);

        /* Set the cursor position, clipping it to the screen */
        cursorx = (USHORT)(offset % columns);
        cursory = (USHORT)(offset / columns);
        // cursorx = min(max(cursorx, 0), columns - 1);
        cursory = min(max(cursory, 0), rows - 1);
    }
    else
    {
        /* Cooked output mode */
        for (i = 0; i < stk->Parameters.Write.Length; i++, pch++)
        {
            switch (*pch)
            {
            case '\b':
            {
                if (cursorx > 0)
                {
                    cursorx--;
                }
                else if (cursory > 0)
                {
                    cursory--;
                    cursorx = columns - 1;
                }
                offset = cursorx + cursory * columns;
                vidmem[offset * 2] = ' ';
                vidmem[offset * 2 + 1] = (char)DeviceExtension->CharAttribute;
                break;
            }

            case '\n':
                cursory++;
                /* Fall back */
            case '\r':
                cursorx = 0;
                break;

            case '\t':
            {
                offset = TAB_WIDTH - (cursorx % TAB_WIDTH);
                while (offset--)
                {
                    vidmem[(cursorx + cursory * columns) * 2] = ' ';
                    cursorx++;
                    if (cursorx >= columns)
                    {
                        cursorx = 0;
                        cursory++;
                        /* We jumped to the next line, stop there */
                        break;
                    }
                }
                break;
            }

            default:
            {
                offset = cursorx + cursory * columns;
                vidmem[offset * 2] = *pch;
                vidmem[offset * 2 + 1] = (char)DeviceExtension->CharAttribute;
                cursorx++;
                if (cursorx >= columns)
                {
                    cursorx = 0;
                    cursory++;
                }
                break;
            }
            }

            /* Scroll up the contents of the screen if we are at the end */
            if (cursory >= rows)
            {
                PUSHORT LinePtr;

                RtlCopyMemory(vidmem,
                              &vidmem[columns * 2],
                              columns * (rows - 1) * 2);

                LinePtr = (PUSHORT)&vidmem[columns * (rows - 1) * 2];

                for (j = 0; j < columns; j++)
                {
                    LinePtr[j] = DeviceExtension->CharAttribute << 8;
                }
                cursory = rows - 1;
                for (j = 0; j < columns; j++)
                {
                    offset = j + cursory * columns;
                    vidmem[offset * 2] = ' ';
                    vidmem[offset * 2 + 1] = (char)DeviceExtension->CharAttribute;
                }
            }
        }
    }

    /* Set the cursor position */
    ASSERT((0 <= cursorx) && (cursorx < DeviceExtension->Columns));
    ASSERT((0 <= cursory) && (cursory < DeviceExtension->Rows));
    DeviceExtension->CursorX = cursorx;
    DeviceExtension->CursorY = cursory;
    ScrSetCursor(DeviceExtension);

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_VIDEO_INCREMENT);

    return Status;
}

static DRIVER_DISPATCH ScrIoControl;
static NTSTATUS
NTAPI
ScrIoControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    switch (stk->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_CONSOLE_RESET_SCREEN:
        {
            BOOLEAN Enable;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            Enable = !!*(PULONG)Irp->AssociatedIrp.SystemBuffer;

            /* Fully enable or disable the screen */
            Status = (ScrResetScreen(DeviceExtension, TRUE, Enable)
                        ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
            Irp->IoStatus.Information = 0;
            break;
        }

        case IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO:
        {
            PCONSOLE_SCREEN_BUFFER_INFO pcsbi;
            USHORT rows = DeviceExtension->Rows;
            USHORT columns = DeviceExtension->Columns;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CONSOLE_SCREEN_BUFFER_INFO))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(pcsbi, sizeof(CONSOLE_SCREEN_BUFFER_INFO));

            pcsbi->dwSize.X = columns;
            pcsbi->dwSize.Y = rows;

            pcsbi->dwCursorPosition.X = DeviceExtension->CursorX;
            pcsbi->dwCursorPosition.Y = DeviceExtension->CursorY;

            pcsbi->wAttributes = DeviceExtension->CharAttribute;

            pcsbi->srWindow.Left   = 0;
            pcsbi->srWindow.Right  = columns - 1;
            pcsbi->srWindow.Top    = 0;
            pcsbi->srWindow.Bottom = rows - 1;

            pcsbi->dwMaximumWindowSize.X = columns;
            pcsbi->dwMaximumWindowSize.Y = rows;

            Irp->IoStatus.Information = sizeof(CONSOLE_SCREEN_BUFFER_INFO);
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO:
        {
            PCONSOLE_SCREEN_BUFFER_INFO pcsbi;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONSOLE_SCREEN_BUFFER_INFO))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcsbi = (PCONSOLE_SCREEN_BUFFER_INFO)Irp->AssociatedIrp.SystemBuffer;

            if ( pcsbi->dwCursorPosition.X < 0 || pcsbi->dwCursorPosition.X >= DeviceExtension->Columns ||
                 pcsbi->dwCursorPosition.Y < 0 || pcsbi->dwCursorPosition.Y >= DeviceExtension->Rows )
            {
                Irp->IoStatus.Information = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            DeviceExtension->CharAttribute = pcsbi->wAttributes;

            /* Set the cursor position */
            ASSERT((0 <= pcsbi->dwCursorPosition.X) && (pcsbi->dwCursorPosition.X < DeviceExtension->Columns));
            ASSERT((0 <= pcsbi->dwCursorPosition.Y) && (pcsbi->dwCursorPosition.Y < DeviceExtension->Rows));
            DeviceExtension->CursorX = pcsbi->dwCursorPosition.X;
            DeviceExtension->CursorY = pcsbi->dwCursorPosition.Y;
            if (DeviceExtension->Enabled)
                ScrSetCursor(DeviceExtension);

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_GET_CURSOR_INFO:
        {
            PCONSOLE_CURSOR_INFO pcci;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CONSOLE_CURSOR_INFO))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcci = (PCONSOLE_CURSOR_INFO)Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(pcci, sizeof(CONSOLE_CURSOR_INFO));

            pcci->dwSize = DeviceExtension->CursorSize;
            pcci->bVisible = DeviceExtension->CursorVisible;

            Irp->IoStatus.Information = sizeof(CONSOLE_CURSOR_INFO);
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_SET_CURSOR_INFO:
        {
            PCONSOLE_CURSOR_INFO pcci;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONSOLE_CURSOR_INFO))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcci = (PCONSOLE_CURSOR_INFO)Irp->AssociatedIrp.SystemBuffer;

            DeviceExtension->CursorSize = pcci->dwSize;
            DeviceExtension->CursorVisible = pcci->bVisible;
            if (DeviceExtension->Enabled)
                ScrSetCursorShape(DeviceExtension);

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_GET_MODE:
        {
            PCONSOLE_MODE pcm;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CONSOLE_MODE))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(pcm, sizeof(CONSOLE_MODE));

            pcm->dwMode = DeviceExtension->Mode;

            Irp->IoStatus.Information = sizeof(CONSOLE_MODE);
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_SET_MODE:
        {
            PCONSOLE_MODE pcm;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONSOLE_MODE))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            pcm = (PCONSOLE_MODE)Irp->AssociatedIrp.SystemBuffer;
            DeviceExtension->Mode = pcm->dwMode;

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE:
        {
            POUTPUT_ATTRIBUTE Buf;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            /* Validate input and output buffers */
            if (stk->Parameters.DeviceIoControl.InputBufferLength  < sizeof(OUTPUT_ATTRIBUTE) ||
                stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(OUTPUT_ATTRIBUTE))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            Buf = (POUTPUT_ATTRIBUTE)Irp->AssociatedIrp.SystemBuffer;
            nMaxLength = Buf->nLength;

            Buf->dwTransfered = 0;
            Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);

            if ( Buf->dwCoord.X < 0 || Buf->dwCoord.X >= DeviceExtension->Columns ||
                 Buf->dwCoord.Y < 0 || Buf->dwCoord.Y >= DeviceExtension->Rows    ||
                 nMaxLength == 0 )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.X + Buf->dwCoord.Y * DeviceExtension->Columns) * 2 + 1;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - Buf->dwCoord.Y)
                                    * DeviceExtension->Columns - Buf->dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++)
                {
                    vidmem[offset + (dwCount * 2)] = (char)Buf->wAttribute;
                }
                Buf->dwTransfered = dwCount;
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE:
        {
            POUTPUT_ATTRIBUTE Buf;
            PUSHORT pAttr;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(OUTPUT_ATTRIBUTE))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            Buf = (POUTPUT_ATTRIBUTE)Irp->AssociatedIrp.SystemBuffer;
            Irp->IoStatus.Information = 0;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength == 0)
            {
                Status = STATUS_SUCCESS;
                break;
            }
            ASSERT(Irp->MdlAddress);
            pAttr = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (pAttr == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if ( Buf->dwCoord.X < 0 || Buf->dwCoord.X >= DeviceExtension->Columns ||
                 Buf->dwCoord.Y < 0 || Buf->dwCoord.Y >= DeviceExtension->Rows )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            nMaxLength = stk->Parameters.DeviceIoControl.OutputBufferLength;
            nMaxLength /= sizeof(USHORT);

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.X + Buf->dwCoord.Y * DeviceExtension->Columns) * 2 + 1;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - Buf->dwCoord.Y)
                                    * DeviceExtension->Columns - Buf->dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++, pAttr++)
                {
                    *((PCHAR)pAttr) = vidmem[offset + (dwCount * 2)];
                }
                Irp->IoStatus.Information = dwCount * sizeof(USHORT);
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE:
        {
            COORD dwCoord;
            PCOORD pCoord;
            PUSHORT pAttr;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            //
            // NOTE: For whatever reason no OUTPUT_ATTRIBUTE structure
            // is used for this IOCTL.
            //

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(COORD))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->MdlAddress);
            pCoord = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (pCoord == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            /* Capture the input info data */
            dwCoord = *pCoord;

            nMaxLength = stk->Parameters.DeviceIoControl.OutputBufferLength - sizeof(COORD);
            nMaxLength /= sizeof(USHORT);

            Irp->IoStatus.Information = 0;

            if ( dwCoord.X < 0 || dwCoord.X >= DeviceExtension->Columns ||
                 dwCoord.Y < 0 || dwCoord.Y >= DeviceExtension->Rows    ||
                 nMaxLength == 0 )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            pAttr = (PUSHORT)(pCoord + 1);

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (dwCoord.X + dwCoord.Y * DeviceExtension->Columns) * 2 + 1;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - dwCoord.Y)
                                    * DeviceExtension->Columns - dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++, pAttr++)
                {
                    vidmem[offset + (dwCount * 2)] = *((PCHAR)pAttr);
                }
                Irp->IoStatus.Information = dwCount * sizeof(USHORT);
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE:
        {
            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(USHORT))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            DeviceExtension->CharAttribute = *(PUSHORT)Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER:
        {
            POUTPUT_CHARACTER Buf;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            /* Validate input and output buffers */
            if (stk->Parameters.DeviceIoControl.InputBufferLength  < sizeof(OUTPUT_CHARACTER) ||
                stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(OUTPUT_CHARACTER))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
            nMaxLength = Buf->nLength;

            Buf->dwTransfered = 0;
            Irp->IoStatus.Information = sizeof(OUTPUT_CHARACTER);

            if ( Buf->dwCoord.X < 0 || Buf->dwCoord.X >= DeviceExtension->Columns ||
                 Buf->dwCoord.Y < 0 || Buf->dwCoord.Y >= DeviceExtension->Rows    ||
                 nMaxLength == 0 )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.X + Buf->dwCoord.Y * DeviceExtension->Columns) * 2;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - Buf->dwCoord.Y)
                                    * DeviceExtension->Columns - Buf->dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++)
                {
                    vidmem[offset + (dwCount * 2)] = (char)Buf->cCharacter;
                }
                Buf->dwTransfered = dwCount;
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_READ_OUTPUT_CHARACTER:
        {
            POUTPUT_CHARACTER Buf;
            PCHAR pChar;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(OUTPUT_CHARACTER))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
            Irp->IoStatus.Information = 0;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength == 0)
            {
                Status = STATUS_SUCCESS;
                break;
            }
            ASSERT(Irp->MdlAddress);
            pChar = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (pChar == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if ( Buf->dwCoord.X < 0 || Buf->dwCoord.X >= DeviceExtension->Columns ||
                 Buf->dwCoord.Y < 0 || Buf->dwCoord.Y >= DeviceExtension->Rows )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            nMaxLength = stk->Parameters.DeviceIoControl.OutputBufferLength;

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.X + Buf->dwCoord.Y * DeviceExtension->Columns) * 2;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - Buf->dwCoord.Y)
                                    * DeviceExtension->Columns - Buf->dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++, pChar++)
                {
                    *pChar = vidmem[offset + (dwCount * 2)];
                }
                Irp->IoStatus.Information = dwCount * sizeof(CHAR);
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER:
        {
            COORD dwCoord;
            PCOORD pCoord;
            PCHAR pChar;
            PUCHAR vidmem;
            ULONG offset;
            ULONG dwCount;
            ULONG nMaxLength;

            //
            // NOTE: For whatever reason no OUTPUT_CHARACTER structure
            // is used for this IOCTL.
            //

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(COORD))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->MdlAddress);
            pCoord = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (pCoord == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            /* Capture the input info data */
            dwCoord = *pCoord;

            nMaxLength = stk->Parameters.DeviceIoControl.OutputBufferLength - sizeof(COORD);
            Irp->IoStatus.Information = 0;

            if ( dwCoord.X < 0 || dwCoord.X >= DeviceExtension->Columns ||
                 dwCoord.Y < 0 || dwCoord.Y >= DeviceExtension->Rows    ||
                 nMaxLength == 0 )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            pChar = (PCHAR)(pCoord + 1);

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                vidmem = DeviceExtension->VideoMemory;
                offset = (dwCoord.X + dwCoord.Y * DeviceExtension->Columns) * 2;

                nMaxLength = min(nMaxLength,
                                 (DeviceExtension->Rows - dwCoord.Y)
                                    * DeviceExtension->Columns - dwCoord.X);

                for (dwCount = 0; dwCount < nMaxLength; dwCount++, pChar++)
                {
                    vidmem[offset + (dwCount * 2)] = *pChar;
                }
                Irp->IoStatus.Information = dwCount * sizeof(CHAR);
            }

            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_DRAW:
        {
            CONSOLE_DRAW ConsoleDraw;
            PCONSOLE_DRAW pConsoleDraw;
            PUCHAR Src, Dest;
            UINT32 SrcDelta, DestDelta, i;

            /* Validate output buffer */
            if (stk->Parameters.DeviceIoControl.OutputBufferLength < sizeof(CONSOLE_DRAW))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->MdlAddress);
            pConsoleDraw = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (pConsoleDraw == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            /* Capture the input info data */
            ConsoleDraw = *pConsoleDraw;

            /* Check whether we have the size for the header plus the data area */
            if ((stk->Parameters.DeviceIoControl.OutputBufferLength - sizeof(CONSOLE_DRAW)) / 2
                    < ((ULONG)ConsoleDraw.SizeX * (ULONG)ConsoleDraw.SizeY))
            {
                Status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            // TODO: For the moment if the ConsoleDraw rectangle has borders
            // out of the screen-buffer we just bail out. Would it be better
            // to actually clip the rectangle within its borders instead?
            if ( ConsoleDraw.X < 0 || ConsoleDraw.X >= DeviceExtension->Columns ||
                 ConsoleDraw.Y < 0 || ConsoleDraw.Y >= DeviceExtension->Rows )
            {
                Status = STATUS_SUCCESS;
                break;
            }
            if ( ConsoleDraw.SizeX >= DeviceExtension->Columns - ConsoleDraw.X ||
                 ConsoleDraw.SizeY >= DeviceExtension->Rows    - ConsoleDraw.Y )
            {
                Status = STATUS_SUCCESS;
                break;
            }

            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
            {
                Src = (PUCHAR)(pConsoleDraw + 1);
                SrcDelta = ConsoleDraw.SizeX * 2;
                Dest = DeviceExtension->VideoMemory +
                        (ConsoleDraw.X + ConsoleDraw.Y * DeviceExtension->Columns) * 2;
                DestDelta = DeviceExtension->Columns * 2;
                /* 2 == sizeof(CHAR) + sizeof(BYTE) */

                /* Copy each line */
                for (i = 0; i < ConsoleDraw.SizeY; i++)
                {
                    RtlCopyMemory(Dest, Src, SrcDelta);
                    Src += SrcDelta;
                    Dest += DestDelta;
                }
            }

            /* Set the cursor position, clipping it to the screen */
            DeviceExtension->CursorX = min(max(ConsoleDraw.CursorX, 0), DeviceExtension->Columns - 1);
            DeviceExtension->CursorY = min(max(ConsoleDraw.CursorY, 0), DeviceExtension->Rows    - 1);
            if (DeviceExtension->Enabled)
                ScrSetCursor(DeviceExtension);

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_LOADFONT:
        {
            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            if (DeviceExtension->FontBitfield)
            {
                ExFreePoolWithTag(DeviceExtension->FontBitfield, TAG_BLUE);
                DeviceExtension->FontBitfield = NULL;
            }
            DeviceExtension->CodePage = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

            /* Upload a font for the codepage if needed */
            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
                ScrLoadFontTable(DeviceExtension->CodePage);

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        case IOCTL_CONSOLE_SETFONT:
        {
            /* Validate input buffer */
            if (stk->Parameters.DeviceIoControl.InputBufferLength < 256 * 8)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            ASSERT(Irp->AssociatedIrp.SystemBuffer);

            DeviceExtension->CodePage = 0;
            if (DeviceExtension->FontBitfield)
                ExFreePoolWithTag(DeviceExtension->FontBitfield, TAG_BLUE);
            DeviceExtension->FontBitfield = ExAllocatePoolWithTag(NonPagedPool, 256 * 8, TAG_BLUE);
            if (!DeviceExtension->FontBitfield)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }
            RtlCopyMemory(DeviceExtension->FontBitfield, Irp->AssociatedIrp.SystemBuffer, 256 * 8);

            /* Upload the font if needed */
            if (DeviceExtension->Enabled && DeviceExtension->VideoMemory)
                ScrSetFont(DeviceExtension->FontBitfield);

            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }

        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_VIDEO_INCREMENT);

    return Status;
}

static DRIVER_DISPATCH ScrDispatch;
static NTSTATUS
NTAPI
ScrDispatch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    DPRINT1("ScrDispatch(0x%p): stk->MajorFunction = %lu UNIMPLEMENTED\n",
            DeviceObject, stk->MajorFunction);

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * Module entry point
 */
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\BlueScreen");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\??\\BlueScreen");

    DPRINT("Screen Driver 0.0.6\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = ScrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ]   = ScrDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = ScrWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScrIoControl;

    Status = IoCreateDevice(DriverObject,
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

    Status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (NT_SUCCESS(Status))
    {
        /* By default disable the screen (but don't touch INBV: ResetDisplayParametersDeviceExtension is still NULL) */
        ScrResetScreen(DeviceObject->DeviceExtension, TRUE, FALSE);
        /* Now set ResetDisplayParametersDeviceExtension to enable synchronizing with INBV */
        ResetDisplayParametersDeviceExtension = DeviceObject->DeviceExtension;
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    else
    {
        IoDeleteDevice(DeviceObject);
    }
    return Status;
}

/* EOF */
