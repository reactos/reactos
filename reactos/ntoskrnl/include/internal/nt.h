#ifndef __INCLUDE_INTERNAL_NT_H
#define __INCLUDE_INTERNAL_NT_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifndef AS_INVOKED

VOID NtInitializeEventImplementation(VOID);
VOID NtInitializeEventPairImplementation(VOID);
VOID NtInitializeSemaphoreImplementation(VOID);
VOID NtInitializeMutantImplementation(VOID);
VOID NtInitializeTimerImplementation(VOID);
NTSTATUS NiInitPort(VOID);
VOID NtInitializeProfileImplementation(VOID);

#endif /* !AS_INVOKED */

#endif
