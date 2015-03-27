/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            consrv/subsysreg.c
 * PURPOSE:         Registration APIs for VDM, OS2 and IME subsystems
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"

#define NDEBUG
#include <debug.h>

/* PUBLIC SERVER APIS *********************************************************/

/*
 * VDM Subsystem
 */

CSR_API(SrvRegisterConsoleVDM)
{
    NTSTATUS Status;
    PCONSOLE_REGISTERVDM RegisterVDMRequest = &((PCONSOLE_API_MESSAGE)ApiMessage)->Data.RegisterVDMRequest;
    PCONSRV_CONSOLE Console;

    DPRINT1("SrvRegisterConsoleVDM(%d)\n", RegisterVDMRequest->RegisterFlags);

    Status = ConSrvGetConsole(ConsoleGetPerProcessData(CsrGetClientThread()->Process),
                              &Console, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't get console, status %lx\n", Status);
        return Status;
    }

    if (RegisterVDMRequest->RegisterFlags != 0)
    {
        LARGE_INTEGER SectionSize;
        ULONG Size, ViewSize = 0;
        HANDLE ProcessHandle;

        /*
         * Remember the handle to the process so that we can close or unmap
         * correctly the allocated resources when the client releases the
         * screen buffer.
         */
        ProcessHandle = CsrGetClientThread()->Process->ProcessHandle;
        Console->VDMClientProcess = ProcessHandle;

        Console->VDMBufferSize = RegisterVDMRequest->VDMBufferSize;

        Size = Console->VDMBufferSize.X * Console->VDMBufferSize.Y
                                        * sizeof(CHAR_CELL);

        /*
         * Create a memory section for the VDM buffer, to share with the client.
         */
        SectionSize.QuadPart = Size;
        Status = NtCreateSection(&Console->VDMBufferSection,
                                 SECTION_ALL_ACCESS,
                                 NULL,
                                 &SectionSize,
                                 PAGE_READWRITE,
                                 SEC_COMMIT,
                                 NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Error: Impossible to create a shared section, Status = 0x%08lx\n", Status);
            goto Quit;
        }

        /*
         * Create a view for our needs.
         */
        ViewSize = 0;
        Console->VDMBuffer = NULL;
        Status = NtMapViewOfSection(Console->VDMBufferSection,
                                    NtCurrentProcess(),
                                    (PVOID*)&Console->VDMBuffer,
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
            NtClose(Console->VDMBufferSection);
            goto Quit;
        }

        /*
         * Create a view for the client. We must keep a trace of it so that
         * we can unmap it when the client releases the VDM buffer.
         */
        ViewSize = 0;
        Console->ClientVDMBuffer = NULL;
        Status = NtMapViewOfSection(Console->VDMBufferSection,
                                    ProcessHandle,
                                    (PVOID*)&Console->ClientVDMBuffer,
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
            NtUnmapViewOfSection(NtCurrentProcess(), Console->VDMBuffer);
            NtClose(Console->VDMBufferSection);
            goto Quit;
        }

        // TODO: Duplicate the event handles.

        RegisterVDMRequest->VDMBuffer = Console->ClientVDMBuffer;

        Status = STATUS_SUCCESS;
    }
    else
    {
        /* RegisterFlags == 0 means we are unregistering the VDM */

        // TODO: Close the duplicated handles.

        if (Console->VDMBuffer)
        {
            /*
             * Uninitialize the graphics screen buffer
             * in the reverse way we initialized it.
             */
            NtUnmapViewOfSection(Console->VDMClientProcess, Console->ClientVDMBuffer);
            NtUnmapViewOfSection(NtCurrentProcess(), Console->VDMBuffer);
            NtClose(Console->VDMBufferSection);
        }
        Console->VDMBuffer = Console->ClientVDMBuffer = NULL;

        Console->VDMBufferSize.X = Console->VDMBufferSize.Y = 0;
    }

Quit:
    ConSrvReleaseConsole(Console, TRUE);
    return Status;
}

CSR_API(SrvVDMConsoleOperation)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * OS/2 Subsystem
 */

CSR_API(SrvRegisterConsoleOS2)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvSetConsoleOS2OemFormat)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * IME Subsystem
 */

CSR_API(SrvRegisterConsoleIME)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(SrvUnregisterConsoleIME)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
