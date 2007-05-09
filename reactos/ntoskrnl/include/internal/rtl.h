#ifndef __NTOSKRNL_INCLUDE_INTERNAL_NLS_H
#define __NTOSKRNL_INCLUDE_INTERNAL_NLS_H

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

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_NLS_H */

/* EOF */
