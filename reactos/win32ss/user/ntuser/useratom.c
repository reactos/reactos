/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User Atom helper routines
 * FILE:             subsys/win32k/ntuser/useratom.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMisc);

RTL_ATOM FASTCALL
IntAddAtom(LPWSTR AtomName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTHREADINFO pti;
    RTL_ATOM Atom;

    pti = PsGetCurrentThreadWin32Thread();
    if (pti->rpdesk == NULL)
    {
        SetLastNtError(Status);
        return (RTL_ATOM)0;
    }

    Status = RtlAddAtomToAtomTable(gAtomTable, AtomName, &Atom);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return (RTL_ATOM)0;
    }
    return Atom;
}

ULONG FASTCALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG cjBufSize)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTHREADINFO pti;
    ULONG Size = cjBufSize;

    pti = PsGetCurrentThreadWin32Thread();
    if (pti->rpdesk == NULL)
    {
        SetLastNtError(Status);
        return 0;
    }

    Status = RtlQueryAtomInAtomTable(gAtomTable, nAtom, NULL, NULL, lpBuffer, &Size);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return 0;
    }

    return Size;
}

RTL_ATOM FASTCALL
IntAddGlobalAtom(LPWSTR lpBuffer, BOOL PinAtom)
{
    RTL_ATOM Atom;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = RtlAddAtomToAtomTable(gAtomTable, lpBuffer, &Atom);

    if (!NT_SUCCESS(Status))
    {
        ERR("Error init Global Atom.\n");
        return 0;
    }

    if (Atom && PinAtom)
        RtlPinAtomInAtomTable(gAtomTable, Atom);

    return Atom;
}

/*!
 * \brief Returns the name of an atom.
 *
 * \param atom - The atom to be queried.
 * \param pustrName - Pointer to an initialized UNICODE_STRING that receives
 *                    the name of the atom. The function does not update the
                      Length member. The string is always NULL-terminated.
 *
 * \return The length of the name in characters, or 0 if the function fails.
 *
 * \note The function does not aquire any global lock, since synchronisation is
 *       handled by the RtlAtom function.
 */
_Success_(return!=0)
_At_(pustrName->Buffer, _Out_z_bytecap_post_bytecount_(pustrName->MaximumLength, return*2+2))
ULONG
APIENTRY
NtUserGetAtomName(
    _In_ ATOM atom,
    _Inout_ PUNICODE_STRING pustrName)
{
    WCHAR awcBuffer[256];
    ULONG cjLength;

    /* Retrieve the atom name into a local buffer (max length is 255 chars) */
    cjLength = IntGetAtomName((RTL_ATOM)atom, awcBuffer, sizeof(awcBuffer));
    if (cjLength != 0)
    {
        _SEH2_TRY
        {
            /* Probe the unicode string and the buffer */
            ProbeForRead(pustrName, sizeof(*pustrName), 1);
            ProbeForWrite(pustrName->Buffer, pustrName->MaximumLength, 1);

            /* Check if we have enough space to write the NULL termination */
            if (pustrName->MaximumLength >= sizeof(UNICODE_NULL))
            {
                /* Limit the length to the buffer size */
                cjLength = min(pustrName->MaximumLength - sizeof(UNICODE_NULL),
                               cjLength);

                /* Copy the string and NULL terminate it */
                RtlCopyMemory(pustrName->Buffer, awcBuffer, cjLength);
                pustrName->Buffer[cjLength / sizeof(WCHAR)] = L'\0';
            }
            else
            {
                cjLength = 0;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* On exception, set last error and fail */
            SetLastNtError(_SEH2_GetExceptionCode());
            cjLength = 0;
        }
        _SEH2_END
    }

    /* Return the length in characters */
    return cjLength / sizeof(WCHAR);
}

/* EOF */
