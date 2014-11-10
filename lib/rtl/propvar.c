/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/propvar.c
 * PURPOSE:           Native properties and variants API
 * PROGRAMMER:        Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

UNICODE_STRING Old32Dll = RTL_CONSTANT_STRING(L"ole32.dll");
/* FIXME: (or not)
 * Define those here to allow build. They don't need to be dereferenced
 * so it's OK.
 * Furthermore till Vista those Ole32 API were private so those defines
 * should be made in a private header
 * Finally, having those defined that way allows to write that code plain C.
 */
typedef PVOID PPMemoryAllocator;
typedef PVOID PSERIALIZEDPROPERTYVALUE;

/*
 * @implemented
 */
PVOID
LoadOle32Export(PVOID * BaseAddress, const PCHAR ProcedureName)
{
    NTSTATUS Status;
    ANSI_STRING ExportName;
    PVOID ProcedureAddress;

    /* First load ole32.dll */
    Status = LdrLoadDll(NULL, NULL, &Old32Dll, BaseAddress);
    if (!NT_SUCCESS(Status))
    {
        RtlRaiseStatus(Status);
    }

    RtlInitAnsiString(&ExportName, ProcedureName);

    /* Look for the procedure */
    Status = LdrGetProcedureAddress(*BaseAddress, &ExportName,
                                    0, &ProcedureAddress);
    if (!NT_SUCCESS(Status))
    {
        RtlRaiseStatus(Status);
    }

    /* Return its address */
    return ProcedureAddress;
}

/*
 * @implemented
 */
ULONG
NTAPI
PropertyLengthAsVariant(IN PSERIALIZEDPROPERTYVALUE pProp,
                        IN ULONG cbProp,
                        IN USHORT CodePage,
                        IN BYTE bReserved)
{
    ULONG Length = 0;
    PVOID BaseAddress = NULL;
    ULONG (*ProcedureAddress)(PSERIALIZEDPROPERTYVALUE, ULONG, USHORT, BYTE);

    _SEH2_TRY
    {
        /*  Simply call the appropriate Ole32 export */
        ProcedureAddress = LoadOle32Export(&BaseAddress,
                                           "StgPropertyLengthAsVariant");

        Length = ProcedureAddress(pProp, cbProp, CodePage, bReserved);
    }
    _SEH2_FINALLY
    {
        if (BaseAddress != NULL)
        {
            LdrUnloadDll(BaseAddress);
        }
    }
    _SEH2_END;

    return Length;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlConvertPropertyToVariant(IN PSERIALIZEDPROPERTYVALUE prop,
                            IN USHORT CodePage,
                            OUT PROPVARIANT * pvar,
                            IN PPMemoryAllocator pma)
{
    BOOLEAN Success = FALSE;
    PVOID BaseAddress = NULL;
    BOOLEAN (*ProcedureAddress)(PSERIALIZEDPROPERTYVALUE, USHORT, PROPVARIANT*, PPMemoryAllocator);

    _SEH2_TRY
    {
        /*  Simply call the appropriate Ole32 export */
        ProcedureAddress = LoadOle32Export(&BaseAddress,
                                           "StgConvertPropertyToVariant");

        Success = ProcedureAddress(prop, CodePage, pvar, pma);
    }
    _SEH2_FINALLY
    {
        if (BaseAddress != NULL)
        {
            LdrUnloadDll(BaseAddress);
        }
    }
    _SEH2_END;

    return Success;
}

/*
 * @implemented
 */
PSERIALIZEDPROPERTYVALUE
NTAPI
RtlConvertVariantToProperty(IN const PROPVARIANT * pvar,
                            IN USHORT CodePage,
                            OUT PSERIALIZEDPROPERTYVALUE pprop OPTIONAL,
                            IN OUT PULONG pcb,
                            IN PROPID pid,
                            IN BOOLEAN fReserved,
                            IN OUT PULONG pcIndirect OPTIONAL)
{
    PSERIALIZEDPROPERTYVALUE Serialized = NULL;
    PVOID BaseAddress = NULL;
    PSERIALIZEDPROPERTYVALUE (*ProcedureAddress)(const PROPVARIANT*, USHORT, PSERIALIZEDPROPERTYVALUE,
                                                 PULONG, PROPID, BOOLEAN, PULONG);

    _SEH2_TRY
    {
        /*  Simply call the appropriate Ole32 export */
        ProcedureAddress = LoadOle32Export(&BaseAddress,
                                           "StgConvertVariantToProperty");

        Serialized = ProcedureAddress(pvar, CodePage, pprop, pcb, pid, fReserved, pcIndirect);
    }
    _SEH2_FINALLY
    {
        if (BaseAddress != NULL)
        {
            LdrUnloadDll(BaseAddress);
        }
    }
    _SEH2_END;

    return Serialized;
}


/* EOF */
