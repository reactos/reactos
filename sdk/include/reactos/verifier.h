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



// Verifier stop codes
#define APPLICATION_VERIFIER_INTERNAL_ERROR                                 0x80000000
#define APPLICATION_VERIFIER_INTERNAL_WARNING                               0x40000000
#define APPLICATION_VERIFIER_NO_BREAK                                       0x20000000
#define APPLICATION_VERIFIER_CONTINUABLE_BREAK                              0x10000000

#define APPLICATION_VERIFIER_UNKNOWN_ERROR                                      0x0001
#define APPLICATION_VERIFIER_ACCESS_VIOLATION                                   0x0002
#define APPLICATION_VERIFIER_UNSYNCHRONIZED_ACCESS                              0x0003
#define APPLICATION_VERIFIER_EXTREME_SIZE_REQUEST                               0x0004
#define APPLICATION_VERIFIER_BAD_HEAP_HANDLE                                    0x0005
#define APPLICATION_VERIFIER_SWITCHED_HEAP_HANDLE                               0x0006
#define APPLICATION_VERIFIER_DOUBLE_FREE                                        0x0007
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK                               0x0008
#define APPLICATION_VERIFIER_DESTROY_PROCESS_HEAP                               0x0009
#define APPLICATION_VERIFIER_UNEXPECTED_EXCEPTION                               0x000A
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_EXCEPTION_RAISED_FOR_HEADER   0x000B
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_EXCEPTION_RAISED_FOR_PROBING  0x000C
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_HEADER                        0x000D
#define APPLICATION_VERIFIER_CORRUPTED_FREED_HEAP_BLOCK                         0x000E
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_SUFFIX                        0x000F
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_START_STAMP                   0x0010
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_END_STAMP                     0x0011
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_BLOCK_PREFIX                        0x0012
#define APPLICATION_VERIFIER_FIRST_CHANCE_ACCESS_VIOLATION                      0x0013
#define APPLICATION_VERIFIER_CORRUPTED_HEAP_LIST                                0x0014

#define APPLICATION_VERIFIER_TERMINATE_THREAD_CALL                              0x0100
#define APPLICATION_VERIFIER_STACK_OVERFLOW                                     0x0101
#define APPLICATION_VERIFIER_INVALID_EXIT_PROCESS_CALL                          0x0102

#define APPLICATION_VERIFIER_EXIT_THREAD_OWNS_LOCK                              0x0200
#define APPLICATION_VERIFIER_LOCK_IN_UNLOADED_DLL                               0x0201
#define APPLICATION_VERIFIER_LOCK_IN_FREED_HEAP                                 0x0202
#define APPLICATION_VERIFIER_LOCK_DOUBLE_INITIALIZE                             0x0203
#define APPLICATION_VERIFIER_LOCK_IN_FREED_MEMORY                               0x0204
#define APPLICATION_VERIFIER_LOCK_CORRUPTED                                     0x0205
#define APPLICATION_VERIFIER_LOCK_INVALID_OWNER                                 0x0206
#define APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT                       0x0207
#define APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT                            0x0208
#define APPLICATION_VERIFIER_LOCK_OVER_RELEASED                                 0x0209
#define APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED                               0x0210
#define APPLICATION_VERIFIER_LOCK_ALREADY_INITIALIZED                           0x0211
#define APPLICATION_VERIFIER_LOCK_IN_FREED_VMEM                                 0x0212
#define APPLICATION_VERIFIER_LOCK_IN_UNMAPPED_MEM                               0x0213
#define APPLICATION_VERIFIER_THREAD_NOT_LOCK_OWNER                              0x0214

#define APPLICATION_VERIFIER_INVALID_HANDLE                                     0x0300
#define APPLICATION_VERIFIER_INVALID_TLS_VALUE                                  0x0301
#define APPLICATION_VERIFIER_INCORRECT_WAIT_CALL                                0x0302
#define APPLICATION_VERIFIER_NULL_HANDLE                                        0x0303
#define APPLICATION_VERIFIER_WAIT_IN_DLLMAIN                                    0x0304

#define APPLICATION_VERIFIER_COM_ERROR                                          0x0400
#define APPLICATION_VERIFIER_COM_API_IN_DLLMAIN                                 0x0401
#define APPLICATION_VERIFIER_COM_UNHANDLED_EXCEPTION                            0x0402
#define APPLICATION_VERIFIER_COM_UNBALANCED_COINIT                              0x0403
#define APPLICATION_VERIFIER_COM_UNBALANCED_OLEINIT                             0x0404
#define APPLICATION_VERIFIER_COM_UNBALANCED_SWC                                 0x0405
#define APPLICATION_VERIFIER_COM_NULL_DACL                                      0x0406
#define APPLICATION_VERIFIER_COM_UNSAFE_IMPERSONATION                           0x0407
#define APPLICATION_VERIFIER_COM_SMUGGLED_WRAPPER                               0x0408
#define APPLICATION_VERIFIER_COM_SMUGGLED_PROXY                                 0x0409
#define APPLICATION_VERIFIER_COM_CF_SUCCESS_WITH_NULL                           0x040A
#define APPLICATION_VERIFIER_COM_GCO_SUCCESS_WITH_NULL                          0x040B
#define APPLICATION_VERIFIER_COM_OBJECT_IN_FREED_MEMORY                         0x040C
#define APPLICATION_VERIFIER_COM_OBJECT_IN_UNLOADED_DLL                         0x040D
#define APPLICATION_VERIFIER_COM_VTBL_IN_FREED_MEMORY                           0x040E
#define APPLICATION_VERIFIER_COM_VTBL_IN_UNLOADED_DLL                           0x040F
#define APPLICATION_VERIFIER_COM_HOLDING_LOCKS_ON_CALL                          0x0410

#define APPLICATION_VERIFIER_RPC_ERROR                                          0x0500

#define APPLICATION_VERIFIER_INVALID_FREEMEM                                    0x0600
#define APPLICATION_VERIFIER_INVALID_ALLOCMEM                                   0x0601
#define APPLICATION_VERIFIER_INVALID_MAPVIEW                                    0x0602
#define APPLICATION_VERIFIER_PROBE_INVALID_ADDRESS                              0x0603
#define APPLICATION_VERIFIER_PROBE_FREE_MEM                                     0x0604
#define APPLICATION_VERIFIER_PROBE_GUARD_PAGE                                   0x0605
#define APPLICATION_VERIFIER_PROBE_NULL                                         0x0606
#define APPLICATION_VERIFIER_PROBE_INVALID_START_OR_SIZE                        0x0607
#define APPLICATION_VERIFIER_SIZE_HEAP_UNEXPECTED_EXCEPTION                     0x0618

#define VERIFIER_STOP(Code, Msg, Val1, Desc1, Val2, Desc2, Val3, Desc3, Val4, Desc4)    \
    do {                                                                                \
        RtlApplicationVerifierStop((Code),                                              \
                                   (Msg),                                               \
                                   (Val1), (Desc1),                                     \
                                   (Val2), (Desc2),                                     \
                                   (Val3), (Desc3),                                     \
                                   (Val4), (Desc4));                                   \
    } while (0)


VOID
NTAPI
RtlApplicationVerifierStop(
    _In_ ULONG_PTR Code,
    _In_ PCSTR Message,
    _In_ PVOID Value1,
    _In_ PCSTR Description1,
    _In_ PVOID Value2,
    _In_ PCSTR Description2,
    _In_ PVOID Value3,
    _In_ PCSTR Description3,
    _In_ PVOID Value4,
    _In_ PCSTR Description4);


#endif // REACTOS_VERIFIER_H
