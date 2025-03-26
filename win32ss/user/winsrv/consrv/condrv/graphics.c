/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Driver DLL
 * FILE:            win32ss/user/winsrv/consrv/condrv/graphics.c
 * PURPOSE:         Console Output Functions for graphics-mode screen-buffers
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * NOTE:            See http://blog.airesoft.co.uk/2012/10/things-ms-can-do-that-they-dont-tell-you-about-console-graphics/
 *                  for more information.
 */

/* INCLUDES *******************************************************************/

#include <consrv.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
CONSOLE_SCREEN_BUFFER_Initialize(
    OUT PCONSOLE_SCREEN_BUFFER* Buffer,
    IN PCONSOLE Console,
    IN CONSOLE_IO_OBJECT_TYPE Type,
    IN SIZE_T Size);

VOID
CONSOLE_SCREEN_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer);


NTSTATUS
GRAPHICS_BUFFER_Initialize(OUT PCONSOLE_SCREEN_BUFFER* Buffer,
                           IN PCONSOLE Console,
                           IN HANDLE ProcessHandle,
                           IN PGRAPHICS_BUFFER_INFO GraphicsInfo)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PGRAPHICS_SCREEN_BUFFER NewBuffer = NULL;

    LARGE_INTEGER SectionSize;
    SIZE_T ViewSize = 0;

    if (Buffer == NULL || Console == NULL || GraphicsInfo == NULL)
        return STATUS_INVALID_PARAMETER;

    *Buffer = NULL;

    Status = CONSOLE_SCREEN_BUFFER_Initialize((PCONSOLE_SCREEN_BUFFER*)&NewBuffer,
                                              Console,
                                              GRAPHICS_BUFFER,
                                              sizeof(GRAPHICS_SCREEN_BUFFER));
    if (!NT_SUCCESS(Status)) return Status;

    /*
     * Remember the handle to the process so that we can close or unmap
     * correctly the allocated resources when the client releases the
     * screen buffer.
     */
    NewBuffer->ClientProcess = ProcessHandle;

    /* Get information from the graphics buffer information structure */
    NewBuffer->BitMapInfoLength = GraphicsInfo->Info.dwBitMapInfoLength;

    NewBuffer->BitMapInfo = ConsoleAllocHeap(HEAP_ZERO_MEMORY, NewBuffer->BitMapInfoLength);
    if (NewBuffer->BitMapInfo == NULL)
    {
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Adjust the bitmap height if needed (bottom-top vs. top-bottom). Use always bottom-up. */
    if (GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biHeight > 0)
        GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biHeight = -GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biHeight;

    /* We do not use anything else than uncompressed bitmaps */
    if (GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biCompression != BI_RGB)
    {
        DPRINT1("biCompression == %d != BI_RGB, fix that!\n",
                GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biCompression);
        GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biCompression = BI_RGB;
    }

    RtlCopyMemory(NewBuffer->BitMapInfo,
                  GraphicsInfo->Info.lpBitMapInfo,
                  GraphicsInfo->Info.dwBitMapInfoLength);

    NewBuffer->BitMapUsage = GraphicsInfo->Info.dwUsage;

    /* Set the screen buffer size. Fight against overflows. */
    if ( GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biWidth  <= 0xFFFF &&
        -GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biHeight <= 0xFFFF )
    {
        /* Be careful about the sign of biHeight */
        NewBuffer->ScreenBufferSize.X =  (SHORT)GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biWidth ;
        NewBuffer->ScreenBufferSize.Y = (SHORT)-GraphicsInfo->Info.lpBitMapInfo->bmiHeader.biHeight;

        NewBuffer->OldViewSize = NewBuffer->ViewSize =
            NewBuffer->OldScreenBufferSize = NewBuffer->ScreenBufferSize;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    /*
     * Create a mutex to synchronize bitmap memory access
     * between ourselves and the client.
     */
    Status = NtCreateMutant(&NewBuffer->Mutex, MUTANT_ALL_ACCESS, NULL, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateMutant() failed: %lu\n", Status);
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    /*
     * Duplicate the Mutex for the client. We must keep a trace of it
     * so that we can close it when the client releases the screen buffer.
     */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               NewBuffer->Mutex,
                               ProcessHandle,
                               &NewBuffer->ClientMutex,
                               0, 0, DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateObject() failed: %lu\n", Status);
        NtClose(NewBuffer->Mutex);
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    /*
     * Create a memory section for the bitmap area, to share with the client.
     */
    SectionSize.QuadPart = NewBuffer->BitMapInfo->bmiHeader.biSizeImage;
    Status = NtCreateSection(&NewBuffer->hSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to create a shared section, Status = 0x%08lx\n", Status);
        NtDuplicateObject(ProcessHandle, NewBuffer->ClientMutex,
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        NtClose(NewBuffer->Mutex);
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    /*
     * Create a view for our needs.
     */
    ViewSize = 0;
    NewBuffer->BitMap = NULL;
    Status = NtMapViewOfSection(NewBuffer->hSection,
                                NtCurrentProcess(),
                                (PVOID*)&NewBuffer->BitMap,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to map the shared section, Status = 0x%08lx\n", Status);
        NtClose(NewBuffer->hSection);
        NtDuplicateObject(ProcessHandle, NewBuffer->ClientMutex,
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        NtClose(NewBuffer->Mutex);
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    /*
     * Create a view for the client. We must keep a trace of it so that
     * we can unmap it when the client releases the screen buffer.
     */
    ViewSize = 0;
    NewBuffer->ClientBitMap = NULL;
    Status = NtMapViewOfSection(NewBuffer->hSection,
                                ProcessHandle,
                                (PVOID*)&NewBuffer->ClientBitMap,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Error: Impossible to map the shared section, Status = 0x%08lx\n", Status);
        NtUnmapViewOfSection(NtCurrentProcess(), NewBuffer->BitMap);
        NtClose(NewBuffer->hSection);
        NtDuplicateObject(ProcessHandle, NewBuffer->ClientMutex,
                          NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
        NtClose(NewBuffer->Mutex);
        ConsoleFreeHeap(NewBuffer->BitMapInfo);
        CONSOLE_SCREEN_BUFFER_Destroy((PCONSOLE_SCREEN_BUFFER)NewBuffer);
        goto Quit;
    }

    NewBuffer->ViewOrigin.X = NewBuffer->ViewOrigin.Y = 0;

    NewBuffer->CursorBlinkOn  = FALSE;
    NewBuffer->ForceCursorOff = TRUE;
    NewBuffer->CursorInfo.bVisible = FALSE;
    NewBuffer->CursorInfo.dwSize   = 0;
    NewBuffer->CursorPosition.X = NewBuffer->CursorPosition.Y = 0;

    NewBuffer->Mode = 0;

    *Buffer = (PCONSOLE_SCREEN_BUFFER)NewBuffer;
    Status = STATUS_SUCCESS;

Quit:
    return Status;
}

VOID
GRAPHICS_BUFFER_Destroy(IN OUT PCONSOLE_SCREEN_BUFFER Buffer)
{
    PGRAPHICS_SCREEN_BUFFER Buff = (PGRAPHICS_SCREEN_BUFFER)Buffer;

    /*
     * IMPORTANT !! Reinitialize the type so that we don't enter a recursive
     * infinite loop when calling CONSOLE_SCREEN_BUFFER_Destroy.
     */
    Buffer->Header.Type = SCREEN_BUFFER;

    /*
     * Uninitialize the graphics screen buffer
     * in the reverse way we initialized it.
     */
    NtUnmapViewOfSection(Buff->ClientProcess, Buff->ClientBitMap);
    NtUnmapViewOfSection(NtCurrentProcess(), Buff->BitMap);
    NtClose(Buff->hSection);
    NtDuplicateObject(Buff->ClientProcess, Buff->ClientMutex,
                      NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
    NtClose(Buff->Mutex);
    ConsoleFreeHeap(Buff->BitMapInfo);

    CONSOLE_SCREEN_BUFFER_Destroy(Buffer);
}

/* EOF */
