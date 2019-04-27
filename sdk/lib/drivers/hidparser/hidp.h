PVOID NTAPI AllocFunction(ULONG Size);
VOID NTAPI FreeFunction(PVOID Item);
VOID NTAPI ZeroFunction(PVOID Item, ULONG Size);
VOID NTAPI CopyFunction(PVOID Target, PVOID Source, ULONG Size);
VOID __cdecl DebugFunction(LPCSTR Src, ...);

