#pragma once

NTSTATUS
NTAPI
RtlQueryAtomListInAtomTable(
    IN PRTL_ATOM_TABLE AtomTable,
    IN ULONG MaxAtomCount,
    OUT ULONG *AtomCount,
    OUT RTL_ATOM *AtomList
);

VOID
NTAPI
RtlInitializeRangeListPackage(
    VOID
);

/* EOF */
