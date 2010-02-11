#pragma once

ULONG NTAPI DebugService(IN ULONG Service, IN PVOID Argument1, IN PVOID Argument2, IN PVOID Argument3, IN PVOID Argument4);
VOID NTAPI DebugService2(IN PVOID Argument1, IN PVOID Argument2, IN ULONG Service);

BOOLEAN NTAPI RtlpCheckForActiveDebugger(VOID);
