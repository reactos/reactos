/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            consrv/condrv/conoutput.c
 * PURPOSE:         General Console Output Functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
TEXTMODE_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN PCONSOLE Console,
                           IN HANDLE ProcessHandle,
                           IN PTEXTMODE_BUFFER_INFO TextModeInfo);
NTSTATUS
GRAPHICS_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN PCONSOLE Console,
                           IN HANDLE ProcessHandle,
                           IN PGRAPHICS_BUFFER_INFO GraphicsInfo);

VOID
TEXTMODE_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);
VOID
GRAPHICS_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);


NTSTATUS
CONSOLE_SCREEN_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                                 IN PCONSOLE Console,
                                 IN PCONSOLE_SCREEN_BUFFER_VTBL Vtbl,
                                 IN SIZE_T Size)
{
    if (Buffer == NULL || Console == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = ConsoleAllocHeap(HEAP_ZERO_MEMORY, max(sizeof(CONSOLE_SCREEN_BUFFER), Size));
    if (*Buffer == NULL) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize the header with the default type */
    ConSrvInitObject(&(*Buffer)->Header, SCREEN_BUFFER, Console);
    (*Buffer)->Vtbl = Vtbl;
    return STATUS_SUCCESS;
}

VOID
CONSOLE_SCREEN_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer)
{
    if (Buffer->Header.Type == TEXTMODE_BUFFER)
    {
        TEXTMODE_BUFFER_Destroy(Buffer);
    }
    else if (Buffer->Header.Type == GRAPHICS_BUFFER)
    {
        GRAPHICS_BUFFER_Destroy(Buffer);
    }
    else if (Buffer->Header.Type == SCREEN_BUFFER)
    {
        /* Free the palette handle */
        if (Buffer->PaletteHandle != NULL) DeleteObject(Buffer->PaletteHandle);

        /* Free the screen buffer memory */
        ConsoleFreeHeap(Buffer);
    }
    // else
    //     do_nothing;
}

// ConDrvCreateConsoleScreenBuffer
NTSTATUS
ConDrvCreateScreenBuffer(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                         IN PCONSOLE Console,
                         IN HANDLE ProcessHandle OPTIONAL,
                         IN ULONG BufferType,
                         IN PVOID ScreenBufferInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( Console == NULL || Buffer == NULL ||
        (BufferType != CONSOLE_TEXTMODE_BUFFER && BufferType != CONSOLE_GRAPHICS_BUFFER) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Use the current process if ProcessHandle is NULL */
    if (ProcessHandle == NULL)
        ProcessHandle = NtCurrentProcess();

    if (BufferType == CONSOLE_TEXTMODE_BUFFER)
    {
        Status = TEXTMODE_BUFFER_Initialize(Buffer, Console, ProcessHandle,
                                            (PTEXTMODE_BUFFER_INFO)ScreenBufferInfo);
    }
    else if (BufferType == CONSOLE_GRAPHICS_BUFFER)
    {
        Status = GRAPHICS_BUFFER_Initialize(Buffer, Console, ProcessHandle,
                                            (PGRAPHICS_BUFFER_INFO)ScreenBufferInfo);
    }
    else
    {
        /* Never ever go there!! */
        ASSERT(FALSE);
    }

    /* Insert the newly created screen buffer into the list, if succeeded */
    if (NT_SUCCESS(Status)) InsertHeadList(&Console->BufferList, &(*Buffer)->ListEntry);

    return Status;
}

static VOID
ConioSetActiveScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer);

VOID NTAPI
ConDrvDeleteScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;
    PCONSOLE_SCREEN_BUFFER NewBuffer;

    /*
     * We should notify temporarily the frontend because we are susceptible
     * to delete the screen buffer it is using (which may be different from
     * the active screen buffer in some cases), and because, if it actually
     * uses the active screen buffer, we are going to nullify its pointer to
     * change it.
     */
    TermReleaseScreenBuffer(Console, Buffer);

    RemoveEntryList(&Buffer->ListEntry);
    if (Buffer == Console->ActiveBuffer)
    {
        /* Delete active buffer; switch to most recently created */
        if (!IsListEmpty(&Console->BufferList))
        {
            NewBuffer = CONTAINING_RECORD(Console->BufferList.Flink,
                                          CONSOLE_SCREEN_BUFFER,
                                          ListEntry);

            /* Tie console to new buffer and signal the change to the frontend */
            ConioSetActiveScreenBuffer(NewBuffer);
        }
        else
        {
            Console->ActiveBuffer = NULL;
            // InterlockedExchangePointer(&Console->ActiveBuffer, NULL);
        }
    }

    CONSOLE_SCREEN_BUFFER_Destroy(Buffer);
}

static VOID
ConioSetActiveScreenBuffer(PCONSOLE_SCREEN_BUFFER Buffer)
{
    PCONSOLE Console = Buffer->Header.Console;
    Console->ActiveBuffer = Buffer;
    // InterlockedExchangePointer(&Console->ActiveBuffer, Buffer);
    TermSetActiveScreenBuffer(Console);
}

