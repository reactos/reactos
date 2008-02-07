#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

VOID
NTAPI
KeArmInitThreadWithContext(
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context
);

#define KeArchInitThreadWithContext KeArmInitThreadWithContext

#endif