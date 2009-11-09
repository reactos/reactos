#ifndef __NTOSKRNL_INCLUDE_INTERNAL_KDB_LOCK_H
#define __NTOSKRNL_INCLUDE_INTERNAL_KDB_LOCK_H

VOID KdbgInitWaitReporting();
VOID KdbgRegisterNamedObject
(PUNICODE_STRING Strings, ULONG NumberOfStrings, PVOID Address);
VOID KdbgDeleteNamedObject(PVOID Address);
VOID KdbgEnterWaitable(PVOID Address);
VOID KdbgLeaveWaitable(PVOID Address);
VOID KdbgDeclareWait(PVOID Address);
VOID KdbgDeclareMultiWait(PVOID *Addresses, ULONG Count);
VOID KdbgSatisfyWait(PVOID Address);
VOID KdbgSatisfyMultiWait(PVOID *Addresses, ULONG Count);
VOID KdbgWaitDescribeThread(ULONG Thread);
VOID KdbgWaitDescribeObject(PVOID Object);

#endif/*__NTOSKRNL_INCLUDE_INTERNAL_KDB_LOCK_H*/