NTSTATUS NTAPI
ConDrvSetConsoleActiveScreenBuffer(IN PCONSOLE Console,
                                   IN PCONSOLE_SCREEN_BUFFER Buffer)
{
    if (Console == NULL || Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    if (Buffer == Console->ActiveBuffer) return STATUS_SUCCESS;

    /* If old buffer has no handles, it's now unreferenced */
    if (Console->ActiveBuffer->Header.ReferenceCount == 0)
    {
        ConDrvDeleteScreenBuffer(Console->ActiveBuffer);
    }

    /* Tie console to new buffer and signal the change to the frontend */
    ConioSetActiveScreenBuffer(Buffer);

    return STATUS_SUCCESS;
}

PCONSOLE_SCREEN_BUFFER
ConDrvGetActiveScreenBuffer(IN PCONSOLE Console)
{
    return (Console ? Console->ActiveBuffer : NULL);
}

/* PUBLIC DRIVER APIS *********************************************************/

NTSTATUS NTAPI
ConDrvInvalidateBitMapRect(IN PCONSOLE Console,
                           IN PCONSOLE_SCREEN_BUFFER Buffer,
                           IN PSMALL_RECT Region)
{
    if (Console == NULL || Buffer == NULL || Region == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    /* If the output buffer is the current one, redraw the correct portion of the screen */
    if (Buffer == Console->ActiveBuffer) TermDrawRegion(Console, Region);

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsolePalette(IN PCONSOLE Console,
                        // IN PGRAPHICS_SCREEN_BUFFER Buffer,
                        IN PCONSOLE_SCREEN_BUFFER Buffer,
                        IN HPALETTE PaletteHandle,
                        IN UINT PaletteUsage)
{
    BOOL Success;

    /*
     * Parameters validation
     */
    if (Console == NULL || Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    if ( PaletteUsage != SYSPAL_STATIC   &&
         PaletteUsage != SYSPAL_NOSTATIC &&
         PaletteUsage != SYSPAL_NOSTATIC256 )
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    /* Change the palette */
    Success = TermSetPalette(Console, PaletteHandle, PaletteUsage);
    if (Success)
    {
        /* Free the old palette handle if there was already one set */
        if ( Buffer->PaletteHandle != NULL &&
             Buffer->PaletteHandle != PaletteHandle )
        {
            DeleteObject(Buffer->PaletteHandle);
        }

        /* Save the new palette in the screen buffer */
        Buffer->PaletteHandle = PaletteHandle;
        Buffer->PaletteUsage  = PaletteUsage;
    }

    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS NTAPI
ConDrvGetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           OUT PCONSOLE_CURSOR_INFO CursorInfo)
{
    if (Console == NULL || Buffer == NULL || CursorInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    *CursorInfo = Buffer->CursorInfo;
    // CursorInfo->bVisible = Buffer->CursorInfo.bVisible;
    // CursorInfo->dwSize   = Buffer->CursorInfo.dwSize;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorInfo(IN PCONSOLE Console,
                           IN PTEXTMODE_SCREEN_BUFFER Buffer,
                           IN PCONSOLE_CURSOR_INFO CursorInfo)
{
    ULONG Size;
    BOOLEAN Visible, Success = TRUE;

    if (Console == NULL || Buffer == NULL || CursorInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    Size    = min(max(CursorInfo->dwSize, 1), 100);
    Visible = CursorInfo->bVisible;

    if ( (Size != Buffer->CursorInfo.dwSize)         ||
         (Visible && !Buffer->CursorInfo.bVisible)   ||
         (!Visible && Buffer->CursorInfo.bVisible) )
    {
        Buffer->CursorInfo.dwSize   = Size;
        Buffer->CursorInfo.bVisible = Visible;

        Success = TermSetCursorInfo(Console, (PCONSOLE_SCREEN_BUFFER)Buffer);
    }

    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

NTSTATUS NTAPI
ConDrvSetConsoleCursorPosition(IN PCONSOLE Console,
                               IN PTEXTMODE_SCREEN_BUFFER Buffer,
                               IN PCOORD Position)
{
    SHORT OldCursorX, OldCursorY;

    if (Console == NULL || Buffer == NULL || Position == NULL)
        return STATUS_INVALID_PARAMETER;

    /* Validity check */
    ASSERT(Console == Buffer->Header.Console);

    if ( Position->X < 0 || Position->X >= Buffer->ScreenBufferSize.X ||
         Position->Y < 0 || Position->Y >= Buffer->ScreenBufferSize.Y )
    {
        return STATUS_INVALID_PARAMETER;
    }

    OldCursorX = Buffer->CursorPosition.X;
    OldCursorY = Buffer->CursorPosition.Y;
    Buffer->CursorPosition = *Position;
    // Buffer->CursorPosition.X = Position->X;
    // Buffer->CursorPosition.Y = Position->Y;
    if ( ((PCONSOLE_SCREEN_BUFFER)Buffer == Console->ActiveBuffer) &&
         (!TermSetScreenInfo(Console, (PCONSOLE_SCREEN_BUFFER)Buffer, OldCursorX, OldCursorY)) )
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/* EOF */
