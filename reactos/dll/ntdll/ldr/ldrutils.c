/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrutils.c
 * PURPOSE:         Internal Loader Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
LdrpCallDllEntry(PDLLMAIN_FUNC EntryPoint,
                 PVOID BaseAddress,
                 ULONG Reason,
                 PVOID Context)
{
    /* Call the entry */
    return EntryPoint(BaseAddress, Reason, Context);
}

VOID
NTAPI
LdrpTlsCallback(PVOID BaseAddress, ULONG Reason)
{
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PIMAGE_TLS_CALLBACK *Array, Callback;
    ULONG Size;

    /* Get the TLS Directory */
    TlsDirectory = RtlImageDirectoryEntryToData(BaseAddress,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_TLS,
                                                &Size);

    /* Protect against invalid pointers */
    _SEH2_TRY
    {
        /* Make sure it's valid and we have an array */
        if (TlsDirectory && (Array = (PIMAGE_TLS_CALLBACK *)TlsDirectory->AddressOfCallBacks))
        {
            /* Display debug */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Tls Callbacks Found. Imagebase %p Tls %p CallBacks %p\n",
                        BaseAddress, TlsDirectory, Array);
            }

            /* Loop the array */
            while (*Array)
            {
                /* Get the TLS Entrypoint */
                Callback = *Array++;

                /* Display debug */
                if (ShowSnaps)
                {
                    DPRINT1("LDR: Calling Tls Callback Imagebase %p Function %p\n",
                            BaseAddress, Callback);
                }

                /* Call it */
                LdrpCallDllEntry((PDLLMAIN_FUNC)Callback, BaseAddress, Reason, NULL);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;
}

/* EOF */
