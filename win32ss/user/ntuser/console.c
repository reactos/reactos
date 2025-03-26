/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Console support functions for CONSRV
 * FILE:             win32ss/user/ntuser/console.c
 * PROGRAMMER:       Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

NTSTATUS
APIENTRY
NtUserConsoleControl(
    IN CONSOLECONTROL ConsoleCtrl,
    IN PVOID ConsoleCtrlInfo,
    IN ULONG ConsoleCtrlInfoLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Allow only the Console Server to perform this operation (via CSRSS) */
    if (PsGetCurrentProcess() != gpepCSRSS)
        return STATUS_ACCESS_DENIED;

    UserEnterExclusive();

    switch (ConsoleCtrl)
    {
        case ConsoleCtrlDesktopConsoleThread:
        {
            DESKTOP_CONSOLE_THREAD DesktopConsoleThreadInfo;
            PDESKTOP Desktop = NULL;
            ULONG_PTR OldThreadId;

            if (ConsoleCtrlInfoLength != sizeof(DesktopConsoleThreadInfo))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                ProbeForWrite(ConsoleCtrlInfo, ConsoleCtrlInfoLength, sizeof(USHORT));
                DesktopConsoleThreadInfo = *(PDESKTOP_CONSOLE_THREAD)ConsoleCtrlInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the desktop */
            Status = ObReferenceObjectByHandle(DesktopConsoleThreadInfo.DesktopHandle,
                                               0,
                                               ExDesktopObjectType,
                                               UserMode,
                                               (PVOID*)&Desktop,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Save the old thread ID, it is always returned to the caller */
            OldThreadId = Desktop->dwConsoleThreadId;

            /* Set the new console input thread ID for this desktop if required */
            if (DesktopConsoleThreadInfo.ThreadId != (ULONG_PTR)INVALID_HANDLE_VALUE)
            {
                Desktop->dwConsoleThreadId = DesktopConsoleThreadInfo.ThreadId;
            }

            /* Always return the old thread ID */
            DesktopConsoleThreadInfo.ThreadId = OldThreadId;

            /* Dereference the desktop */
            ObDereferenceObject(Desktop);

            /* Return the information back to the caller */
            _SEH2_TRY
            {
                *(PDESKTOP_CONSOLE_THREAD)ConsoleCtrlInfo = DesktopConsoleThreadInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case GuiConsoleWndClassAtom:
        {
            if (ConsoleCtrlInfoLength != sizeof(ATOM))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                ProbeForRead(ConsoleCtrlInfo, ConsoleCtrlInfoLength, sizeof(USHORT));
                gaGuiConsoleWndClass = *(ATOM*)ConsoleCtrlInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            break;
        }

        case ConsoleMakePalettePublic:
        {
            HPALETTE hPalette;

            if (ConsoleCtrlInfoLength != sizeof(hPalette))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            _SEH2_TRY
            {
                ProbeForRead(ConsoleCtrlInfo, ConsoleCtrlInfoLength, sizeof(USHORT));
                hPalette = *(HPALETTE*)ConsoleCtrlInfo;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Make the palette handle public */
            GreSetObjectOwnerEx(hPalette,
                                GDI_OBJ_HMGR_PUBLIC,
                                GDIOBJFLAG_IGNOREPID);
            break;
        }

        case ConsoleAcquireDisplayOwnership:
        {
            ERR("NtUserConsoleControl - ConsoleAcquireDisplayOwnership is UNIMPLEMENTED\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        default:
            ERR("Calling invalid control %d in NtUserConsoleControl\n", ConsoleCtrl);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    UserLeave();

    return Status;
}

/* EOF */
