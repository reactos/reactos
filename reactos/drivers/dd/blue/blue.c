/* $Id: blue.c,v 1.13 1999/11/09 18:07:03 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <ddk/ntddblue.h>
#include <string.h>
#include <defines.h>

#define NDEBUG
#include <internal/debug.h>


/* DEFINITIONS ***************************************************************/

#define IDMAP_BASE         0xd0000000
#define VIDMEM_BASE        0xb8000

#define NR_ROWS            50
#define NR_COLUMNS         80
#define NR_SCANLINES       8

#define CRTC_COMMAND       ((PUCHAR)0x3d4)
#define CRTC_DATA          ((PUCHAR)0x3d5)

#define CRTC_COLUMNS       0x01
#define CRTC_ROWS          0x12
#define CRTC_SCANLINES     0x09
#define CRTC_CURSORSTART   0x0a
#define CRTC_CURSOREND     0x0b
#define CRTC_CURSORPOSLO   0x0f
#define CRTC_CURSORPOSHI   0x0e

#define ATTRC_WRITEREG     ((PUCHAR)0x3c0)
#define ATTRC_READREG      ((PUCHAR)0x3c1)
#define ATTRC_INPST1       ((PUCHAR)0x3da)

#define TAB_WIDTH          8


/* NOTES ******************************************************************/
/*
 *  [[character][attribute]][[character][attribute]]....
 */


/* TYPEDEFS ***************************************************************/

typedef struct _DEVICE_EXTENSION
{
    PBYTE VideoMemory;    /* Pointer to video memory */
    DWORD CursorSize;
    BOOL  CursorVisible;
    WORD  CharAttribute;
    DWORD Mode;
    BYTE  ScanLines;      /* Height of a text line */
    BYTE  Rows;           /* Number of rows        */
    BYTE  Columns;        /* Number of columns     */
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* FUNCTIONS **************************************************************/

NTSTATUS
ScrCreate (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    unsigned int offset;
    BYTE data, value;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* initialize device extension */

    /* get pointer to video memory */
    /* FIXME : use MmMapIoSpace() */
    DeviceExtension->VideoMemory = (PBYTE)(IDMAP_BASE + VIDMEM_BASE);

    __asm__("cli\n\t");

    /* get current output position */
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    offset = READ_PORT_UCHAR (CRTC_DATA);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset += (READ_PORT_UCHAR (CRTC_DATA) << 8);

    /* switch blinking characters off */
    READ_PORT_UCHAR (ATTRC_INPST1 /*0x3da*/);
    value = READ_PORT_UCHAR (ATTRC_WRITEREG/*0x3c0*/);
    WRITE_PORT_UCHAR (ATTRC_WRITEREG /*0x3c0*/, 0x10);
    data  = READ_PORT_UCHAR (ATTRC_READREG /*0x3c1*/);
    data  = data & ~0x08;
    WRITE_PORT_UCHAR (ATTRC_WRITEREG /*0x3c0*/, data);
    WRITE_PORT_UCHAR (ATTRC_WRITEREG /*0x3c0*/, value);
    READ_PORT_UCHAR (ATTRC_INPST1 /*0x3da*/);
    __asm__("sti\n\t");

    DeviceExtension->ScanLines = NR_SCANLINES; /* FIXME: read it from CRTC */
    DeviceExtension->Rows  = NR_ROWS; /* FIXME: read it from CRTC */
    DeviceExtension->Columns = NR_COLUMNS; /* FIXME: read it from CRTC */

    DeviceExtension->CursorSize    = 5; /* FIXME: value correct?? */
    DeviceExtension->CursorVisible = TRUE;

    /* more initialization */
    DeviceExtension->CharAttribute = 0x17;  /* light grey on blue */
    DeviceExtension->Mode = ENABLE_PROCESSED_OUTPUT |
                            ENABLE_WRAP_AT_EOL_OUTPUT;

    /* show blinking cursor */
    /* FIXME: calculate cursor size */
    __asm__("cli\n\t");
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORSTART);
    WRITE_PORT_UCHAR (CRTC_DATA, 0x47);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSOREND);
    WRITE_PORT_UCHAR (CRTC_DATA, 0x07);
    __asm__("sti\n\t");

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


NTSTATUS
ScrWrite (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stk = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    char *pch = Irp->UserBuffer;
    char *vidmem;
    int i, j, offset;
    int cursorx, cursory;
    int rows, columns;

    DeviceExtension = DeviceObject->DeviceExtension;
    vidmem  = DeviceExtension->VideoMemory;
    rows = DeviceExtension->Rows;
    columns = DeviceExtension->Columns;

    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset = READ_PORT_UCHAR (CRTC_DATA)<<8;
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    offset += READ_PORT_UCHAR (CRTC_DATA);

    cursory = offset / columns;
    cursorx = offset % columns;

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
                break;

            case '\n':
                cursory++;
                cursorx = 0;
                break;

            case '\r':
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
        }
    }


    /* Set the cursor position */
    offset = (cursory * columns) + cursorx;

    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
    WRITE_PORT_UCHAR (CRTC_DATA, offset);
    WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
    offset >>= 8;
    WRITE_PORT_UCHAR (CRTC_DATA, offset);

    Status = STATUS_SUCCESS;
   
    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


