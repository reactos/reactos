/* $Id: thread.h,v 1.1 2003/04/29 02:17:00 hyperion Exp $
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
 IN PTHREAD_START_ROUTINE StartAddress,
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
 IN PTHREAD_START_ROUTINE StartAddress,
 OUT PHANDLE ThreadHandle OPTIONAL,
 OUT PCLIENT_ID ClientId OPTIONAL,
 IN ULONG ParameterCount,
 ...
);

NTSTATUS NTAPI RtlRosInitializeContextEx
(
 IN HANDLE ProcessHandle,
 IN PCONTEXT Context,
 IN PTHREAD_START_ROUTINE StartAddress,
 IN PUSER_STACK UserStack,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
);

#ifdef __cplusplus
}
#endif

/* EOF */
