/* $Id: thread.h,v 1.2 2003/05/29 00:36:41 hyperion Exp $
 */

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS STDCALL RtlRosCreateUserThreadEx
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
);

NTSTATUS CDECL RtlRosCreateUserThreadVa
(
 IN HANDLE ProcessHandle,
 IN POBJECT_ATTRIBUTES ObjectAttributes,
 IN BOOLEAN CreateSuspended,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL,
 IN PVOID StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 ...
);

NTSTATUS NTAPI RtlRosInitializeContextEx
(
 IN HANDLE ProcessHandle,
 OUT PCONTEXT Context,
 IN PVOID StartAddress,
 IN PUSER_STACK UserStack,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
);

NTSTATUS NTAPI RtlRosCreateStack
(
 IN HANDLE ProcessHandle,
 OUT PUSER_STACK UserStack,
 IN LONG StackZeroBits,
 IN OUT PULONG StackReserve OPTIONAL,
 IN OUT PULONG StackCommit OPTIONAL
);

NTSTATUS NTAPI RtlRosDeleteStack
(
 IN HANDLE ProcessHandle,
 IN PUSER_STACK UserStack
);

#ifdef __cplusplus
}
#endif

/* EOF */