NTSTATUS
ScrIoControl (PDEVICE_OBJECT DeviceObject, PIRP Irp)
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

                __asm__("cli\n\t");
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
                offset = READ_PORT_UCHAR (CRTC_DATA);
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
                offset += (READ_PORT_UCHAR (CRTC_DATA) << 8);
                __asm__("sti\n\t");

                pcsbi->dwSize.X = rows;
                pcsbi->dwSize.Y = columns;

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

                __asm__("cli\n\t");
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSLO);
                WRITE_PORT_UCHAR (CRTC_DATA, offset);
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORPOSHI);
                WRITE_PORT_UCHAR (CRTC_DATA, offset>>8);
                __asm__("sti\n\t");

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
                BYTE data;
                DWORD size, height;

                DeviceExtension->CursorSize = pcci->dwSize;
                DeviceExtension->CursorVisible = pcci->bVisible;
                height = DeviceExtension->ScanLines;
                data = (pcci->bVisible) ? 0x40 : 0x20;

                size = (pcci->dwSize * height) / 100;
                if (size < 1)
                    size = 1;

                data |= (BYTE)(height - size);

                __asm__("cli\n\t");
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSORSTART);
                WRITE_PORT_UCHAR (CRTC_DATA, data);
                WRITE_PORT_UCHAR (CRTC_COMMAND, CRTC_CURSOREND);
                WRITE_PORT_UCHAR (CRTC_DATA, height - 1);
                __asm__("sti\n\t");

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
                char *vidmem;
                int offset;
                DWORD dwCount;

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
                LPWORD pAttr = (LPWORD)MmGetSystemAddressForMdl(Irp->MdlAddress);
                char *vidmem;
                int offset;
                DWORD dwCount;

                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                         (Buf->dwCoord.X * 2) + 1;

                for (dwCount = 0; dwCount < stk->Parameters.Write.Length; dwCount++, pAttr++)
                {
                    (char) *pAttr = vidmem[offset + (dwCount * 2)];
                }

                Buf->dwTransfered = dwCount;

                Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);
                Status = STATUS_SUCCESS;
            }
            break;

        case IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE:
            {
                POUTPUT_ATTRIBUTE Buf = (POUTPUT_ATTRIBUTE)Irp->AssociatedIrp.SystemBuffer;
                LPWORD pAttr = (LPWORD)MmGetSystemAddressForMdl(Irp->MdlAddress);
                char *vidmem;
                int offset;
                DWORD dwCount;

                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                         (Buf->dwCoord.X * 2) + 1;

                for (dwCount = 0; dwCount < stk->Parameters.Write.Length; dwCount++, pAttr++)
                {
                    vidmem[offset + (dwCount * 2)] = (char) *pAttr;
                }

                Buf->dwTransfered = dwCount;

                Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);
                Status = STATUS_SUCCESS;
            }
            break;

        case IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE:
            DeviceExtension->CharAttribute = (WORD)*(LPWORD)Irp->AssociatedIrp.SystemBuffer;
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;


        case IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER:
            {
                POUTPUT_CHARACTER Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
                char *vidmem;
                int offset;
                DWORD dwCount;

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
                char *vidmem;
                int offset;
                DWORD dwCount;

                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                         (Buf->dwCoord.X * 2);

                for (dwCount = 0; dwCount < stk->Parameters.Write.Length; dwCount++, pChar++)
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
                POUTPUT_CHARACTER Buf = (POUTPUT_CHARACTER)Irp->AssociatedIrp.SystemBuffer;
                LPSTR pChar = (LPSTR)MmGetSystemAddressForMdl(Irp->MdlAddress);
                char *vidmem;
                int offset;
                DWORD dwCount;

                vidmem = DeviceExtension->VideoMemory;
                offset = (Buf->dwCoord.Y * DeviceExtension->Columns * 2) +
                         (Buf->dwCoord.X * 2) + 1;

                for (dwCount = 0; dwCount < stk->Parameters.Write.Length; dwCount++, pChar++)
                {
                    vidmem[offset + (dwCount * 2)] = (char) *pChar;
                }

                Buf->dwTransfered = dwCount;

                Irp->IoStatus.Information = sizeof(OUTPUT_ATTRIBUTE);
                Status = STATUS_SUCCESS;
            }
            break;


        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return (Status);
}


NTSTATUS
ScrDispatch (PDEVICE_OBJECT DeviceObject, PIRP Irp)
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
    ANSI_STRING adevice_name;
    UNICODE_STRING device_name;
    ANSI_STRING asymlink_name;
    UNICODE_STRING symlink_name;
   
    DbgPrint ("Screen Driver 0.0.6\n");

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ScrCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = ScrDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ]   = ScrDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = ScrWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL ] = ScrIoControl;

    RtlInitAnsiString (&adevice_name, "\\Device\\BlueScreen");
    RtlAnsiStringToUnicodeString (&device_name, &adevice_name, TRUE);
    IoCreateDevice (DriverObject,
                    sizeof(DEVICE_EXTENSION),
                    &device_name,
                    FILE_DEVICE_SCREEN,
                    0,
                    TRUE,
                    &DeviceObject);

    RtlInitAnsiString (&asymlink_name, "\\??\\BlueScreen");
    RtlAnsiStringToUnicodeString (&symlink_name, &asymlink_name, TRUE);
    IoCreateSymbolicLink (&symlink_name, &device_name);

    RtlFreeUnicodeString (&device_name);
    RtlFreeUnicodeString (&symlink_name);

    return (STATUS_SUCCESS);
}

/* EOF */
