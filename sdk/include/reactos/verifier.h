#ifndef REACTOS_VERIFIER_H
#define REACTOS_VERIFIER_H

#define DLL_PROCESS_VERIFIER 4

typedef VOID (NTAPI* RTL_VERIFIER_DLL_LOAD_CALLBACK) (PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved);
typedef VOID (NTAPI* RTL_VERIFIER_DLL_UNLOAD_CALLBACK) (PWSTR DllName, PVOID DllBase, SIZE_T DllSize, PVOID Reserved);
typedef VOID (NTAPI* RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK) (PVOID AllocationBase, SIZE_T AllocationSize);

typedef struct _RTL_VERIFIER_THUNK_DESCRIPTOR {
    PCHAR ThunkName;
    PVOID ThunkOldAddress;
    PVOID ThunkNewAddress;
} RTL_VERIFIER_THUNK_DESCRIPTOR, *PRTL_VERIFIER_THUNK_DESCRIPTOR;

typedef struct _RTL_VERIFIER_DLL_DESCRIPTOR {
    PWCHAR DllName;
    DWORD DllFlags;
    PVOID DllAddress;
    PRTL_VERIFIER_THUNK_DESCRIPTOR DllThunks;
} RTL_VERIFIER_DLL_DESCRIPTOR, *PRTL_VERIFIER_DLL_DESCRIPTOR;

typedef struct _RTL_VERIFIER_PROVIDER_DESCRIPTOR {
    // Provider fields
    DWORD Length;
    PRTL_VERIFIER_DLL_DESCRIPTOR ProviderDlls;
    RTL_VERIFIER_DLL_LOAD_CALLBACK ProviderDllLoadCallback;
    RTL_VERIFIER_DLL_UNLOAD_CALLBACK ProviderDllUnloadCallback;

    // Verifier fields
    PWSTR VerifierImage;
    DWORD VerifierFlags;
    DWORD VerifierDebug;
    PVOID RtlpGetStackTraceAddress;
    PVOID RtlpDebugPageHeapCreate;
    PVOID RtlpDebugPageHeapDestroy;

    // Provider field
    RTL_VERIFIER_NTDLLHEAPFREE_CALLBACK ProviderNtdllHeapFreeCallback;
} RTL_VERIFIER_PROVIDER_DESCRIPTOR, *PRTL_VERIFIER_PROVIDER_DESCRIPTOR;


// VerifierFlags

#define RTL_VRF_FLG_FULL_PAGE_HEAP                      0x00000001
#define RTL_VRF_FLG_RESERVED_DONOTUSE                   0x00000002
#define RTL_VRF_FLG_HANDLE_CHECKS                       0x00000004
#define RTL_VRF_FLG_STACK_CHECKS                        0x00000008
#define RTL_VRF_FLG_APPCOMPAT_CHECKS                    0x00000010
#define RTL_VRF_FLG_TLS_CHECKS                          0x00000020
#define RTL_VRF_FLG_DIRTY_STACKS                        0x00000040
#define RTL_VRF_FLG_RPC_CHECKS                          0x00000080
#define RTL_VRF_FLG_COM_CHECKS                          0x00000100
#define RTL_VRF_FLG_DANGEROUS_APIS                      0x00000200
#define RTL_VRF_FLG_RACE_CHECKS                         0x00000400
#define RTL_VRF_FLG_DEADLOCK_CHECKS                     0x00000800
#define RTL_VRF_FLG_FIRST_CHANCE_EXCEPTION_CHECKS       0x00001000
#define RTL_VRF_FLG_VIRTUAL_MEM_CHECKS                  0x00002000
#define RTL_VRF_FLG_ENABLE_LOGGING                      0x00004000
#define RTL_VRF_FLG_FAST_FILL_HEAP                      0x00008000
#define RTL_VRF_FLG_VIRTUAL_SPACE_TRACKING              0x00010000
#define RTL_VRF_FLG_ENABLED_SYSTEM_WIDE                 0x00020000
#define RTL_VRF_FLG_MISCELLANEOUS_CHECKS                0x00020000
#define RTL_VRF_FLG_LOCK_CHECKS                         0x00040000


// VerifierDebug

#define RTL_VRF_DBG_SHOWSNAPS               0x00001
#define RTL_VRF_DBG_SHOWFOUNDEXPORTS        0x00002
#define RTL_VRF_DBG_SHOWVERIFIEDEXPORTS     0x00004
#define RTL_VRF_DBG_LISTPROVIDERS           0x00008
#define RTL_VRF_DBG_SHOWCHAINING            0x00010
#define RTL_VRF_DBG_SHOWCHAINING_DEBUG      0x00020

#define RTL_VRF_DBG_CS_SPLAYTREE            0x00200
#define RTL_VRF_DBG_CS_DUMP_SPLAYTREE       0x00400
#define RTL_VRF_DBG_CS_CREATE_DELETE        0x00800

#define RTL_VRF_DBG_VERIFIER_LOGCALLS       0x04000
#define RTL_VRF_DBG_VERIFIER_SHOWDYNTHUNKS  0x08000

#define RTL_VRF_DBG_ENTRYPOINT_HOOKS        0x10000
#define RTL_VRF_DBG_ENTRYPOINT_CALLS        0x20000

#endif // REACTOS_VERIFIER_H
