/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/subsysreg.c
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

/* API_NUMBER: ConsolepRegisterVDM */
CON_API(SrvRegisterConsoleVDM,
        CONSOLE_REGISTERVDM, RegisterVDMRequest)
{
    NTSTATUS Status;

    DPRINT1("SrvRegisterConsoleVDM(%d)\n", RegisterVDMRequest->RegisterFlags);

    if (RegisterVDMRequest->RegisterFlags != 0)
    {
        LARGE_INTEGER SectionSize;
        SIZE_T Size, ViewSize = 0;
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
            return Status;
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
            return Status;
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
            return Status;
        }

        // TODO: Duplicate the event handles.

        RegisterVDMRequest->VDMBuffer = Console->ClientVDMBuffer;

        return STATUS_SUCCESS;
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

        return STATUS_SUCCESS;
    }
}

/* API_NUMBER: ConsolepVDMOperation */
CSR_API(SrvVDMConsoleOperation)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * OS/2 Subsystem
 */

/* API_NUMBER: ConsolepRegisterOS2 */
CSR_API(SrvRegisterConsoleOS2)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepSetOS2OemFormat */
CSR_API(SrvSetConsoleOS2OemFormat)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * IME Subsystem
 */

/* API_NUMBER: ConsolepRegisterConsoleIME */
CSR_API(SrvRegisterConsoleIME)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* API_NUMBER: ConsolepUnregisterConsoleIME */
CSR_API(SrvUnregisterConsoleIME)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
