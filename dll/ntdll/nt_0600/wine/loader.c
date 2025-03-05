/*
 * Loader functions
 *
 * Copyright 1995, 2003 Alexandre Julliard
 * Copyright 2002 Dmitry Timoshkov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winioctl.h"
#include "winternl.h"
#include "delayloadhandler.h"

#include "wine/exception.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "ddk/ntddk.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);
WINE_DECLARE_DEBUG_CHANNEL(imports);

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE_64  (((ULONGLONG)0x00002b99 << 32) | 0x2ddfa232)
#endif
#define DEFAULT_SECURITY_COOKIE_32  0xbb40e64e
#define DEFAULT_SECURITY_COOKIE_16  (DEFAULT_SECURITY_COOKIE_32 >> 16)

#ifdef __i386__
static const WCHAR pe_dir[] = L"\\i386-windows";
#elif defined __x86_64__
static const WCHAR pe_dir[] = L"\\x86_64-windows";
#elif defined __arm__
static const WCHAR pe_dir[] = L"\\arm-windows";
#elif defined __aarch64__
static const WCHAR pe_dir[] = L"\\aarch64-windows";
#else
static const WCHAR pe_dir[] = L"";
#endif

/* we don't want to include winuser.h */
#define RT_MANIFEST                         ((ULONG_PTR)24)
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID ((ULONG_PTR)2)

typedef DWORD (CALLBACK *DLLENTRYPROC)(HMODULE,DWORD,LPVOID);
typedef void  (CALLBACK *LDRENUMPROC)(LDR_DATA_TABLE_ENTRY *, void *, BOOLEAN *);

void (FASTCALL *pBaseThreadInitThunk)(DWORD,LPTHREAD_START_ROUTINE,void *) = NULL;
NTSTATUS (WINAPI *__wine_unix_call_dispatcher)( unixlib_handle_t, unsigned int, void * ) = NULL;

static DWORD (WINAPI *pCtrlRoutine)(void *);

SYSTEM_DLL_INIT_BLOCK LdrSystemDllInitBlock = { 0xf0 };

void *__wine_syscall_dispatcher = NULL;
unixlib_handle_t __wine_unixlib_handle = 0;

/* windows directory */
const WCHAR windows_dir[] = L"C:\\windows";
/* system directory with trailing backslash */
const WCHAR system_dir[] = L"C:\\windows\\system32\\";

/* system search path */
static const WCHAR system_path[] = L"C:\\windows\\system32;C:\\windows\\system;C:\\windows";

static BOOL is_prefix_bootstrap;  /* are we bootstrapping the prefix? */
static BOOL imports_fixup_done = FALSE;  /* set once the imports have been fixed up, before attaching them */
static BOOL process_detaching = FALSE;  /* set on process detach to avoid deadlocks with thread detach */
static int free_lib_count;   /* recursion depth of LdrUnloadDll calls */
static LONG path_safe_mode;  /* path mode set by RtlSetSearchPathMode */
static LONG dll_safe_mode = 1;  /* dll search mode */
static UNICODE_STRING dll_directory;  /* extra path for LdrSetDllDirectory */
static UNICODE_STRING system_dll_path; /* path to search for system dependency dlls */
static DWORD default_search_flags;  /* default flags set by LdrSetDefaultDllDirectories */
static WCHAR *default_load_path;    /* default dll search path */

struct dll_dir_entry
{
    struct list entry;
    WCHAR       dir[1];
};

static struct list dll_dir_list = LIST_INIT( dll_dir_list );  /* extra dirs from LdrAddDllDirectory */

struct ldr_notification
{
    struct list                    entry;
    PLDR_DLL_NOTIFICATION_FUNCTION callback;
    void                           *context;
};

static struct list ldr_notifications = LIST_INIT( ldr_notifications );

static const char * const reason_names[] =
{
    "PROCESS_DETACH",
    "PROCESS_ATTACH",
    "THREAD_ATTACH",
    "THREAD_DETACH",
};

struct file_id
{
    BYTE ObjectId[16];
};

#define HASH_MAP_SIZE 32
static LIST_ENTRY hash_table[HASH_MAP_SIZE];

/* internal representation of loaded modules */
typedef struct _wine_modref
{
    LDR_DATA_TABLE_ENTRY  ldr;
    struct file_id        id;
    ULONG                 CheckSum;
    BOOL                  system;
} WINE_MODREF;

static UINT tls_module_count = 32;     /* number of modules with TLS directory */
static IMAGE_TLS_DIRECTORY *tls_dirs;  /* array of TLS directories */

static RTL_CRITICAL_SECTION loader_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &loader_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": loader_section") }
};
static RTL_CRITICAL_SECTION loader_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static CRITICAL_SECTION dlldir_section;
static CRITICAL_SECTION_DEBUG dlldir_critsect_debug =
{
    0, 0, &dlldir_section,
    { &dlldir_critsect_debug.ProcessLocksList, &dlldir_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dlldir_section") }
};
static CRITICAL_SECTION dlldir_section = { &dlldir_critsect_debug, -1, 0, 0, 0, 0 };

static RTL_CRITICAL_SECTION peb_lock;
static RTL_CRITICAL_SECTION_DEBUG peb_critsect_debug =
{
    0, 0, &peb_lock,
    { &peb_critsect_debug.ProcessLocksList, &peb_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": peb_lock") }
};
static RTL_CRITICAL_SECTION peb_lock = { &peb_critsect_debug, -1, 0, 0, 0, 0 };

static PEB_LDR_DATA ldr =
{
    sizeof(ldr), TRUE, NULL,
    { &ldr.InLoadOrderModuleList, &ldr.InLoadOrderModuleList },
    { &ldr.InMemoryOrderModuleList, &ldr.InMemoryOrderModuleList },
    { &ldr.InInitializationOrderModuleList, &ldr.InInitializationOrderModuleList }
};

static RTL_RB_TREE base_address_index_tree;

static RTL_BITMAP tls_bitmap;
static RTL_BITMAP tls_expansion_bitmap;

static WINE_MODREF *cached_modref;
static WINE_MODREF *current_modref;
static WINE_MODREF *last_failed_modref;

static LDR_DDAG_NODE *node_ntdll, *node_kernel32;

static NTSTATUS load_dll( const WCHAR *load_path, const WCHAR *libname, DWORD flags, WINE_MODREF** pwm, BOOL system );
static NTSTATUS process_attach( LDR_DDAG_NODE *node, LPVOID lpReserved );
static FARPROC find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    DWORD exp_size, DWORD ordinal, LPCWSTR load_path );
static FARPROC find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint, LPCWSTR load_path );

/* check whether the file name contains a path */
static inline BOOL contains_path( LPCWSTR name )
{
    return ((*name && (name[1] == ':')) || wcschr(name, '/') || wcschr(name, '\\'));
}

#define RTL_UNLOAD_EVENT_TRACE_NUMBER 64

typedef struct _RTL_UNLOAD_EVENT_TRACE
{
    void *BaseAddress;
    SIZE_T SizeOfImage;
    ULONG Sequence;
    ULONG TimeDateStamp;
    ULONG CheckSum;
    WCHAR ImageName[32];
} RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

static RTL_UNLOAD_EVENT_TRACE unload_traces[RTL_UNLOAD_EVENT_TRACE_NUMBER];
static RTL_UNLOAD_EVENT_TRACE *unload_trace_ptr;
static unsigned int unload_trace_seq;

static void module_push_unload_trace( const WINE_MODREF *wm )
{
    RTL_UNLOAD_EVENT_TRACE *ptr = &unload_traces[unload_trace_seq];
    const LDR_DATA_TABLE_ENTRY *ldr = &wm->ldr;
    unsigned int len = min(sizeof(ptr->ImageName) - sizeof(WCHAR), ldr->BaseDllName.Length);

    ptr->BaseAddress = ldr->DllBase;
    ptr->SizeOfImage = ldr->SizeOfImage;
    ptr->Sequence = unload_trace_seq;
    ptr->TimeDateStamp = ldr->TimeDateStamp;
    ptr->CheckSum = wm->CheckSum;
    memcpy(ptr->ImageName, ldr->BaseDllName.Buffer, len);
    ptr->ImageName[len / sizeof(*ptr->ImageName)] = 0;

    unload_trace_seq = (unload_trace_seq + 1) % ARRAY_SIZE(unload_traces);
    unload_trace_ptr = unload_traces;
}

static int rtl_rb_tree_put( RTL_RB_TREE *tree, const void *key, RTL_BALANCED_NODE *entry,
                            int (*compare_func)( const void *key, const RTL_BALANCED_NODE *entry ))
{
    RTL_BALANCED_NODE *parent = tree->root;
    BOOLEAN right = 0;
    int c;

    while (parent)
    {
        if (!(c = compare_func( key, parent ))) return -1;
        right = c > 0;
        if (!parent->Children[right]) break;
        parent = parent->Children[right];
    }
    RtlRbInsertNodeEx( tree, parent, right, entry );
    return 0;
}

static RTL_BALANCED_NODE *rtl_rb_tree_get( RTL_RB_TREE *tree, const void *key,
                                           int (*compare_func)( const void *key, const RTL_BALANCED_NODE *entry ))
{
    RTL_BALANCED_NODE *parent = tree->root;
    int c;

    while (parent)
    {
        if (!(c = compare_func( key, parent ))) return parent;
        parent = parent->Children[c > 0];
    }
    return NULL;
}


/*********************************************************************
 *           RtlGetUnloadEventTrace [NTDLL.@]
 */
RTL_UNLOAD_EVENT_TRACE * WINAPI RtlGetUnloadEventTrace(void)
{
    return unload_traces;
}

/*********************************************************************
 *           RtlGetUnloadEventTraceEx [NTDLL.@]
 */
void WINAPI RtlGetUnloadEventTraceEx(ULONG **size, ULONG **count, void **trace)
{
    static ULONG element_size = sizeof(*unload_traces);
    static ULONG element_count = ARRAY_SIZE(unload_traces);

    *size = &element_size;
    *count = &element_count;
    *trace = &unload_trace_ptr;
}

/*************************************************************************
 *		call_dll_entry_point
 *
 * Some brain-damaged dlls (ir32_32.dll for instance) modify ebx in
 * their entry point, so we need a small asm wrapper. Testing indicates
 * that only modifying esi leads to a crash, so use this one to backup
 * ebp while running the dll entry proc.
 */
#if defined(__i386__)
extern BOOL call_dll_entry_point( DLLENTRYPROC proc, void *module, UINT reason, void *reserved );
__ASM_GLOBAL_FUNC(call_dll_entry_point,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "pushl %ebx\n\t"
                  __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                  "pushl %esi\n\t"
                  __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                  "pushl %edi\n\t"
                  __ASM_CFI(".cfi_rel_offset %edi,-12\n\t")
                  "movl %ebp,%esi\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %esi\n\t")
                  "pushl 20(%ebp)\n\t"
                  "pushl 16(%ebp)\n\t"
                  "pushl 12(%ebp)\n\t"
                  "movl 8(%ebp),%eax\n\t"
                  "call *%eax\n\t"
                  "movl %esi,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "leal -12(%ebp),%esp\n\t"
                  "popl %edi\n\t"
                  __ASM_CFI(".cfi_same_value %edi\n\t")
                  "popl %esi\n\t"
                  __ASM_CFI(".cfi_same_value %esi\n\t")
                  "popl %ebx\n\t"
                  __ASM_CFI(".cfi_same_value %ebx\n\t")
                  "popl %ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret" )
#elif defined(__x86_64__) && !defined(__arm64ec__)
extern BOOL CDECL call_dll_entry_point( DLLENTRYPROC proc, void *module, UINT reason, void *reserved );
/* Some apps modify rbx in TLS entry point. */
__ASM_GLOBAL_FUNC(call_dll_entry_point,
                  "pushq %rbx\n\t"
                  __ASM_SEH(".seh_pushreg %rbx\n\t")
                  __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                  __ASM_CFI(".cfi_rel_offset %rbx,0\n\t")
                  "subq $48,%rsp\n\t"
                  __ASM_SEH(".seh_stackalloc 48\n\t")
                  __ASM_SEH(".seh_endprologue\n\t")
                  __ASM_CFI(".cfi_adjust_cfa_offset 48\n\t")
                  "mov %rcx,%r10\n\t"
                  "mov %rdx,%rcx\n\t"
                  "mov %r8d,%edx\n\t"
                  "mov %r9,%r8\n\t"
                  "call *%r10\n\t"
                  "addq $48,%rsp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset -48\n\t")
                  "popq %rbx\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbx\n\t")
                  "ret" )
#else
static inline BOOL call_dll_entry_point( DLLENTRYPROC proc, void *module,
                                         UINT reason, void *reserved )
{
    return proc( module, reason, reserved );
}
#endif


#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
/*************************************************************************
 *		stub_entry_point
 *
 * Entry point for stub functions.
 */
static void WINAPI stub_entry_point( const char *dll, const char *name, void *ret_addr )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionCode           = EXCEPTION_WINE_STUB;
    rec.ExceptionFlags          = EXCEPTION_NONCONTINUABLE;
    rec.ExceptionRecord         = NULL;
    rec.ExceptionAddress        = ret_addr;
    rec.NumberParameters        = 2;
    rec.ExceptionInformation[0] = (ULONG_PTR)dll;
    rec.ExceptionInformation[1] = (ULONG_PTR)name;
    for (;;) RtlRaiseException( &rec );
}


#include "pshpack1.h"
#ifdef __i386__
struct stub
{
    BYTE        pushl1;     /* pushl $name */
    const char *name;
    BYTE        pushl2;     /* pushl $dll */
    const char *dll;
    BYTE        call;       /* call stub_entry_point */
    DWORD       entry;
};
#elif defined(__arm__)
struct stub
{
    DWORD ldr_r0;        /* ldr r0, $dll */
    DWORD ldr_r1;        /* ldr r1, $name */
    DWORD mov_r2_lr;     /* mov r2, lr */
    DWORD ldr_pc_pc;     /* ldr pc, [pc, #4] */
    const char *dll;
    const char *name;
    const void* entry;
};
#elif defined(__aarch64__)
struct stub
{
    DWORD ldr_x0;        /* ldr x0, $dll */
    DWORD ldr_x1;        /* ldr x1, $name */
    DWORD mov_x2_lr;     /* mov x2, lr */
    DWORD ldr_x16;       /* ldr x16, $entry */
    DWORD br_x16;        /* br x16 */
    const char *dll;
    const char *name;
    const void *entry;
};
#else
struct stub
{
    BYTE movq_rdi[2];      /* movq $dll,%rdi */
    const char *dll;
    BYTE movq_rsi[2];      /* movq $name,%rsi */
    const char *name;
    BYTE movq_rsp_rdx[4];  /* movq (%rsp),%rdx */
    BYTE movq_rax[2];      /* movq $entry, %rax */
    const void* entry;
    BYTE jmpq_rax[2];      /* jmp %rax */
};
#endif
#include "poppack.h"

/*************************************************************************
 *		allocate_stub
 *
 * Allocate a stub entry point.
 */
static ULONG_PTR allocate_stub( const char *dll, const char *name )
{
#define MAX_SIZE 65536
    static struct stub *stubs;
    static unsigned int nb_stubs;
    struct stub *stub;

    if (nb_stubs >= MAX_SIZE / sizeof(*stub)) return 0xdeadbeef;

    if (!stubs)
    {
        SIZE_T size = MAX_SIZE;
        if (NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&stubs, 0, &size,
                                     MEM_COMMIT, PAGE_EXECUTE_READWRITE ) != STATUS_SUCCESS)
            return 0xdeadbeef;
    }
    stub = &stubs[nb_stubs++];
#ifdef __i386__
    stub->pushl1    = 0x68;  /* pushl $name */
    stub->name      = name;
    stub->pushl2    = 0x68;  /* pushl $dll */
    stub->dll       = dll;
    stub->call      = 0xe8;  /* call stub_entry_point */
    stub->entry     = (BYTE *)stub_entry_point - (BYTE *)(&stub->entry + 1);
#elif defined(__arm__)
    stub->ldr_r0    = 0xe59f0008;   /* ldr r0, [pc, #8] ($dll) */
    stub->ldr_r1    = 0xe59f1008;   /* ldr r1, [pc, #8] ($name) */
    stub->mov_r2_lr = 0xe1a0200e;   /* mov r2, lr */
    stub->ldr_pc_pc = 0xe59ff004;   /* ldr pc, [pc, #4] */
    stub->dll       = dll;
    stub->name      = name;
    stub->entry     = stub_entry_point;
#elif defined(__aarch64__)
    stub->ldr_x0    = 0x580000a0; /* ldr x0, #20 ($dll) */
    stub->ldr_x1    = 0x580000c1; /* ldr x1, #24 ($name) */
    stub->mov_x2_lr = 0xaa1e03e2; /* mov x2, lr */
    stub->ldr_x16   = 0x580000d0; /* ldr x16, #24 ($entry) */
    stub->br_x16    = 0xd61f0200; /* br x16 */
    stub->dll       = dll;
    stub->name      = name;
    stub->entry     = stub_entry_point;
#else
    stub->movq_rdi[0]     = 0x48;  /* movq $dll,%rcx */
    stub->movq_rdi[1]     = 0xb9;
    stub->dll             = dll;
    stub->movq_rsi[0]     = 0x48;  /* movq $name,%rdx */
    stub->movq_rsi[1]     = 0xba;
    stub->name            = name;
    stub->movq_rsp_rdx[0] = 0x4c;  /* movq (%rsp),%r8 */
    stub->movq_rsp_rdx[1] = 0x8b;
    stub->movq_rsp_rdx[2] = 0x04;
    stub->movq_rsp_rdx[3] = 0x24;
    stub->movq_rax[0]     = 0x48;  /* movq $entry, %rax */
    stub->movq_rax[1]     = 0xb8;
    stub->entry           = stub_entry_point;
    stub->jmpq_rax[0]     = 0xff;  /* jmp %rax */
    stub->jmpq_rax[1]     = 0xe0;
#endif
    return (ULONG_PTR)stub;
}

#else  /* __i386__ */
static inline ULONG_PTR allocate_stub( const char *dll, const char *name ) { return 0xdeadbeef; }
#endif  /* __i386__ */

/* call ldr notifications */
static void call_ldr_notifications( ULONG reason, LDR_DATA_TABLE_ENTRY *module )
{
    struct ldr_notification *notify, *notify_next;
    LDR_DLL_NOTIFICATION_DATA data;

    if (process_detaching && reason == LDR_DLL_NOTIFICATION_REASON_UNLOADED) return;

    data.Loaded.Flags       = 0;
    data.Loaded.FullDllName = &module->FullDllName;
    data.Loaded.BaseDllName = &module->BaseDllName;
    data.Loaded.DllBase     = module->DllBase;
    data.Loaded.SizeOfImage = module->SizeOfImage;

    LIST_FOR_EACH_ENTRY_SAFE( notify, notify_next, &ldr_notifications, struct ldr_notification, entry )
    {
        TRACE_(relay)("\1Call LDR notification callback (proc=%p,reason=%lu,data=%p,context=%p)\n",
                notify->callback, reason, &data, notify->context );

        notify->callback(reason, &data, notify->context);

        TRACE_(relay)("\1Ret  LDR notification callback (proc=%p,reason=%lu,data=%p,context=%p)\n",
                notify->callback, reason, &data, notify->context );
    }
}

/* compare base address */
static int base_address_compare( const void *key, const RTL_BALANCED_NODE *entry )
{
    const LDR_DATA_TABLE_ENTRY *mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, BaseAddressIndexNode);
    const char *base = key;

    if (base < (char *)mod->DllBase) return -1;
    if (base > (char *)mod->DllBase) return 1;
    return 0;
}

/* compute basename hash */
static ULONG hash_basename( const UNICODE_STRING *basename )
{
    ULONG hash = 0;

    RtlHashUnicodeString( basename, TRUE, HASH_STRING_ALGORITHM_DEFAULT, &hash );
    return hash % HASH_MAP_SIZE;
}

/*************************************************************************
 *		get_modref
 *
 * Looks for the referenced HMODULE in the current process
 * The loader_section must be locked while calling this function.
 */
static WINE_MODREF *get_modref( HMODULE hmod )
{
    PLDR_DATA_TABLE_ENTRY mod;
    RTL_BALANCED_NODE *node;

    if (cached_modref && cached_modref->ldr.DllBase == hmod) return cached_modref;

    if (!(node = rtl_rb_tree_get( &base_address_index_tree, hmod, base_address_compare ))) return NULL;
    mod = CONTAINING_RECORD(node, LDR_DATA_TABLE_ENTRY, BaseAddressIndexNode);
    return cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
}


/**********************************************************************
 *	    find_basename_module
 *
 * Find a module from its base name.
 * The loader_section must be locked while calling this function
 */
static WINE_MODREF *find_basename_module( LPCWSTR name )
{
    PLIST_ENTRY mark, entry;
    UNICODE_STRING name_str;

    RtlInitUnicodeString( &name_str, name );

    if (cached_modref && RtlEqualUnicodeString( &name_str, &cached_modref->ldr.BaseDllName, TRUE ))
        return cached_modref;

    mark = &hash_table[hash_basename( &name_str )];
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        WINE_MODREF *mod = CONTAINING_RECORD(entry, WINE_MODREF, ldr.HashLinks);
        if (RtlEqualUnicodeString( &name_str, &mod->ldr.BaseDllName, TRUE ) && !mod->system)
        {
            cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
            return cached_modref;
        }
    }
    return NULL;
}


/**********************************************************************
 *	    find_fullname_module
 *
 * Find a module from its full path name.
 * The loader_section must be locked while calling this function
 */
static WINE_MODREF *find_fullname_module( const UNICODE_STRING *nt_name )
{
    PLIST_ENTRY mark, entry;
    UNICODE_STRING name = *nt_name;

    if (name.Length <= 4 * sizeof(WCHAR)) return NULL;
    name.Length -= 4 * sizeof(WCHAR);  /* for \??\ prefix */
    name.Buffer += 4;

    if (cached_modref && RtlEqualUnicodeString( &name, &cached_modref->ldr.FullDllName, TRUE ))
        return cached_modref;

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        LDR_DATA_TABLE_ENTRY *mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        if (RtlEqualUnicodeString( &name, &mod->FullDllName, TRUE ))
        {
            cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
            return cached_modref;
        }
    }
    return NULL;
}


/**********************************************************************
 *	    find_fileid_module
 *
 * Find a module from its file id.
 * The loader_section must be locked while calling this function
 */
static WINE_MODREF *find_fileid_module( const struct file_id *id )
{
    LIST_ENTRY *mark, *entry;

    if (cached_modref && !memcmp( &cached_modref->id, id, sizeof(*id) )) return cached_modref;

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        LDR_DATA_TABLE_ENTRY *mod = CONTAINING_RECORD( entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks );
        WINE_MODREF *wm = CONTAINING_RECORD( mod, WINE_MODREF, ldr );

        if (!memcmp( &wm->id, id, sizeof(*id) ))
        {
            cached_modref = wm;
            return wm;
        }
    }
    return NULL;
}


/******************************************************************************
 *	get_apiset_entry
 */
static NTSTATUS get_apiset_entry( const API_SET_NAMESPACE *map, const WCHAR *name, ULONG len,
                                  const API_SET_NAMESPACE_ENTRY **entry )
{
    const API_SET_HASH_ENTRY *hash_entry;
    ULONG hash, i, hash_len;
    int min, max;

    if (len <= 4) return STATUS_INVALID_PARAMETER;
    if (wcsnicmp( name, L"api-", 4 ) && wcsnicmp( name, L"ext-", 4 )) return STATUS_INVALID_PARAMETER;
    if (!map) return STATUS_APISET_NOT_PRESENT;

    for (i = hash_len = 0; i < len; i++)
    {
        if (name[i] == '.') break;
        if (name[i] == '-') hash_len = i;
    }
    for (i = hash = 0; i < hash_len; i++)
        hash = hash * map->HashFactor + ((name[i] >= 'A' && name[i] <= 'Z') ? name[i] + 32 : name[i]);

    hash_entry = (API_SET_HASH_ENTRY *)((char *)map + map->HashOffset);
    min = 0;
    max = map->Count - 1;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (hash_entry[pos].Hash < hash) min = pos + 1;
        else if (hash_entry[pos].Hash > hash) max = pos - 1;
        else
        {
            *entry = (API_SET_NAMESPACE_ENTRY *)((char *)map + map->EntryOffset) + hash_entry[pos].Index;
            if ((*entry)->HashedLength != hash_len * sizeof(WCHAR)) break;
            if (wcsnicmp( (WCHAR *)((char *)map + (*entry)->NameOffset), name, hash_len )) break;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_APISET_NOT_PRESENT;
}


/******************************************************************************
 *	get_apiset_target
 */
static NTSTATUS get_apiset_target( const API_SET_NAMESPACE *map, const API_SET_NAMESPACE_ENTRY *entry,
                                   const WCHAR *host, UNICODE_STRING *ret )
{
    const API_SET_VALUE_ENTRY *value = (API_SET_VALUE_ENTRY *)((char *)map + entry->ValueOffset);
    ULONG i, len;

    if (!entry->ValueCount) return STATUS_DLL_NOT_FOUND;
    if (host)
    {
        /* look for specific host in entries 1..n, entry 0 is the default */
        for (i = 1; i < entry->ValueCount; i++)
        {
            len = value[i].NameLength / sizeof(WCHAR);
            if (!wcsnicmp( host, (WCHAR *)((char *)map + value[i].NameOffset), len ) && !host[len])
            {
                value += i;
                break;
            }
        }
    }
    if (!value->ValueOffset) return STATUS_DLL_NOT_FOUND;
    ret->Buffer = (WCHAR *)((char *)map + value->ValueOffset);
    ret->Length = value->ValueLength;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *	    build_import_name
 */
static NTSTATUS build_import_name( WCHAR buffer[256], const char *import, int len )
{
    const API_SET_NAMESPACE *map = NtCurrentTeb()->Peb->ApiSetMap;
    const API_SET_NAMESPACE_ENTRY *entry;
    const WCHAR *host = current_modref ? current_modref->ldr.BaseDllName.Buffer : NULL;
    UNICODE_STRING str;

    while (len && import[len-1] == ' ') len--;  /* remove trailing spaces */
    if (len + sizeof(".dll") > 256) return STATUS_DLL_NOT_FOUND;
    ascii_to_unicode( buffer, import, len );
    buffer[len] = 0;
    if (!wcschr( buffer, '.' )) wcscpy( buffer + len, L".dll" );

    if (get_apiset_entry( map, buffer, wcslen(buffer), &entry )) return STATUS_SUCCESS;

    if (get_apiset_target( map, entry, host, &str )) return STATUS_DLL_NOT_FOUND;
    if (str.Length >= 256 * sizeof(WCHAR)) return STATUS_DLL_NOT_FOUND;

    TRACE( "found %s for %s\n", debugstr_us(&str), debugstr_w(buffer));
    memcpy( buffer, str.Buffer, str.Length );
    buffer[str.Length / sizeof(WCHAR)] = 0;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *	    append_dll_ext
 */
static WCHAR *append_dll_ext( const WCHAR *name )
{
    const WCHAR *ext = wcsrchr( name, '.' );

    if (!ext || wcschr( ext, '/' ) || wcschr( ext, '\\'))
    {
        WCHAR *ret = RtlAllocateHeap( GetProcessHeap(), 0,
                                      wcslen(name) * sizeof(WCHAR) + sizeof(L".dll") );
        if (!ret) return NULL;
        wcscpy( ret, name );
        wcscat( ret, L".dll" );
        return ret;
    }
    return NULL;
}


/***********************************************************************
 *           is_import_dll_system
 */
static BOOL is_import_dll_system( LDR_DATA_TABLE_ENTRY *mod, const IMAGE_IMPORT_DESCRIPTOR *import )
{
    const char *name = get_rva( mod->DllBase, import->Name );

    return !_stricmp( name, "ntdll.dll" ) || !_stricmp( name, "kernel32.dll" );
}

/**********************************************************************
 *	    insert_single_list_tail
 */
static void insert_single_list_after( LDRP_CSLIST *list, SINGLE_LIST_ENTRY *prev, SINGLE_LIST_ENTRY *entry )
{
    if (!list->Tail)
    {
        assert( !prev );
        entry->Next = entry;
        list->Tail = entry;
        return;
    }
    if (!prev)
    {
        /* Insert at head. */
        entry->Next = list->Tail->Next;
        list->Tail->Next = entry;
        return;
    }
    entry->Next = prev->Next;
    prev->Next = entry;
    if (prev == list->Tail) list->Tail = entry;
}

/**********************************************************************
 *	    remove_single_list_entry
 */
static void remove_single_list_entry( LDRP_CSLIST *list, SINGLE_LIST_ENTRY *entry )
{
    SINGLE_LIST_ENTRY *prev;

    assert( list->Tail );

    if (entry->Next == entry)
    {
        assert( list->Tail == entry );
        list->Tail = NULL;
        return;
    }

    prev = list->Tail->Next;
    while (prev->Next != entry && prev != list->Tail)
        prev = prev->Next;
    assert( prev->Next == entry );
    prev->Next = entry->Next;
    if (list->Tail == entry) list->Tail = prev;
    entry->Next = NULL;
}

/**********************************************************************
 *	    add_module_dependency_after
 */
static BOOL add_module_dependency_after( LDR_DDAG_NODE *from, LDR_DDAG_NODE *to,
                                         SINGLE_LIST_ENTRY *dep_after )
{
    LDR_DEPENDENCY *dep;

    if (!(dep = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*dep) ))) return FALSE;

    dep->dependency_from = from;
    insert_single_list_after( &from->Dependencies, dep_after, &dep->dependency_to_entry );
    dep->dependency_to = to;
    insert_single_list_after( &to->IncomingDependencies, NULL, &dep->dependency_from_entry );

    return TRUE;
}

/**********************************************************************
 *	    add_module_dependency
 */
static BOOL add_module_dependency( LDR_DDAG_NODE *from, LDR_DDAG_NODE *to )
{
    return add_module_dependency_after( from, to, from->Dependencies.Tail );
}

/**********************************************************************
 *	    remove_module_dependency
 */
static void remove_module_dependency( LDR_DEPENDENCY *dep )
{
    remove_single_list_entry( &dep->dependency_to->IncomingDependencies, &dep->dependency_from_entry );
    remove_single_list_entry( &dep->dependency_from->Dependencies, &dep->dependency_to_entry );
    RtlFreeHeap( GetProcessHeap(), 0, dep );
}

/**********************************************************************
 *	    walk_node_dependencies
 */
static NTSTATUS walk_node_dependencies( LDR_DDAG_NODE *node, void *context,
                                        NTSTATUS (*callback)( LDR_DDAG_NODE *, void * ))
{
    SINGLE_LIST_ENTRY *entry;
    LDR_DEPENDENCY *dep;
    NTSTATUS status;

    if (!(entry = node->Dependencies.Tail)) return STATUS_SUCCESS;

    do
    {
        entry = entry->Next;
        dep = CONTAINING_RECORD( entry, LDR_DEPENDENCY, dependency_to_entry );
        assert( dep->dependency_from == node );
        if ((status = callback( dep->dependency_to, context ))) break;
    } while (entry != node->Dependencies.Tail);

    return status;
}

/*************************************************************************
 *		find_forwarded_export
 *
 * Find the final function pointer for a forwarded function.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_forwarded_export( HMODULE module, const char *forward, LPCWSTR load_path )
{
    const IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    WINE_MODREF *wm;
    WCHAR mod_name[256];
    const char *end = strrchr(forward, '.');
    FARPROC proc = NULL;

    if (!end) return NULL;
    if (build_import_name( mod_name, forward, end - forward )) return NULL;

    if (!(wm = find_basename_module( mod_name )))
    {
        WINE_MODREF *imp = get_modref( module );
        TRACE( "delay loading %s for '%s'\n", debugstr_w(mod_name), forward );
        if (load_dll( load_path, mod_name, 0, &wm, imp->system ) == STATUS_SUCCESS &&
            !(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS))
        {
            if (!imports_fixup_done && current_modref)
            {
                add_module_dependency( current_modref->ldr.DdagNode, wm->ldr.DdagNode );
            }
            else if (process_attach( wm->ldr.DdagNode, NULL ) != STATUS_SUCCESS)
            {
                LdrUnloadDll( wm->ldr.DllBase );
                wm = NULL;
            }
        }

        if (!wm)
        {
            ERR( "module not found for forward '%s' used by %s\n",
                 forward, debugstr_w(imp->ldr.FullDllName.Buffer) );
            return NULL;
        }
    }
    if ((exports = RtlImageDirectoryEntryToData( wm->ldr.DllBase, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        const char *name = end + 1;

        if (*name == '#') { /* ordinal */
            proc = find_ordinal_export( wm->ldr.DllBase, exports, exp_size,
                                        atoi(name+1) - exports->Base, load_path );
        } else
            proc = find_named_export( wm->ldr.DllBase, exports, exp_size, name, -1, load_path );
    }

    if (!proc)
    {
        ERR("function not found for forward '%s' used by %s."
            " If you are using builtin %s, try using the native one instead.\n",
            forward, debugstr_w(get_modref(module)->ldr.FullDllName.Buffer),
            debugstr_w(get_modref(module)->ldr.BaseDllName.Buffer) );
    }
    return proc;
}


/*************************************************************************
 *		find_ordinal_export
 *
 * Find an exported function by ordinal.
 * The exports base must have been subtracted from the ordinal already.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    DWORD exp_size, DWORD ordinal, LPCWSTR load_path )
{
    FARPROC proc;
    const DWORD *functions = get_rva( module, exports->AddressOfFunctions );

    if (ordinal >= exports->NumberOfFunctions)
    {
        TRACE("	ordinal %ld out of range!\n", ordinal + exports->Base );
        return NULL;
    }
    if (!functions[ordinal]) return NULL;

    proc = get_rva( module, functions[ordinal] );

    /* if the address falls into the export dir, it's a forward */
    if (((const char *)proc >= (const char *)exports) && 
        ((const char *)proc < (const char *)exports + exp_size))
        return find_forwarded_export( module, (const char *)proc, load_path );

    if (TRACE_ON(snoop))
    {
        const WCHAR *user = current_modref ? current_modref->ldr.BaseDllName.Buffer : NULL;
        proc = SNOOP_GetProcAddress( module, exports, exp_size, proc, ordinal, user );
    }
    if (TRACE_ON(relay))
    {
        const WCHAR *user = current_modref ? current_modref->ldr.BaseDllName.Buffer : NULL;
        proc = RELAY_GetProcAddress( module, exports, exp_size, proc, ordinal, user );
    }
    return proc;
}


/*************************************************************************
 *		find_name_in_exports
 *
 * Helper for find_named_export.
 */
static int find_name_in_exports( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports, const char *name )
{
    const WORD *ordinals = get_rva( module, exports->AddressOfNameOrdinals );
    const DWORD *names = get_rva( module, exports->AddressOfNames );
    int min = 0, max = exports->NumberOfNames - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = get_rva( module, names[pos] );
        if (!(res = strcmp( ename, name ))) return ordinals[pos];
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    return -1;
}


/*************************************************************************
 *		find_named_export
 *
 * Find an exported function by name.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint, LPCWSTR load_path )
{
    const WORD *ordinals = get_rva( module, exports->AddressOfNameOrdinals );
    const DWORD *names = get_rva( module, exports->AddressOfNames );
    int ordinal;

    /* first check the hint */
    if (hint >= 0 && hint < exports->NumberOfNames)
    {
        char *ename = get_rva( module, names[hint] );
        if (!strcmp( ename, name ))
            return find_ordinal_export( module, exports, exp_size, ordinals[hint], load_path );
    }

    /* then do a binary search */
    if ((ordinal = find_name_in_exports( module, exports, name )) == -1) return NULL;
    return find_ordinal_export( module, exports, exp_size, ordinal, load_path );

}


/*************************************************************************
 *		RtlFindExportedRoutineByName
 */
void * WINAPI RtlFindExportedRoutineByName( HMODULE module, const char *name )
{
    const IMAGE_EXPORT_DIRECTORY *exports;
    const DWORD *functions;
    DWORD exp_size;
    int ordinal;
    void *proc;

    exports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size );
    if (!exports || exp_size < sizeof(*exports)) return NULL;

    if ((ordinal = find_name_in_exports( module, exports, name )) == -1) return NULL;
    if (ordinal >= exports->NumberOfFunctions) return NULL;
    functions = get_rva( module, exports->AddressOfFunctions );
    if (!functions[ordinal]) return NULL;
    proc = get_rva( module, functions[ordinal] );
    /* if the address falls into the export dir, it's a forward */
    if (((const char *)proc >= (const char *)exports) &&
        ((const char *)proc < (const char *)exports + exp_size))
        return NULL;
    return proc;
}


/*************************************************************************
 *		import_dll
 *
 * Import the dll specified by the given import descriptor.
 * The loader_section must be locked while calling this function.
 */
static BOOL import_dll( HMODULE module, const IMAGE_IMPORT_DESCRIPTOR *descr, LPCWSTR load_path, WINE_MODREF **pwm )
{
    BOOL system = current_modref->system || (current_modref->ldr.Flags & LDR_WINE_INTERNAL);
    NTSTATUS status;
    WINE_MODREF *wmImp;
    HMODULE imp_mod;
    const IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    const IMAGE_THUNK_DATA *import_list;
    IMAGE_THUNK_DATA *thunk_list;
    WCHAR buffer[256];
    const char *name = get_rva( module, descr->Name );
    DWORD len = strlen(name);
    PVOID protect_base;
    SIZE_T protect_size = 0;
    DWORD protect_old;

    thunk_list = get_rva( module, (DWORD)descr->FirstThunk );
    if (descr->OriginalFirstThunk)
        import_list = get_rva( module, (DWORD)descr->OriginalFirstThunk );
    else
        import_list = thunk_list;

    if (!import_list->u1.Ordinal)
    {
        WARN( "Skipping unused import %s\n", name );
        *pwm = NULL;
        return TRUE;
    }

    status = build_import_name( buffer, name, len );
    if (!status) status = load_dll( load_path, buffer, 0, &wmImp, system );

    if (status)
    {
        if (status == STATUS_DLL_NOT_FOUND)
            ERR("Library %s (which is needed by %s) not found\n",
                name, debugstr_w(current_modref->ldr.FullDllName.Buffer));
        else
            ERR("Loading library %s (which is needed by %s) failed (error %lx).\n",
                name, debugstr_w(current_modref->ldr.FullDllName.Buffer), status);
        return FALSE;
    }

    /* unprotect the import address table since it can be located in
     * readonly section */
    while (import_list[protect_size].u1.Ordinal) protect_size++;
    protect_base = thunk_list;
    protect_size *= sizeof(*thunk_list);
    NtProtectVirtualMemory( NtCurrentProcess(), &protect_base,
                            &protect_size, PAGE_READWRITE, &protect_old );

    imp_mod = wmImp->ldr.DllBase;
    exports = RtlImageDirectoryEntryToData( imp_mod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size );

    if (!exports)
    {
        /* set all imported function to deadbeef */
        while (import_list->u1.Ordinal)
        {
            if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal))
            {
                int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);
                WARN("No implementation for %s.%d", name, ordinal );
                thunk_list->u1.Function = allocate_stub( name, IntToPtr(ordinal) );
            }
            else
            {
                IMAGE_IMPORT_BY_NAME *pe_name = get_rva( module, (DWORD)import_list->u1.AddressOfData );
                WARN("No implementation for %s.%s", name, pe_name->Name );
                thunk_list->u1.Function = allocate_stub( name, (const char*)pe_name->Name );
            }
            WARN(" imported from %s, allocating stub %p\n",
                 debugstr_w(current_modref->ldr.FullDllName.Buffer),
                 (void *)thunk_list->u1.Function );
            import_list++;
            thunk_list++;
        }
        goto done;
    }

    while (import_list->u1.Ordinal)
    {
        if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal))
        {
            int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

            thunk_list->u1.Function = (ULONG_PTR)find_ordinal_export( imp_mod, exports, exp_size,
                                                                      ordinal - exports->Base, load_path );
            if (!thunk_list->u1.Function)
            {
                thunk_list->u1.Function = allocate_stub( name, IntToPtr(ordinal) );
                WARN("No implementation for %s.%d imported from %s, setting to %p\n",
                     name, ordinal, debugstr_w(current_modref->ldr.FullDllName.Buffer),
                     (void *)thunk_list->u1.Function );
            }
            TRACE_(imports)("--- Ordinal %s.%d = %p\n", name, ordinal, (void *)thunk_list->u1.Function );
        }
        else  /* import by name */
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = get_rva( module, (DWORD)import_list->u1.AddressOfData );
            thunk_list->u1.Function = (ULONG_PTR)find_named_export( imp_mod, exports, exp_size,
                                                                    (const char*)pe_name->Name,
                                                                    pe_name->Hint, load_path );
            if (!thunk_list->u1.Function)
            {
                thunk_list->u1.Function = allocate_stub( name, (const char*)pe_name->Name );
                WARN("No implementation for %s.%s imported from %s, setting to %p\n",
                     name, pe_name->Name, debugstr_w(current_modref->ldr.FullDllName.Buffer),
                     (void *)thunk_list->u1.Function );
            }
            TRACE_(imports)("--- %s %s.%d = %p\n",
                            pe_name->Name, name, pe_name->Hint, (void *)thunk_list->u1.Function);
        }
        import_list++;
        thunk_list++;
    }

done:
    /* restore old protection of the import address table */
    NtProtectVirtualMemory( NtCurrentProcess(), &protect_base, &protect_size, protect_old, &protect_old );
    *pwm = wmImp;
    return TRUE;
}


/***********************************************************************
 *           create_module_activation_context
 */
static NTSTATUS create_module_activation_context( LDR_DATA_TABLE_ENTRY *module )
{
    NTSTATUS status;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry;

    info.Type = RT_MANIFEST;
    info.Name = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
    info.Language = 0;
    if (!(status = LdrFindResource_U( module->DllBase, &info, 3, &entry )))
    {
        ACTCTXW ctx;
        ctx.cbSize   = sizeof(ctx);
        ctx.lpSource = NULL;
        ctx.dwFlags  = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        ctx.hModule  = module->DllBase;
        ctx.lpResourceName = (LPCWSTR)ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
        status = RtlCreateActivationContext( &module->ActivationContext, &ctx );
    }
    return status;
}


/*************************************************************************
 *		is_dll_native_subsystem
 *
 * Check if dll is a proper native driver.
 * Some dlls (corpol.dll from IE6 for instance) are incorrectly marked as native
 * while being perfectly normal DLLs.  This heuristic should catch such breakages.
 */
static BOOL is_dll_native_subsystem( LDR_DATA_TABLE_ENTRY *mod, const IMAGE_NT_HEADERS *nt, LPCWSTR filename )
{
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    DWORD i, size;

    if (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) return FALSE;
    if (nt->OptionalHeader.SectionAlignment < page_size) return TRUE;
    if (mod->Flags & LDR_WINE_INTERNAL) return TRUE;

    if ((imports = RtlImageDirectoryEntryToData( mod->DllBase, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for (i = 0; imports[i].Name; i++)
            if (is_import_dll_system( mod, &imports[i] ))
            {
                TRACE( "%s imports system dll, assuming not native\n", debugstr_w(filename) );
                return FALSE;
            }
    }
    return TRUE;
}

/*************************************************************************
 *		alloc_tls_slot
 *
 * Allocate a TLS slot for a newly-loaded module.
 * The loader_section must be locked while calling this function.
 */
static BOOL alloc_tls_slot( LDR_DATA_TABLE_ENTRY *mod )
{
    const IMAGE_TLS_DIRECTORY *dir;
    ULONG i, size;
    void *new_ptr;
    UINT old_module_count = tls_module_count;
    HANDLE thread = NULL, next;

    if (!(dir = RtlImageDirectoryEntryToData( mod->DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &size )))
        return FALSE;

    size = dir->EndAddressOfRawData - dir->StartAddressOfRawData;
    if (!size && !dir->SizeOfZeroFill && !dir->AddressOfCallBacks) return FALSE;

    for (i = 0; i < tls_module_count; i++)
    {
        if (!tls_dirs[i].StartAddressOfRawData && !tls_dirs[i].EndAddressOfRawData &&
            !tls_dirs[i].SizeOfZeroFill && !tls_dirs[i].AddressOfCallBacks)
            break;
    }

    TRACE( "module %p data %p-%p zerofill %lu index %p callback %p flags %lx -> slot %lu\n", mod->DllBase,
           (void *)dir->StartAddressOfRawData, (void *)dir->EndAddressOfRawData, dir->SizeOfZeroFill,
           (void *)dir->AddressOfIndex, (void *)dir->AddressOfCallBacks, dir->Characteristics, i );

    if (i == tls_module_count)
    {
        UINT new_count = tls_module_count * 2;

        new_ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, tls_dirs,
                                     new_count * sizeof(*tls_dirs) );
        if (!new_ptr) return FALSE;
        tls_dirs = new_ptr;
        tls_module_count = new_count;
    }

    /* allocate the data block in all running threads */
    while (!NtGetNextThread( GetCurrentProcess(), thread, THREAD_QUERY_LIMITED_INFORMATION, 0, 0, &next ))
    {
        THREAD_BASIC_INFORMATION tbi;
        TEB *teb;

        if (thread) NtClose( thread );
        thread = next;
        if (NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL ) || !tbi.TebBaseAddress)
        {
            ERR( "NtQueryInformationThread failed.\n" );
            continue;
        }
        teb = tbi.TebBaseAddress;
        if (!teb->ThreadLocalStoragePointer)
        {
            /* Thread is not initialized by loader yet or already teared down. */
            TRACE( "thread %04lx NULL tls block.\n", HandleToULong(tbi.ClientId.UniqueThread) );
            continue;
        }

        if (old_module_count < tls_module_count)
        {
            void **old = teb->ThreadLocalStoragePointer;
            void **new = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, tls_module_count * sizeof(*new));

            if (!new) return FALSE;
            if (old) memcpy( new, old, old_module_count * sizeof(*new) );
            teb->ThreadLocalStoragePointer = new;
#ifdef __x86_64__  /* macOS-specific hack */
            if (teb->Instrumentation[0]) ((TEB *)teb->Instrumentation[0])->ThreadLocalStoragePointer = new;
#endif
            TRACE( "thread %04lx tls block %p -> %p\n", HandleToULong(teb->ClientId.UniqueThread), old, new );
            /* FIXME: can't free old block here, should be freed at thread exit */
        }

        if (!(new_ptr = RtlAllocateHeap( GetProcessHeap(), 0, size + dir->SizeOfZeroFill ))) return -1;
        memcpy( new_ptr, (void *)dir->StartAddressOfRawData, size );
        memset( (char *)new_ptr + size, 0, dir->SizeOfZeroFill );

        TRACE( "thread %04lx slot %lu: %lu/%lu bytes at %p\n",
               HandleToULong(teb->ClientId.UniqueThread), i, size, dir->SizeOfZeroFill, new_ptr );

        RtlFreeHeap( GetProcessHeap(), 0,
                     InterlockedExchangePointer( (void **)teb->ThreadLocalStoragePointer + i, new_ptr ));
    }
    if (thread) NtClose( thread );

    *(DWORD *)dir->AddressOfIndex = i;
    tls_dirs[i] = *dir;
    return TRUE;
}


/*************************************************************************
 *		free_tls_slot
 *
 * Free the module TLS slot on unload.
 * The loader_section must be locked while calling this function.
 */
static void free_tls_slot( LDR_DATA_TABLE_ENTRY *mod )
{
    const IMAGE_TLS_DIRECTORY *dir;
    ULONG i, size;

    if (mod->TlsIndex != -1)
        return;
    if (!(dir = RtlImageDirectoryEntryToData( mod->DllBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &size )))
        return;

    i = *(ULONG*)dir->AddressOfIndex;
    assert( i < tls_module_count );
    memset( &tls_dirs[i], 0, sizeof(tls_dirs[i]) );
}


/****************************************************************
 *       fixup_imports_ilonly
 *
 * Fixup imports for an IL-only module. All we do is import mscoree.
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS fixup_imports_ilonly( WINE_MODREF *wm, LPCWSTR load_path, void **entry )
{
    NTSTATUS status;
    void *proc;
    const char *name;
    WINE_MODREF *prev, *imp;

    if (!(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS)) return STATUS_SUCCESS;  /* already done */
    wm->ldr.Flags &= ~LDR_DONT_RESOLVE_REFS;

    prev = current_modref;
    current_modref = wm;
    assert( !wm->ldr.DdagNode->Dependencies.Tail );
    if (!(status = load_dll( load_path, L"mscoree.dll", 0, &imp, FALSE ))
          && !add_module_dependency_after( wm->ldr.DdagNode, imp->ldr.DdagNode, NULL ))
        status = STATUS_NO_MEMORY;
    current_modref = prev;
    if (status)
    {
        ERR( "mscoree.dll not found, IL-only binary %s cannot be loaded\n",
             debugstr_w(wm->ldr.BaseDllName.Buffer) );
        return status;
    }

    TRACE( "loaded mscoree for %s\n", debugstr_w(wm->ldr.FullDllName.Buffer) );

    name = (wm->ldr.Flags & LDR_IMAGE_IS_DLL) ? "_CorDllMain" : "_CorExeMain";
    if (!(proc = RtlFindExportedRoutineByName( imp->ldr.DllBase, name ))) return STATUS_PROCEDURE_NOT_FOUND;
    *entry = proc;
    return STATUS_SUCCESS;
}


/****************************************************************
 *       fixup_imports
 *
 * Fixup all imports of a given module.
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS fixup_imports( WINE_MODREF *wm, LPCWSTR load_path )
{
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    SINGLE_LIST_ENTRY *dep_after;
    WINE_MODREF *prev, *imp;
    int i, nb_imports;
    DWORD size;
    NTSTATUS status;
    ULONG_PTR cookie;

    if (!(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS)) return STATUS_SUCCESS;  /* already done */
    wm->ldr.Flags &= ~LDR_DONT_RESOLVE_REFS;

    if (alloc_tls_slot( &wm->ldr )) wm->ldr.TlsIndex = -1;

    if (!(imports = RtlImageDirectoryEntryToData( wm->ldr.DllBase, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
        return STATUS_SUCCESS;

    nb_imports = 0;
    while (imports[nb_imports].Name && imports[nb_imports].FirstThunk) nb_imports++;

    if (!nb_imports) return STATUS_SUCCESS;  /* no imports */

    if (!create_module_activation_context( &wm->ldr ))
        RtlActivateActivationContext( 0, wm->ldr.ActivationContext, &cookie );

    /* load the imported modules. They are automatically
     * added to the modref list of the process.
     */
    prev = current_modref;
    current_modref = wm;
    status = STATUS_SUCCESS;
    for (i = 0; i < nb_imports; i++)
    {
        dep_after = wm->ldr.DdagNode->Dependencies.Tail;
        if (!import_dll( wm->ldr.DllBase, &imports[i], load_path, &imp ))
            status = STATUS_DLL_NOT_FOUND;
        else if (imp && imp->ldr.DdagNode != node_ntdll && imp->ldr.DdagNode != node_kernel32)
            add_module_dependency_after( wm->ldr.DdagNode, imp->ldr.DdagNode, dep_after );
    }
    current_modref = prev;
    if (wm->ldr.ActivationContext) RtlDeactivateActivationContext( 0, cookie );
    return status;
}


/*************************************************************************
 *		alloc_module
 *
 * Allocate a WINE_MODREF structure and add it to the process list
 * The loader_section must be locked while calling this function.
 */
static WINE_MODREF *alloc_module( HMODULE hModule, const UNICODE_STRING *nt_name, BOOL builtin )
{
    WCHAR *buffer;
    WINE_MODREF *wm;
    const WCHAR *p;
    const IMAGE_NT_HEADERS *nt = RtlImageNtHeader(hModule);

    if (!(wm = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wm) ))) return NULL;

    wm->ldr.DllBase       = hModule;
    wm->ldr.SizeOfImage   = nt->OptionalHeader.SizeOfImage;
    wm->ldr.Flags         = LDR_DONT_RESOLVE_REFS | (builtin ? LDR_WINE_INTERNAL : 0);
    wm->ldr.TlsIndex      = 0;
    wm->ldr.LoadCount     = 1;
    wm->CheckSum          = nt->OptionalHeader.CheckSum;
    wm->ldr.TimeDateStamp = nt->FileHeader.TimeDateStamp;

    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, nt_name->Length - 3 * sizeof(WCHAR) )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, wm );
        return NULL;
    }

    if (!(wm->ldr.DdagNode = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wm->ldr.DdagNode) )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
        RtlFreeHeap( GetProcessHeap(), 0, wm );
        return NULL;
    }
    InitializeListHead(&wm->ldr.DdagNode->Modules);
    InsertTailList(&wm->ldr.DdagNode->Modules, &wm->ldr.NodeModuleLink);

    if (nt_name->Length >= 8 * sizeof(WCHAR) && !wcsncmp(nt_name->Buffer + 4, L"UNC\\", 4))
    {
        buffer[0] = '\\';
        memcpy( buffer + 1, nt_name->Buffer + 7 /* \??\UNC prefix */, nt_name->Length - 7 * sizeof(WCHAR) );
        buffer[nt_name->Length/sizeof(WCHAR) - 6] = 0;
    }
    else
    {
        memcpy( buffer, nt_name->Buffer + 4 /* \??\ prefix */, nt_name->Length - 4 * sizeof(WCHAR) );
        buffer[nt_name->Length/sizeof(WCHAR) - 4] = 0;
    }
    if ((p = wcsrchr( buffer, '\\' ))) p++;
    else p = buffer;
    RtlInitUnicodeString( &wm->ldr.FullDllName, buffer );
    RtlInitUnicodeString( &wm->ldr.BaseDllName, p );

    if (!is_dll_native_subsystem( &wm->ldr, nt, p ))
    {
        if (nt->FileHeader.Characteristics & IMAGE_FILE_DLL)
            wm->ldr.Flags |= LDR_IMAGE_IS_DLL;
        if (nt->OptionalHeader.AddressOfEntryPoint)
            wm->ldr.EntryPoint = (char *)hModule + nt->OptionalHeader.AddressOfEntryPoint;
    }

    InsertTailList(&NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList,
                   &wm->ldr.InLoadOrderLinks);
    InsertTailList(&NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList,
                   &wm->ldr.InMemoryOrderLinks);
    InsertTailList(&hash_table[hash_basename( &wm->ldr.BaseDllName )], &wm->ldr.HashLinks);
    if (rtl_rb_tree_put( &base_address_index_tree, wm->ldr.DllBase, &wm->ldr.BaseAddressIndexNode, base_address_compare ))
        ERR( "rtl_rb_tree_put failed.\n" );
    /* wait until init is called for inserting into InInitializationOrderModuleList */

    if (!(nt->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT))
    {
        ULONG flags = MEM_EXECUTE_OPTION_ENABLE;
        WARN( "disabling no-exec because of %s\n", debugstr_w(wm->ldr.BaseDllName.Buffer) );
        NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &flags, sizeof(flags) );
    }
    return wm;
}


/*************************************************************************
 *              alloc_thread_tls
 *
 * Allocate the per-thread structure for module TLS storage.
 */
static NTSTATUS alloc_thread_tls(void)
{
    void **pointers;
    UINT i, size;

    if (!(pointers = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      tls_module_count * sizeof(*pointers) )))
        return STATUS_NO_MEMORY;

    for (i = 0; i < tls_module_count; i++)
    {
        const IMAGE_TLS_DIRECTORY *dir = &tls_dirs[i];

        if (!dir) continue;
        size = dir->EndAddressOfRawData - dir->StartAddressOfRawData;
        if (!size && !dir->SizeOfZeroFill) continue;

        if (!(pointers[i] = RtlAllocateHeap( GetProcessHeap(), 0, size + dir->SizeOfZeroFill )))
        {
            while (i) RtlFreeHeap( GetProcessHeap(), 0, pointers[--i] );
            RtlFreeHeap( GetProcessHeap(), 0, pointers );
            return STATUS_NO_MEMORY;
        }
        memcpy( pointers[i], (void *)dir->StartAddressOfRawData, size );
        memset( (char *)pointers[i] + size, 0, dir->SizeOfZeroFill );

        TRACE( "slot %u: %u/%lu bytes at %p\n", i, size, dir->SizeOfZeroFill, pointers[i] );
    }
    NtCurrentTeb()->ThreadLocalStoragePointer = pointers;
#ifdef __x86_64__  /* macOS-specific hack */
    if (NtCurrentTeb()->Instrumentation[0])
        ((TEB *)NtCurrentTeb()->Instrumentation[0])->ThreadLocalStoragePointer = pointers;
#endif
    return STATUS_SUCCESS;
}


/*************************************************************************
 *              call_tls_callbacks
 */
static void call_tls_callbacks( HMODULE module, UINT reason )
{
    const IMAGE_TLS_DIRECTORY *dir;
    const PIMAGE_TLS_CALLBACK *callback;
    ULONG dirsize;

    dir = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &dirsize );
    if (!dir || !dir->AddressOfCallBacks) return;

    for (callback = (const PIMAGE_TLS_CALLBACK *)dir->AddressOfCallBacks; *callback; callback++)
    {
        TRACE_(relay)("\1Call TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                      *callback, module, reason_names[reason] );
        __TRY
        {
            call_dll_entry_point( (DLLENTRYPROC)*callback, module, reason, NULL );
        }
        __EXCEPT_ALL
        {
            TRACE_(relay)("\1exception %08lx in TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                          GetExceptionCode(), callback, module, reason_names[reason] );
            return;
        }
        __ENDTRY
        TRACE_(relay)("\1Ret  TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                      *callback, module, reason_names[reason] );
    }
}

/*************************************************************************
 *              MODULE_InitDLL
 */
static NTSTATUS MODULE_InitDLL( WINE_MODREF *wm, UINT reason, LPVOID lpReserved )
{
    WCHAR mod_name[64];
    NTSTATUS status = STATUS_SUCCESS;
    DLLENTRYPROC entry = wm->ldr.EntryPoint;
    void *module = wm->ldr.DllBase;
    BOOL retv = FALSE;

    /* Skip calls for modules loaded with special load flags */

    if (wm->ldr.Flags & LDR_DONT_RESOLVE_REFS) return STATUS_SUCCESS;
    if (wm->ldr.TlsIndex == -1) call_tls_callbacks( wm->ldr.DllBase, reason );
    if (!entry) return STATUS_SUCCESS;

    if (TRACE_ON(relay))
    {
        size_t len = min( wm->ldr.BaseDllName.Length, sizeof(mod_name)-sizeof(WCHAR) );
        memcpy( mod_name, wm->ldr.BaseDllName.Buffer, len );
        mod_name[len / sizeof(WCHAR)] = 0;
        TRACE_(relay)("\1Call PE DLL (proc=%p,module=%p %s,reason=%s,res=%p)\n",
                      entry, module, debugstr_w(mod_name), reason_names[reason], lpReserved );
    }
    else TRACE("(%p %s,%s,%p) - CALL\n", module, debugstr_w(wm->ldr.BaseDllName.Buffer),
               reason_names[reason], lpReserved );

    __TRY
    {
        retv = call_dll_entry_point( entry, module, reason, lpReserved );
        if (!retv)
            status = STATUS_DLL_INIT_FAILED;
    }
    __EXCEPT_ALL
    {
        status = GetExceptionCode();
        TRACE_(relay)("\1exception %08lx in PE entry point (proc=%p,module=%p,reason=%s,res=%p)\n",
                      status, entry, module, reason_names[reason], lpReserved );
    }
    __ENDTRY

    /* The state of the module list may have changed due to the call
       to the dll. We cannot assume that this module has not been
       deleted.  */
    if (TRACE_ON(relay))
        TRACE_(relay)("\1Ret  PE DLL (proc=%p,module=%p %s,reason=%s,res=%p) retval=%x\n",
                      entry, module, debugstr_w(mod_name), reason_names[reason], lpReserved, retv );
    else
        TRACE("(%p,%s,%p) - RETURN %d\n", module, reason_names[reason], lpReserved, retv );

    return status;
}


/*************************************************************************
 *		process_attach
 *
 * Send the process attach notification to all DLLs the given module
 * depends on (recursively). This is somewhat complicated due to the fact that
 *
 * - we have to respect the module dependencies, i.e. modules implicitly
 *   referenced by another module have to be initialized before the module
 *   itself can be initialized
 *
 * - the initialization routine of a DLL can itself call LoadLibrary,
 *   thereby introducing a whole new set of dependencies (even involving
 *   the 'old' modules) at any time during the whole process
 *
 * (Note that this routine can be recursively entered not only directly
 *  from itself, but also via LoadLibrary from one of the called initialization
 *  routines.)
 *
 * Furthermore, we need to rearrange the main WINE_MODREF list to allow
 * the process *detach* notifications to be sent in the correct order.
 * This must not only take into account module dependencies, but also
 * 'hidden' dependencies created by modules calling LoadLibrary in their
 * attach notification routine.
 *
 * The strategy is rather simple: we move a WINE_MODREF to the head of the
 * list after the attach notification has returned.  This implies that the
 * detach notifications are called in the reverse of the sequence the attach
 * notifications *returned*.
 *
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS process_attach( LDR_DDAG_NODE *node, LPVOID lpReserved )
{
    NTSTATUS status = STATUS_SUCCESS;
    LDR_DATA_TABLE_ENTRY *mod;
    ULONG_PTR cookie;
    WINE_MODREF *wm;

    if (process_detaching) return status;

    mod = CONTAINING_RECORD( node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink );
    wm = CONTAINING_RECORD( mod, WINE_MODREF, ldr );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->ldr.Flags & LDR_LOAD_IN_PROGRESS )
         || ( wm->ldr.Flags & LDR_PROCESS_ATTACHED ) )
        return status;

    TRACE("(%s,%p) - START\n", debugstr_w(wm->ldr.BaseDllName.Buffer), lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->ldr.Flags |= LDR_LOAD_IN_PROGRESS;
    if (lpReserved) wm->ldr.LoadCount = -1;  /* pin it if imported by the main exe */
    if (wm->ldr.ActivationContext) RtlActivateActivationContext( 0, wm->ldr.ActivationContext, &cookie );

    /* Recursively attach all DLLs this one depends on */
    status = walk_node_dependencies( node, lpReserved, process_attach );

    if (!wm->ldr.InInitializationOrderLinks.Flink)
        InsertTailList(&NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList,
                &wm->ldr.InInitializationOrderLinks);

    /* Call DLL entry point */
    if (status == STATUS_SUCCESS)
    {
        WINE_MODREF *prev = current_modref;
        current_modref = wm;

        call_ldr_notifications( LDR_DLL_NOTIFICATION_REASON_LOADED, &wm->ldr );
        status = MODULE_InitDLL( wm, DLL_PROCESS_ATTACH, lpReserved );
        if (status == STATUS_SUCCESS)
        {
            wm->ldr.Flags |= LDR_PROCESS_ATTACHED;
        }
        else
        {
            MODULE_InitDLL( wm, DLL_PROCESS_DETACH, lpReserved );
            call_ldr_notifications( LDR_DLL_NOTIFICATION_REASON_UNLOADED, &wm->ldr );

            /* point to the name so LdrInitializeThunk can print it */
            last_failed_modref = wm;
            WARN("Initialization of %s failed\n", debugstr_w(wm->ldr.BaseDllName.Buffer));
        }
        current_modref = prev;
    }

    if (wm->ldr.ActivationContext) RtlDeactivateActivationContext( 0, cookie );
    /* Remove recursion flag */
    wm->ldr.Flags &= ~LDR_LOAD_IN_PROGRESS;

    TRACE("(%s,%p) - END\n", debugstr_w(wm->ldr.BaseDllName.Buffer), lpReserved );
    return status;
}


/*************************************************************************
 *		process_detach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at process_attach.
 */
static void process_detach(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_DATA_TABLE_ENTRY mod;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    do
    {
        for (entry = mark->Blink; entry != mark; entry = entry->Blink)
        {
            mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY,
                                    InInitializationOrderLinks);
            /* Check whether to detach this DLL */
            if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
                continue;
            if ( mod->LoadCount && !process_detaching )
                continue;

            /* Call detach notification */
            mod->Flags &= ~LDR_PROCESS_ATTACHED;
            MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), 
                            DLL_PROCESS_DETACH, ULongToPtr(process_detaching) );
            call_ldr_notifications( LDR_DLL_NOTIFICATION_REASON_UNLOADED, mod );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while (entry != mark);
}

/*************************************************************************
 *		thread_attach
 *
 * Send DLL thread attach notifications. These are sent in the
 * reverse sequence of process detach notification.
 * The loader_section must be locked while calling this function.
 */
static void thread_attach(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_DATA_TABLE_ENTRY mod;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY,
                                InInitializationOrderLinks);
        if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
            continue;
        if ( mod->Flags & LDR_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), DLL_THREAD_ATTACH, NULL );
    }
}

/******************************************************************
 *		LdrDisableThreadCalloutsForDll (NTDLL.@)
 *
 */
NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HMODULE hModule)
{
    WINE_MODREF *wm;
    NTSTATUS    ret = STATUS_SUCCESS;

    RtlEnterCriticalSection( &loader_section );

    wm = get_modref( hModule );
    if (!wm || wm->ldr.TlsIndex == -1)
        ret = STATUS_DLL_NOT_FOUND;
    else
        wm->ldr.Flags |= LDR_NO_DLL_CALLS;

    RtlLeaveCriticalSection( &loader_section );

    return ret;
}

/* compare base address */
static int module_address_search_compare( const void *key, const RTL_BALANCED_NODE *entry )
{
    const LDR_DATA_TABLE_ENTRY *mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, BaseAddressIndexNode);
    const char *addr = key;

    if (addr < (char *)mod->DllBase) return -1;
    if (addr >= (char *)mod->DllBase + mod->SizeOfImage) return 1;
    return 0;
}

/******************************************************************
 *              LdrFindEntryForAddress (NTDLL.@)
 *
 * The loader_section must be locked while calling this function
 */
NTSTATUS WINAPI LdrFindEntryForAddress( const void *addr, PLDR_DATA_TABLE_ENTRY *pmod )
{
    RTL_BALANCED_NODE *node;

    if (!(node = rtl_rb_tree_get( &base_address_index_tree, addr, module_address_search_compare )))
        return STATUS_NO_MORE_ENTRIES;
    *pmod = CONTAINING_RECORD(node, LDR_DATA_TABLE_ENTRY, BaseAddressIndexNode);
    return STATUS_SUCCESS;
}

/******************************************************************
 *              LdrEnumerateLoadedModules (NTDLL.@)
 */
NTSTATUS WINAPI LdrEnumerateLoadedModules( void *unknown, LDRENUMPROC callback, void *context )
{
    LIST_ENTRY *mark, *entry;
    LDR_DATA_TABLE_ENTRY *mod;
    BOOLEAN stop = FALSE;

    TRACE( "(%p, %p, %p)\n", unknown, callback, context );

    if (unknown || !callback)
        return STATUS_INVALID_PARAMETER;

    RtlEnterCriticalSection( &loader_section );

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD( entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
        callback( mod, context, &stop );
        if (stop) break;
    }

    RtlLeaveCriticalSection( &loader_section );
    return STATUS_SUCCESS;
}

/******************************************************************
 *              LdrRegisterDllNotification (NTDLL.@)
 */
NTSTATUS WINAPI LdrRegisterDllNotification(ULONG flags, PLDR_DLL_NOTIFICATION_FUNCTION callback,
                                           void *context, void **cookie)
{
    struct ldr_notification *notify;

    TRACE( "(%lx, %p, %p, %p)\n", flags, callback, context, cookie );

    if (!callback || !cookie)
        return STATUS_INVALID_PARAMETER;

    if (flags)
        FIXME( "ignoring flags %lx\n", flags );

    notify = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*notify) );
    if (!notify) return STATUS_NO_MEMORY;
    notify->callback = callback;
    notify->context = context;

    RtlEnterCriticalSection( &loader_section );
    list_add_tail( &ldr_notifications, &notify->entry );
    RtlLeaveCriticalSection( &loader_section );

    *cookie = notify;
    return STATUS_SUCCESS;
}

/******************************************************************
 *              LdrUnregisterDllNotification (NTDLL.@)
 */
NTSTATUS WINAPI LdrUnregisterDllNotification( void *cookie )
{
    struct ldr_notification *notify = cookie;

    TRACE( "(%p)\n", cookie );

    if (!notify) return STATUS_INVALID_PARAMETER;

    RtlEnterCriticalSection( &loader_section );
    list_remove( &notify->entry );
    RtlLeaveCriticalSection( &loader_section );

    RtlFreeHeap( GetProcessHeap(), 0, notify );
    return STATUS_SUCCESS;
}

/******************************************************************
 *		LdrLockLoaderLock  (NTDLL.@)
 *
 * Note: some flags are not implemented.
 * Flag 0x01 is used to raise exceptions on errors.
 */
NTSTATUS WINAPI LdrLockLoaderLock( ULONG flags, ULONG *result, ULONG_PTR *magic )
{
    if (flags & ~0x2) FIXME( "flags %lx not supported\n", flags );

    if (result) *result = 0;
    if (magic) *magic = 0;
    if (flags & ~0x3) return STATUS_INVALID_PARAMETER_1;
    if (!result && (flags & 0x2)) return STATUS_INVALID_PARAMETER_2;
    if (!magic) return STATUS_INVALID_PARAMETER_3;

    if (flags & 0x2)
    {
        if (!RtlTryEnterCriticalSection( &loader_section ))
        {
            *result = 2;
            return STATUS_SUCCESS;
        }
        *result = 1;
    }
    else
    {
        RtlEnterCriticalSection( &loader_section );
        if (result) *result = 1;
    }
    *magic = GetCurrentThreadId();
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrUnlockLoaderUnlock  (NTDLL.@)
 */
NTSTATUS WINAPI LdrUnlockLoaderLock( ULONG flags, ULONG_PTR magic )
{
    if (magic)
    {
        if (magic != GetCurrentThreadId()) return STATUS_INVALID_PARAMETER_2;
        RtlLeaveCriticalSection( &loader_section );
    }
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrGetProcedureAddress  (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE module, const ANSI_STRING *name,
                                       ULONG ord, PVOID *address)
{
    IMAGE_EXPORT_DIRECTORY *exports;
    WINE_MODREF *wm;
    DWORD exp_size;
    NTSTATUS ret = STATUS_PROCEDURE_NOT_FOUND;

    RtlEnterCriticalSection( &loader_section );

    /* check if the module itself is invalid to return the proper error */
    if (!(wm = get_modref( module ))) ret = STATUS_DLL_NOT_FOUND;
    else if ((exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                      IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        void *proc = name ? find_named_export( module, exports, exp_size, name->Buffer, -1, NULL )
                          : find_ordinal_export( module, exports, exp_size, ord - exports->Base, NULL );
        if (proc)
        {
            *address = proc;
            ret = STATUS_SUCCESS;
        }
        else
        {
            WARN( "%s (ordinal %lu) not found in %s\n", debugstr_a(name ? name->Buffer : NULL),
                  ord, debugstr_us(&wm->ldr.FullDllName) );
        }
    }

    RtlLeaveCriticalSection( &loader_section );
    return ret;
}


/***********************************************************************
 *           set_security_cookie
 *
 * Create a random security cookie for buffer overflow protection. Make
 * sure it does not accidentally match the default cookie value.
 */
static void set_security_cookie( ULONG_PTR *cookie )
{
    static ULONG seed;

    TRACE( "initializing security cookie %p\n", cookie );

    if (!seed) seed = NtGetTickCount() ^ GetCurrentProcessId();
    for (;;)
    {
        if (*cookie == DEFAULT_SECURITY_COOKIE_16)
            *cookie = RtlRandom( &seed ) >> 16; /* leave the high word clear */
        else if (*cookie == DEFAULT_SECURITY_COOKIE_32)
            *cookie = RtlRandom( &seed );
#ifdef DEFAULT_SECURITY_COOKIE_64
        else if (*cookie == DEFAULT_SECURITY_COOKIE_64)
        {
            *cookie = RtlRandom( &seed );
            /* fill up, but keep the highest word clear */
            *cookie ^= (ULONG_PTR)RtlRandom( &seed ) << 16;
        }
#endif
        else
            break;
    }
}


/***********************************************************************
 *           update_load_config
 */
static void update_load_config( void *module )
{
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module );
    IMAGE_LOAD_CONFIG_DIRECTORY *cfg;
    ULONG size;

    cfg = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size );
    if (!cfg) return;
    size = min( size, cfg->Size );
    if (size > offsetof( IMAGE_LOAD_CONFIG_DIRECTORY, SecurityCookie ) &&
        cfg->SecurityCookie > (ULONG_PTR)module &&
        cfg->SecurityCookie < (ULONG_PTR)module + nt->OptionalHeader.SizeOfImage)
    {
        set_security_cookie( (ULONG_PTR *)cfg->SecurityCookie );
    }
#ifdef __arm64ec__
    if (size > offsetof( IMAGE_LOAD_CONFIG_DIRECTORY, CHPEMetadataPointer ) &&
        cfg->CHPEMetadataPointer > (ULONG_PTR)module &&
        cfg->CHPEMetadataPointer < (ULONG_PTR)module + nt->OptionalHeader.SizeOfImage)
    {
        arm64ec_update_hybrid_metadata( module, nt, (void *)cfg->CHPEMetadataPointer );
    }
#endif
}


static NTSTATUS perform_relocations( void *module, IMAGE_NT_HEADERS *nt, SIZE_T len )
{
    char *base;
    IMAGE_BASE_RELOCATION *rel, *end;
    const IMAGE_DATA_DIRECTORY *relocs;
    const IMAGE_SECTION_HEADER *sec;
    INT_PTR delta;
    ULONG protect_old[96], i;

    base = (char *)nt->OptionalHeader.ImageBase;
    if (module == base) return STATUS_SUCCESS;  /* nothing to do */

    /* no relocations are performed on non page-aligned binaries */
    if (nt->OptionalHeader.SectionAlignment < page_size)
        return STATUS_SUCCESS;

    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL) &&
        module != NtCurrentTeb()->Peb->ImageBaseAddress)
        return STATUS_SUCCESS;

    relocs = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (nt->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        WARN( "Need to relocate module from %p to %p, but there are no relocation records\n",
              base, module );
        return STATUS_CONFLICTING_ADDRESSES;
    }

    if (!relocs->Size) return STATUS_SUCCESS;
    if (!relocs->VirtualAddress) return STATUS_CONFLICTING_ADDRESSES;

    if (nt->FileHeader.NumberOfSections > ARRAY_SIZE( protect_old ))
        return STATUS_INVALID_IMAGE_FORMAT;

    sec = IMAGE_FIRST_SECTION( nt );
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr,
                                &size, PAGE_READWRITE, &protect_old[i] );
    }

    TRACE( "relocating from %p-%p to %p-%p\n",
           base, base + len, module, (char *)module + len );

    rel = get_rva( module, relocs->VirtualAddress );
    end = get_rva( module, relocs->VirtualAddress + relocs->Size );
    delta = (char *)module - base;

    while (rel < end - 1 && rel->SizeOfBlock)
    {
        if (rel->VirtualAddress >= len)
        {
            WARN( "invalid address %p in relocation %p\n", get_rva( module, rel->VirtualAddress ), rel );
            return STATUS_ACCESS_VIOLATION;
        }
        rel = LdrProcessRelocationBlock( get_rva( module, rel->VirtualAddress ),
                                         (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT),
                                         (USHORT *)(rel + 1), delta );
        if (!rel) return STATUS_INVALID_IMAGE_FORMAT;
    }

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr,
                                &size, protect_old[i], &protect_old[i] );
    }

    return STATUS_SUCCESS;
}


/*************************************************************************
 *		build_module
 *
 * Build the module data for a mapped dll.
 */
static NTSTATUS build_module( LPCWSTR load_path, const UNICODE_STRING *nt_name, void **module,
                              const SECTION_IMAGE_INFORMATION *image_info, const struct file_id *id,
                              DWORD flags, BOOL system, WINE_MODREF **pwm )
{
    static const char builtin_signature[] = "Wine builtin DLL";
    char *signature = (char *)((IMAGE_DOS_HEADER *)*module + 1);
    BOOL is_builtin;
    IMAGE_NT_HEADERS *nt;
    WINE_MODREF *wm;
    NTSTATUS status;
    SIZE_T map_size;

    if (!(nt = RtlImageNtHeader( *module ))) return STATUS_INVALID_IMAGE_FORMAT;

    map_size = (nt->OptionalHeader.SizeOfImage + page_size - 1) & ~(page_size - 1);
    if ((status = perform_relocations( *module, nt, map_size ))) return status;

    is_builtin = ((char *)nt - signature >= sizeof(builtin_signature) &&
                  !memcmp( signature, builtin_signature, sizeof(builtin_signature) ));

    /* create the MODREF */

    if (!(wm = alloc_module( *module, nt_name, is_builtin ))) return STATUS_NO_MEMORY;

    if (id) wm->id = *id;
    if (image_info->LoaderFlags) wm->ldr.Flags |= LDR_COR_IMAGE;
    if (image_info->ComPlusILOnly) wm->ldr.Flags |= LDR_COR_ILONLY;
    wm->system = system;

    update_load_config( *module );

    /* fixup imports */

    if (!(flags & DONT_RESOLVE_DLL_REFERENCES) &&
        ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) ||
         nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE))
    {
        if (wm->ldr.Flags & LDR_COR_ILONLY)
            status = fixup_imports_ilonly( wm, load_path, &wm->ldr.EntryPoint );
        else
            status = fixup_imports( wm, load_path );
        if (status != STATUS_SUCCESS)
        {
            /* the module has only be inserted in the load & memory order lists */
            RemoveEntryList(&wm->ldr.InLoadOrderLinks);
            RemoveEntryList(&wm->ldr.InMemoryOrderLinks);
            RemoveEntryList(&wm->ldr.HashLinks);
            RtlRbRemoveNode( &base_address_index_tree, &wm->ldr.BaseAddressIndexNode );

            /* FIXME: there are several more dangling references
             * left. Including dlls loaded by this dll before the
             * failed one. Unrolling is rather difficult with the
             * current structure and we can leave them lying
             * around with no problems, so we don't care.
             * As these might reference our wm, we don't free it.
             */
            *module = NULL;
            return status;
        }
    }

    TRACE( "loaded %s %p %p\n", debugstr_us(nt_name), wm, *module );

    if (is_builtin)
    {
        if (TRACE_ON(relay)) RELAY_SetupDLL( *module );
    }
    else
    {
        if ((wm->ldr.Flags & LDR_IMAGE_IS_DLL) && TRACE_ON(snoop)) SNOOP_SetupDLL( *module );
    }

    TRACE_(loaddll)( "Loaded %s at %p: %s\n", debugstr_w(wm->ldr.FullDllName.Buffer), *module,
                     is_builtin ? "builtin" : "native" );

    wm->ldr.LoadCount = 1;
    *pwm = wm;
    *module = NULL;
    return STATUS_SUCCESS;
}


/*************************************************************************
 *		build_ntdll_module
 *
 * Build the module data for the initially-loaded ntdll.
 */
static void build_ntdll_module(void)
{
    UNICODE_STRING nt_name = RTL_CONSTANT_STRING( L"\\??\\C:\\windows\\system32\\ntdll.dll" );
    MEMORY_BASIC_INFORMATION meminfo;
    WINE_MODREF *wm;
    void *module;

    NtQueryVirtualMemory( GetCurrentProcess(), LdrInitializeThunk, MemoryBasicInformation,
                          &meminfo, sizeof(meminfo), NULL );
    module = meminfo.AllocationBase;
    wm = alloc_module( module, &nt_name, TRUE );
    assert( wm );
    wm->ldr.Flags &= ~LDR_DONT_RESOLVE_REFS;
    node_ntdll = wm->ldr.DdagNode;
    if (TRACE_ON(relay)) RELAY_SetupDLL( module );
}


#ifdef _WIN64
/* convert PE header to 64-bit when loading a 32-bit IL-only module into a 64-bit process */
static BOOL convert_to_pe64( HMODULE module, const SECTION_IMAGE_INFORMATION *info )
{
    static const ULONG copy_dirs[] = { IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                       IMAGE_DIRECTORY_ENTRY_SECURITY,
                                       IMAGE_DIRECTORY_ENTRY_BASERELOC,
                                       IMAGE_DIRECTORY_ENTRY_DEBUG,
                                       IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR };
    IMAGE_OPTIONAL_HEADER32 hdr32 = { IMAGE_NT_OPTIONAL_HDR32_MAGIC };
    IMAGE_OPTIONAL_HEADER64 hdr64 = { IMAGE_NT_OPTIONAL_HDR64_MAGIC };
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module );
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION( nt );
    SIZE_T hdr_size = min( sizeof(hdr32), nt->FileHeader.SizeOfOptionalHeader );
    SIZE_T size = min( nt->OptionalHeader.SizeOfHeaders, nt->OptionalHeader.SizeOfImage );
    void *addr = module;
    ULONG i, old_prot;

    if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return TRUE;  /* already 64-bit */
    if (NtCurrentTeb()->WowTebOffset) return TRUE;  /* no need to convert */
    if (!info->ImageContainsCode) return TRUE;  /* no need to convert */

    TRACE( "%p\n", module );

    if (NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size, PAGE_READWRITE, &old_prot ))
        return FALSE;

    if ((char *)module + size < (char *)(nt + 1) + nt->FileHeader.NumberOfSections * sizeof(*sec))
    {
        NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size, old_prot, &old_prot );
        return FALSE;
    }

    memcpy( &hdr32, &nt->OptionalHeader, hdr_size );
    memcpy( &hdr64, &hdr32, offsetof( IMAGE_OPTIONAL_HEADER64, SizeOfStackReserve ));
    hdr64.Magic               = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    hdr64.AddressOfEntryPoint = 0;
    hdr64.ImageBase           = hdr32.ImageBase;
    hdr64.SizeOfStackReserve  = hdr32.SizeOfStackReserve;
    hdr64.SizeOfStackCommit   = hdr32.SizeOfStackCommit;
    hdr64.SizeOfHeapReserve   = hdr32.SizeOfHeapReserve;
    hdr64.SizeOfHeapCommit    = hdr32.SizeOfHeapCommit;
    hdr64.LoaderFlags         = hdr32.LoaderFlags;
    hdr64.NumberOfRvaAndSizes = hdr32.NumberOfRvaAndSizes;
    for (i = 0; i < ARRAY_SIZE( copy_dirs ); i++)
        hdr64.DataDirectory[copy_dirs[i]] = hdr32.DataDirectory[copy_dirs[i]];

    memmove( nt + 1, sec, nt->FileHeader.NumberOfSections * sizeof(*sec) );
    nt->FileHeader.SizeOfOptionalHeader = sizeof(hdr64);
    nt->OptionalHeader = hdr64;
    NtProtectVirtualMemory( NtCurrentProcess(), &addr, &size, old_prot, &old_prot );
    return TRUE;
}

/* read data out of a PE image directory */
static ULONG read_image_directory( HANDLE file, const SECTION_IMAGE_INFORMATION *info,
                                   ULONG dir, void *buffer, ULONG maxlen, USHORT *magic )
{
    IMAGE_DOS_HEADER mz;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER offset;
    IMAGE_SECTION_HEADER sec[96];
    unsigned int i, count;
    DWORD va, size;
    union
    {
        IMAGE_NT_HEADERS32 nt32;
        IMAGE_NT_HEADERS64 nt64;
    } nt;

    offset.QuadPart = 0;
    if (NtReadFile( file, 0, NULL, NULL, &io, &mz, sizeof(mz), &offset, NULL )) return 0;
    if (io.Information != sizeof(mz)) return 0;
    if (mz.e_magic != IMAGE_DOS_SIGNATURE) return 0;
    offset.QuadPart = mz.e_lfanew;
    if (NtReadFile( file, 0, NULL, NULL, &io, &nt, sizeof(nt), &offset, NULL )) return 0;
    if (io.Information != sizeof(nt)) return 0;
    if (nt.nt32.Signature != IMAGE_NT_SIGNATURE) return 0;
    *magic = nt.nt32.OptionalHeader.Magic;
    switch (nt.nt32.OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        va = nt.nt32.OptionalHeader.DataDirectory[dir].VirtualAddress;
        size = nt.nt32.OptionalHeader.DataDirectory[dir].Size;
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        va = nt.nt64.OptionalHeader.DataDirectory[dir].VirtualAddress;
        size = nt.nt64.OptionalHeader.DataDirectory[dir].Size;
        break;
    default:
        return 0;
    }
    if (!va) return 0;
    offset.QuadPart += offsetof( IMAGE_NT_HEADERS32, OptionalHeader ) + nt.nt32.FileHeader.SizeOfOptionalHeader;
    count = min( 96, nt.nt32.FileHeader.NumberOfSections );
    if (NtReadFile( file, 0, NULL, NULL, &io, &sec, count * sizeof(*sec), &offset, NULL )) return 0;
    if (io.Information != count * sizeof(*sec)) return 0;
    for (i = 0; i < count; i++)
    {
        if (va < sec[i].VirtualAddress) continue;
        if (sec[i].Misc.VirtualSize && va - sec[i].VirtualAddress >= sec[i].Misc.VirtualSize) continue;
        offset.QuadPart = sec[i].PointerToRawData + va - sec[i].VirtualAddress;
        if (NtReadFile( file, 0, NULL, NULL, &io, buffer, min( maxlen, size ), &offset, NULL )) return 0;
        return io.Information;
    }
    return 0;
}

/* check COM header for ILONLY flag, ignoring runtime version */
static BOOL is_com_ilonly( HANDLE file, const SECTION_IMAGE_INFORMATION *info )
{
    USHORT magic;
    IMAGE_COR20_HEADER cor_header;
    ULONG len = read_image_directory( file, info, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                      &cor_header, sizeof(cor_header), &magic );

    if (len != sizeof(cor_header)) return FALSE;
    if (magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return FALSE;
    return !!(cor_header.Flags & COMIMAGE_FLAGS_ILONLY);
}

/* check LOAD_CONFIG header for CHPE metadata */
static BOOL has_chpe_metadata( HANDLE file, const SECTION_IMAGE_INFORMATION *info )
{
    USHORT magic;
    IMAGE_LOAD_CONFIG_DIRECTORY64 loadcfg;
    ULONG len = read_image_directory( file, info, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                      &loadcfg, sizeof(loadcfg), &magic );

    if (!len) return FALSE;
    if (magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return FALSE;
    len = min( len, loadcfg.Size );
    if (len <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, CHPEMetadataPointer )) return FALSE;
    return !!loadcfg.CHPEMetadataPointer;
}

/* On WoW64 setups, an image mapping can also be created for the other 32/64 CPU */
/* but it cannot necessarily be loaded as a dll, so we need some additional checks */
static BOOL is_valid_binary( HANDLE file, const SECTION_IMAGE_INFORMATION *info )
{
    if (info->Machine == current_machine) return TRUE;
    if (NtCurrentTeb()->WowTebOffset) return TRUE;
    /* support ARM64EC binaries on x86-64 */
    if (current_machine == IMAGE_FILE_MACHINE_AMD64 && has_chpe_metadata( file, info )) return TRUE;
    /* support 32-bit IL-only images on 64-bit */
    if (!info->ImageContainsCode) return TRUE;
    if (info->ComPlusNativeReady) return TRUE;
    return is_com_ilonly( file, info );
}

#else  /* _WIN64 */

static BOOL is_valid_binary( HANDLE file, const SECTION_IMAGE_INFORMATION *info )
{
    return (info->Machine == current_machine);
}

#endif  /* _WIN64 */


/******************************************************************
 *		get_module_path_end
 *
 * Returns the end of the directory component of the module path.
 */
static inline const WCHAR *get_module_path_end( const WCHAR *module )
{
    const WCHAR *p;
    const WCHAR *mod_end = module;

    if ((p = wcsrchr( mod_end, '\\' ))) mod_end = p;
    if ((p = wcsrchr( mod_end, '/' ))) mod_end = p;
    if (mod_end == module + 2 && module[1] == ':') mod_end++;
    if (mod_end == module && module[0] && module[1] == ':') mod_end += 2;
    return mod_end;
}


/******************************************************************
 *		append_path
 *
 * Append a counted string to the load path. Helper for get_dll_load_path.
 */
static inline WCHAR *append_path( WCHAR *p, const WCHAR *str, int len )
{
    if (len == -1) len = wcslen(str);
    if (!len) return p;
    memcpy( p, str, len * sizeof(WCHAR) );
    p[len] = ';';
    return p + len + 1;
}


/******************************************************************
 *           get_dll_load_path
 */
static NTSTATUS get_dll_load_path( LPCWSTR module, LPCWSTR dll_dir, ULONG safe_mode, WCHAR **path )
{
    const WCHAR *mod_end = module;
    UNICODE_STRING name = RTL_CONSTANT_STRING( L"PATH" ), value;
    WCHAR *p, *ret;
    int len = ARRAY_SIZE(system_path) + 1, path_len = 0;

    if (module)
    {
        mod_end = get_module_path_end( module );
        len += (mod_end - module) + 1;
    }

    value.Length = 0;
    value.MaximumLength = 0;
    value.Buffer = NULL;
    if (RtlQueryEnvironmentVariable_U( NULL, &name, &value ) == STATUS_BUFFER_TOO_SMALL)
        path_len = value.Length;

    if (dll_dir) len += wcslen( dll_dir ) + 1;
    else len += 2;  /* current directory */
    if (!(p = ret = RtlAllocateHeap( GetProcessHeap(), 0, path_len + len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    p = append_path( p, module, mod_end - module );
    if (dll_dir) p = append_path( p, dll_dir, -1 );
    else if (!safe_mode) p = append_path( p, L".", -1 );
    p = append_path( p, system_path, -1 );
    if (!dll_dir && safe_mode) p = append_path( p, L".", -1 );

    value.Buffer = p;
    value.MaximumLength = path_len;

    while (RtlQueryEnvironmentVariable_U( NULL, &name, &value ) == STATUS_BUFFER_TOO_SMALL)
    {
        WCHAR *new_ptr;

        /* grow the buffer and retry */
        path_len = value.Length;
        if (!(new_ptr = RtlReAllocateHeap( GetProcessHeap(), 0, ret, path_len + len * sizeof(WCHAR) )))
        {
            RtlFreeHeap( GetProcessHeap(), 0, ret );
            return STATUS_NO_MEMORY;
        }
        value.Buffer = new_ptr + (value.Buffer - ret);
        value.MaximumLength = path_len;
        ret = new_ptr;
    }
    value.Buffer[value.Length / sizeof(WCHAR)] = 0;
    *path = ret;
    return STATUS_SUCCESS;
}


/******************************************************************
 *		get_dll_load_path_search_flags
 */
static NTSTATUS get_dll_load_path_search_flags( LPCWSTR module, DWORD flags, WCHAR **path )
{
    const WCHAR *image = NULL, *mod_end, *image_end;
    struct dll_dir_entry *dir;
    WCHAR *p, *ret;
    int len = 1;

    if (flags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
        flags |= (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                  LOAD_LIBRARY_SEARCH_USER_DIRS |
                  LOAD_LIBRARY_SEARCH_SYSTEM32);

    if (flags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)
    {
        DWORD type = RtlDetermineDosPathNameType_U( module );
        if (type != ABSOLUTE_DRIVE_PATH && type != ABSOLUTE_PATH && type != DEVICE_PATH && type != UNC_PATH)
            return STATUS_INVALID_PARAMETER;
        mod_end = get_module_path_end( module );
        len += (mod_end - module) + 1;
    }
    else module = NULL;

    if (flags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
    {
        image = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
        image_end = get_module_path_end( image );
        len += (image_end - image) + 1;
    }

    if (flags & LOAD_LIBRARY_SEARCH_USER_DIRS)
    {
        LIST_FOR_EACH_ENTRY( dir, &dll_dir_list, struct dll_dir_entry, entry )
            len += wcslen( dir->dir + 4 /* \??\ */ ) + 1;
        if (dll_directory.Length) len += dll_directory.Length / sizeof(WCHAR) + 1;
    }

    if (flags & LOAD_LIBRARY_SEARCH_SYSTEM32) len += wcslen( system_dir );

    if ((p = ret = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        if (module) p = append_path( p, module, mod_end - module );
        if (image) p = append_path( p, image, image_end - image );
        if (flags & LOAD_LIBRARY_SEARCH_USER_DIRS)
        {
            LIST_FOR_EACH_ENTRY( dir, &dll_dir_list, struct dll_dir_entry, entry )
                p = append_path( p, dir->dir + 4 /* \??\ */, -1 );
            p = append_path( p, dll_directory.Buffer, dll_directory.Length / sizeof(WCHAR) );
        }
        if (flags & LOAD_LIBRARY_SEARCH_SYSTEM32) wcscpy( p, system_dir );
        else
        {
            if (p > ret) p--;
            *p = 0;
        }
    }
    *path = ret;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *	open_dll_file
 *
 * Open a file for a new dll. Helper for find_dll_file.
 */
static NTSTATUS open_dll_file( UNICODE_STRING *nt_name, WINE_MODREF **pwm, HANDLE *mapping,
                               SECTION_IMAGE_INFORMATION *image_info, struct file_id *id )
{
    FILE_BASIC_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER size;
    FILE_OBJECTID_BUFFER fid;
    NTSTATUS status;
    HANDLE handle;

    if ((*pwm = find_fullname_module( nt_name ))) return STATUS_SUCCESS;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    if ((status = NtOpenFile( &handle, GENERIC_READ | SYNCHRONIZE, &attr, &io,
                              FILE_SHARE_READ | FILE_SHARE_DELETE,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE )))
    {
        if (status != STATUS_OBJECT_PATH_NOT_FOUND &&
            status != STATUS_OBJECT_NAME_NOT_FOUND &&
            !NtQueryAttributesFile( &attr, &info ))
        {
            /* if the file exists but failed to open, report the error */
            return status;
        }
        /* otherwise continue searching */
        return STATUS_DLL_NOT_FOUND;
    }

    if (!NtFsControlFile( handle, 0, NULL, NULL, &io, FSCTL_GET_OBJECT_ID, NULL, 0, &fid, sizeof(fid) ))
    {
        memcpy( id, fid.ObjectId, sizeof(*id) );
        if ((*pwm = find_fileid_module( id )))
        {
            TRACE( "%s is the same file as existing module %p %s\n", debugstr_w( nt_name->Buffer ),
                   (*pwm)->ldr.DllBase, debugstr_w( (*pwm)->ldr.FullDllName.Buffer ));
            NtClose( handle );
            return STATUS_SUCCESS;
        }
    }

    size.QuadPart = 0;
    status = NtCreateSection( mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                              SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                              NULL, &size, PAGE_EXECUTE_READ, SEC_IMAGE, handle );
    if (!status)
    {
        NtQuerySection( *mapping, SectionImageInformation, image_info, sizeof(*image_info), NULL );
        if (!is_valid_binary( handle, image_info ))
        {
            TRACE( "%s is for arch %x, continuing search\n", debugstr_us(nt_name), image_info->Machine );
            status = STATUS_NOT_SUPPORTED;
            NtClose( *mapping );
            *mapping = NULL;
        }
    }
    NtClose( handle );
    return status;
}


/******************************************************************************
 *	find_existing_module
 *
 * Find an existing module that is the same mapping as the new module.
 */
static WINE_MODREF *find_existing_module( HMODULE module )
{
    WINE_MODREF *wm;
    LIST_ENTRY *mark, *entry;
    LDR_DATA_TABLE_ENTRY *mod;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( module );

    if ((wm = get_modref( module ))) return wm;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD( entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
        if (mod->TimeDateStamp != nt->FileHeader.TimeDateStamp) continue;
        wm = CONTAINING_RECORD( mod, WINE_MODREF, ldr );
        if (wm->CheckSum != nt->OptionalHeader.CheckSum) continue;
        if (NtAreMappedFilesTheSame( mod->DllBase, module ) != STATUS_SUCCESS) continue;
        return CONTAINING_RECORD( mod, WINE_MODREF, ldr );
    }
    return NULL;
}


/******************************************************************************
 *	load_native_dll  (internal)
 */
static NTSTATUS load_native_dll( LPCWSTR load_path, const UNICODE_STRING *nt_name, HANDLE mapping,
                                 const SECTION_IMAGE_INFORMATION *image_info, const struct file_id *id,
                                 DWORD flags, BOOL system, WINE_MODREF** pwm )
{
    void *module = NULL;
    SIZE_T len = 0;
    NTSTATUS status = NtMapViewOfSection( mapping, NtCurrentProcess(), &module, 0, 0, NULL, &len,
                                          ViewShare, 0, PAGE_EXECUTE_READ );

    if (!NT_SUCCESS(status)) return status;

    if ((*pwm = find_existing_module( module )))  /* already loaded */
    {
        if ((*pwm)->ldr.LoadCount != -1) (*pwm)->ldr.LoadCount++;
        TRACE( "found %s for %s at %p, count=%d\n",
               debugstr_us(&(*pwm)->ldr.FullDllName), debugstr_us(nt_name),
               (*pwm)->ldr.DllBase, (*pwm)->ldr.LoadCount);
        if (module != (*pwm)->ldr.DllBase) NtUnmapViewOfSection( NtCurrentProcess(), module );
        return STATUS_SUCCESS;
    }
#ifdef _WIN64
    if (status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH && !convert_to_pe64( module, image_info ))
        status = STATUS_INVALID_IMAGE_FORMAT;
#endif
    if (NT_SUCCESS(status)) status = build_module( load_path, nt_name, &module, image_info, id, flags, system, pwm );
    if (status && module) NtUnmapViewOfSection( NtCurrentProcess(), module );
    return status;
}


/***********************************************************************
 *           load_so_dll
 */
static NTSTATUS load_so_dll( LPCWSTR load_path, const UNICODE_STRING *nt_name,
                             DWORD flags, WINE_MODREF **pwm )
{
    void *module;
    NTSTATUS status;
    WINE_MODREF *wm;
    struct load_so_dll_params params = { *nt_name, &module };

    TRACE( "trying %s as so lib\n", debugstr_us(nt_name) );
    if ((status = WINE_UNIX_CALL( unix_load_so_dll, &params )))
    {
        WARN( "failed to load .so lib %s\n", debugstr_us(nt_name) );
        if (status == STATUS_INVALID_IMAGE_FORMAT) status = STATUS_INVALID_IMAGE_NOT_MZ;
        return status;
    }

    if ((wm = get_modref( module )))  /* already loaded */
    {
        TRACE( "Found %s at %p for builtin %s\n",
               debugstr_w(wm->ldr.FullDllName.Buffer), wm->ldr.DllBase, debugstr_us(nt_name) );
        if (wm->ldr.LoadCount != -1) wm->ldr.LoadCount++;
    }
    else
    {
        SECTION_IMAGE_INFORMATION image_info = { 0 };

        if ((status = build_module( load_path, &params.nt_name, &module, &image_info, NULL, flags, FALSE, &wm )))
        {
            if (module) NtUnmapViewOfSection( NtCurrentProcess(), module );
            return status;
        }
        TRACE_(loaddll)( "Loaded %s at %p: builtin\n", debugstr_us(nt_name), module );
    }
    *pwm = wm;
    return STATUS_SUCCESS;
}


/*************************************************************************
 *		build_main_module
 *
 * Build the module data for the main image.
 */
static WINE_MODREF *build_main_module(void)
{
    SECTION_IMAGE_INFORMATION info;
    UNICODE_STRING nt_name;
    WINE_MODREF *wm;
    NTSTATUS status;
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
    void *module = NtCurrentTeb()->Peb->ImageBaseAddress;

    NtQueryInformationProcess( GetCurrentProcess(), ProcessImageInformation, &info, sizeof(info), NULL );
    if (info.ImageCharacteristics & IMAGE_FILE_DLL)
    {
        MESSAGE( "wine: %s is a dll, not an executable\n", debugstr_us(&params->ImagePathName) );
        NtTerminateProcess( GetCurrentProcess(), STATUS_INVALID_IMAGE_FORMAT );
    }
#ifdef _WIN64
    if (!convert_to_pe64( module, &info ))
    {
        status = STATUS_INVALID_IMAGE_FORMAT;
        goto failed;
    }
#endif
    status = RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &nt_name, NULL, NULL );
    if (status) goto failed;
    status = build_module( NULL, &nt_name, &module, &info, NULL, DONT_RESOLVE_DLL_REFERENCES, FALSE, &wm );
    if (status) goto failed;
    RtlFreeUnicodeString( &nt_name );
    wm->ldr.LoadCount = -1;
    return wm;
failed:
    MESSAGE( "wine: failed to create main module for %s, status %lx\n",
             debugstr_us(&params->ImagePathName), status );
    NtTerminateProcess( GetCurrentProcess(), status );
    return NULL;  /* unreached */
}


/***********************************************************************
 *	build_dlldata_path
 *
 * Helper for find_actctx_dll.
 */
static NTSTATUS build_dlldata_path( LPCWSTR libname, ACTCTX_SECTION_KEYED_DATA *data, LPWSTR *fullname )
{
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *dlldata = data->lpData;
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *path;
    char *base = data->lpSectionBase;
    SIZE_T total = dlldata->TotalPathLength + (wcslen(libname) + 1) * sizeof(WCHAR);
    WCHAR *p, *buffer;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;

    if (!(p = buffer = RtlAllocateHeap( GetProcessHeap(), 0, total ))) return STATUS_NO_MEMORY;
    path = (ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *)(dlldata + 1);
    for (i = 0; i < dlldata->PathSegmentCount; i++)
    {
        memcpy( p, base + path[i].Offset, path[i].Length );
        p += path[i].Length / sizeof(WCHAR);
    }
    if (p == buffer || p[-1] == '\\') wcscpy( p, libname );
    else *p = 0;

    if (dlldata->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND)
    {
        RtlExpandEnvironmentStrings( NULL, buffer, wcslen(buffer), NULL, 0, &total );
        if ((*fullname = RtlAllocateHeap( GetProcessHeap(), 0, total * sizeof(WCHAR) )))
            RtlExpandEnvironmentStrings( NULL, buffer, wcslen(buffer), *fullname, total, NULL );
        else
            status = STATUS_NO_MEMORY;

        RtlFreeHeap( GetProcessHeap(), 0, buffer );
    }
    else *fullname = buffer;

    return status;
}


/***********************************************************************
 *	find_actctx_dll
 *
 * Find the full path (if any) of the dll from the activation context.
 */
static NTSTATUS find_actctx_dll( LPCWSTR libname, LPWSTR *fullname )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\'};

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info = NULL;
    ACTCTX_SECTION_KEYED_DATA data;
    ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *dlldata;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    RtlInitUnicodeString( &nameW, libname );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    if (data.ulLength < sizeof(*dlldata))
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }
    dlldata = data.lpData;
    if (!(dlldata->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT))
    {
        status = build_dlldata_path( libname, &data, fullname );
        goto done;
    }

    for (;;)
    {
        if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( GetProcessHeap(), 0, info );
        size = needed;
        /* restart with larger buffer */
    }

    if (!info->lpAssemblyManifestPath)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    if ((p = wcsrchr( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD len, dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
        p++;
        len = wcslen( p );
        if (!dirlen || len <= dirlen ||
            RtlCompareUnicodeStrings( p, dirlen, info->lpAssemblyDirectoryName, dirlen, TRUE ) ||
            wcsicmp( p + dirlen, L".manifest" ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR) + nameW.Length;
            if (!(*fullname = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            p += dirlen;
            wcscpy( p, libname );
            goto done;
        }
    }

    if (!info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    needed = (wcslen(windows_dir) * sizeof(WCHAR) +
              sizeof(winsxsW) + info->ulAssemblyDirectoryNameLength + nameW.Length + 2*sizeof(WCHAR));

    if (!(*fullname = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    wcscpy( p, windows_dir );
    p += wcslen(p);
    memcpy( p, winsxsW, sizeof(winsxsW) );
    p += ARRAY_SIZE( winsxsW );
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = '\\';
    wcscpy( p, libname );
done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    return status;
}



/******************************************************************************
 *	find_apiset_dll
 */
static NTSTATUS find_apiset_dll( const WCHAR *name, WCHAR **fullname )
{
    const API_SET_NAMESPACE *map = NtCurrentTeb()->Peb->ApiSetMap;
    const API_SET_NAMESPACE_ENTRY *entry;
    UNICODE_STRING str;
    ULONG len;

    if (get_apiset_entry( map, name, wcslen(name), &entry )) return STATUS_APISET_NOT_PRESENT;
    if (get_apiset_target( map, entry, NULL, &str )) return STATUS_DLL_NOT_FOUND;

    len = wcslen( system_dir ) + str.Length / sizeof(WCHAR);
    if (!(*fullname = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;
    wcscpy( *fullname, system_dir );
    memcpy( *fullname + wcslen( system_dir ), str.Buffer, str.Length );
    (*fullname)[len] = 0;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *	get_env_var
 */
static NTSTATUS get_env_var( const WCHAR *name, SIZE_T extra, UNICODE_STRING *ret )
{
    NTSTATUS status;
    SIZE_T len, size = 1024 + extra;

    for (;;)
    {
        ret->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, size * sizeof(WCHAR) );
        status = RtlQueryEnvironmentVariable( NULL, name, wcslen(name),
                                              ret->Buffer, size - extra - 1, &len );
        if (!status)
        {
            ret->Buffer[len] = 0;
            ret->Length = len * sizeof(WCHAR);
            ret->MaximumLength = size * sizeof(WCHAR);
            return status;
        }
        RtlFreeHeap( GetProcessHeap(), 0, ret->Buffer );
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            ret->Buffer = NULL;
            return status;
        }
        size = len + 1 + extra;
    }
}


/***********************************************************************
 *	find_builtin_without_file
 *
 * Find a builtin dll when the corresponding file cannot be found in the prefix.
 * This is used during prefix bootstrap.
 */
static NTSTATUS find_builtin_without_file( const WCHAR *name, UNICODE_STRING *new_name,
                                           WINE_MODREF **pwm, HANDLE *mapping,
                                           SECTION_IMAGE_INFORMATION *image_info, struct file_id *id )
{
    const WCHAR *ext;
    WCHAR dllpath[32];
    DWORD i, len;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    BOOL found_image = FALSE;

    if (contains_path( name )) return status;

    if (!is_prefix_bootstrap)
    {
        /* 16-bit files can't be loaded from the prefix */
        if (!name[1] || wcscmp( name + wcslen(name) - 2, L"16" )) return status;
    }

    if (!get_env_var( L"WINEBUILDDIR", 20 + 2 * wcslen(name) + wcslen(pe_dir), new_name ))
    {
        len = new_name->Length;
        RtlAppendUnicodeToString( new_name, L"\\dlls\\" );
        RtlAppendUnicodeToString( new_name, name );
        if ((ext = wcsrchr( name, '.' )) && !wcscmp( ext, L".dll" )) new_name->Length -= 4 * sizeof(WCHAR);
        RtlAppendUnicodeToString( new_name, pe_dir );
        RtlAppendUnicodeToString( new_name, L"\\" );
        RtlAppendUnicodeToString( new_name, name );
        status = open_dll_file( new_name, pwm, mapping, image_info, id );
        if (status != STATUS_DLL_NOT_FOUND) goto done;

        new_name->Length = len;
        RtlAppendUnicodeToString( new_name, L"\\programs\\" );
        RtlAppendUnicodeToString( new_name, name );
        RtlAppendUnicodeToString( new_name, pe_dir );
        RtlAppendUnicodeToString( new_name, L"\\" );
        RtlAppendUnicodeToString( new_name, name );
        status = open_dll_file( new_name, pwm, mapping, image_info, id );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        RtlFreeUnicodeString( new_name );
    }

    for (i = 0; ; i++)
    {
        swprintf( dllpath, ARRAY_SIZE(dllpath), L"WINEDLLDIR%u", i );
        if (get_env_var( dllpath, wcslen(pe_dir) + wcslen(name) + 1, new_name )) break;
        len = new_name->Length;
        RtlAppendUnicodeToString( new_name, pe_dir );
        RtlAppendUnicodeToString( new_name, L"\\" );
        RtlAppendUnicodeToString( new_name, name );
        status = open_dll_file( new_name, pwm, mapping, image_info, id );
        if (status != STATUS_DLL_NOT_FOUND) goto done;
        new_name->Length = len;
        RtlAppendUnicodeToString( new_name, L"\\" );
        RtlAppendUnicodeToString( new_name, name );
        status = open_dll_file( new_name, pwm, mapping, image_info, id );
        if (status == STATUS_NOT_SUPPORTED) found_image = TRUE;
        else if (status != STATUS_DLL_NOT_FOUND) goto done;
        RtlFreeUnicodeString( new_name );
    }
    if (found_image) status = STATUS_NOT_SUPPORTED;

done:
    RtlFreeUnicodeString( new_name );
    if (!status)
    {
        new_name->Length = (4 + wcslen(system_dir) + wcslen(name)) * sizeof(WCHAR);
        new_name->Buffer = RtlAllocateHeap( GetProcessHeap(), 0, new_name->Length + sizeof(WCHAR) );
        wcscpy( new_name->Buffer, L"\\??\\" );
        wcscat( new_name->Buffer, system_dir );
        wcscat( new_name->Buffer, name );
    }
    return status;
}


/***********************************************************************
 *	search_dll_file
 *
 * Search for dll in the specified paths.
 */
static NTSTATUS search_dll_file( LPCWSTR paths, LPCWSTR search, UNICODE_STRING *nt_name,
                                 WINE_MODREF **pwm, HANDLE *mapping, SECTION_IMAGE_INFORMATION *image_info,
                                 struct file_id *id )
{
    WCHAR *name;
    BOOL found_image = FALSE;
    NTSTATUS status = STATUS_DLL_NOT_FOUND;
    ULONG len;

    if (!paths) paths = default_load_path;
    len = wcslen( paths );

    if (len < wcslen( system_dir )) len = wcslen( system_dir );
    len += wcslen( search ) + 2;

    if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    while (*paths)
    {
        LPCWSTR ptr = paths;

        while (*ptr && *ptr != ';') ptr++;
        len = ptr - paths;
        if (*ptr == ';') ptr++;
        memcpy( name, paths, len * sizeof(WCHAR) );
        if (len && name[len - 1] != '\\') name[len++] = '\\';
        wcscpy( name + len, search );

        nt_name->Buffer = NULL;
        if ((status = RtlDosPathNameToNtPathName_U_WithStatus( name, nt_name, NULL, NULL ))) goto done;

        status = open_dll_file( nt_name, pwm, mapping, image_info, id );
        if (status == STATUS_NOT_SUPPORTED) found_image = TRUE;
        else if (status != STATUS_DLL_NOT_FOUND) goto done;
        RtlFreeUnicodeString( nt_name );
        paths = ptr;
    }

    if (found_image) status = STATUS_NOT_SUPPORTED;

done:
    RtlFreeHeap( GetProcessHeap(), 0, name );
    return status;
}

/***********************************************************************
 *	find_dll_file
 *
 * Find the file (or already loaded module) for a given dll name.
 */
static NTSTATUS find_dll_file( const WCHAR *load_path, const WCHAR *libname, UNICODE_STRING *nt_name,
                               WINE_MODREF **pwm, HANDLE *mapping, SECTION_IMAGE_INFORMATION *image_info,
                               struct file_id *id, BOOL find_loaded )
{
    WCHAR *fullname = NULL;
    NTSTATUS status;
    ULONG wow64_old_value = 0;

    *pwm = NULL;

    /* Win 7/2008R2 and up seem to re-enable WoW64 FS redirection when loading libraries */
    RtlWow64EnableFsRedirectionEx( 0, &wow64_old_value );

    nt_name->Buffer = NULL;

    if (!contains_path( libname ))
    {
        status = find_apiset_dll( libname, &fullname );
        if (status == STATUS_DLL_NOT_FOUND) goto done;

        if (status) status = find_actctx_dll( libname, &fullname );

        if (status == STATUS_SUCCESS)
        {
            TRACE ("found %s for %s\n", debugstr_w(fullname), debugstr_w(libname) );
            libname = fullname;
        }
        else
        {
            if (status != STATUS_SXS_KEY_NOT_FOUND) goto done;
            if ((*pwm = find_basename_module( libname )) != NULL)
            {
                status = STATUS_SUCCESS;
                goto done;
            }
            if (find_loaded)
            {
                TRACE( "Skipping file search for %s.\n", debugstr_w(libname) );
                status = STATUS_DLL_NOT_FOUND;
                goto done;
            }
        }
    }

    if (RtlDetermineDosPathNameType_U( libname ) == RELATIVE_PATH)
    {
        status = search_dll_file( load_path, libname, nt_name, pwm, mapping, image_info, id );
        if (status == STATUS_DLL_NOT_FOUND)
            status = find_builtin_without_file( libname, nt_name, pwm, mapping, image_info, id );
    }
    else if (!(status = RtlDosPathNameToNtPathName_U_WithStatus( libname, nt_name, NULL, NULL )))
        status = open_dll_file( nt_name, pwm, mapping, image_info, id );

    if (status == STATUS_NOT_SUPPORTED) status = STATUS_INVALID_IMAGE_FORMAT;

done:
    RtlFreeHeap( GetProcessHeap(), 0, fullname );
    if (wow64_old_value) RtlWow64EnableFsRedirectionEx( 1, &wow64_old_value );
    return status;
}


/***********************************************************************
 *	load_dll  (internal)
 *
 * Load a PE style module according to the load order.
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS load_dll( const WCHAR *load_path, const WCHAR *libname, DWORD flags, WINE_MODREF** pwm, BOOL system )
{
    UNICODE_STRING nt_name;
    struct file_id id;
    HANDLE mapping = 0;
    SECTION_IMAGE_INFORMATION image_info;
    NTSTATUS nts = STATUS_DLL_NOT_FOUND;
    void *prev;

    TRACE( "looking for %s in %s\n", debugstr_w(libname), debugstr_w(load_path) );

    if (system && system_dll_path.Buffer)
        nts = search_dll_file( system_dll_path.Buffer, libname, &nt_name, pwm, &mapping, &image_info, &id );

    if (nts)
    {
        nts = find_dll_file( load_path, libname, &nt_name, pwm, &mapping, &image_info, &id, FALSE );
        system = FALSE;
    }

    if (*pwm)  /* found already loaded module */
    {
        if ((*pwm)->ldr.LoadCount != -1) (*pwm)->ldr.LoadCount++;

        TRACE("Found %s for %s at %p, count=%d\n",
              debugstr_w((*pwm)->ldr.FullDllName.Buffer), debugstr_w(libname),
              (*pwm)->ldr.DllBase, (*pwm)->ldr.LoadCount);
        RtlFreeUnicodeString( &nt_name );
        return STATUS_SUCCESS;
    }

    if (nts && nts != STATUS_INVALID_IMAGE_NOT_MZ) goto done;

    prev = NtCurrentTeb()->Tib.ArbitraryUserPointer;
    NtCurrentTeb()->Tib.ArbitraryUserPointer = nt_name.Buffer + 4;

    switch (nts)
    {
    case STATUS_INVALID_IMAGE_NOT_MZ:  /* not in PE format, maybe it's a .so file */
        if (__wine_unixlib_handle) nts = load_so_dll( load_path, &nt_name, flags, pwm );
        break;

    case STATUS_SUCCESS:  /* valid PE file */
        nts = load_native_dll( load_path, &nt_name, mapping, &image_info, &id, flags, system, pwm );
        break;
    }

    NtCurrentTeb()->Tib.ArbitraryUserPointer = prev;

done:
    if (nts == STATUS_SUCCESS)
        TRACE("Loaded module %s at %p\n", debugstr_us(&nt_name), (*pwm)->ldr.DllBase);
    else
        WARN("Failed to load module %s; status=%lx\n", debugstr_w(libname), nts);

    if (mapping) NtClose( mapping );
    RtlFreeUnicodeString( &nt_name );
    return nts;
}


/***********************************************************************
 *              __wine_ctrl_routine
 */
NTSTATUS WINAPI __wine_ctrl_routine( void *arg )
{
    DWORD ret = pCtrlRoutine ? pCtrlRoutine( arg ) : 0;
    RtlExitUserThread( ret );
}


/***********************************************************************
 *           __wine_unix_spawnvp
 */
NTSTATUS WINAPI __wine_unix_spawnvp( char * const argv[], int wait )
{
    struct wine_spawnvp_params params = { (char **)argv, wait };

    return WINE_UNIX_CALL( unix_wine_spawnvp, &params );
}


/***********************************************************************
 *           wine_server_call
 */
unsigned int CDECL wine_server_call( void *req_ptr )
{
    return WINE_UNIX_CALL( unix_wine_server_call, req_ptr );
}


/***********************************************************************
 *           wine_server_fd_to_handle
 */
NTSTATUS CDECL wine_server_fd_to_handle( int fd, unsigned int access, unsigned int attributes,
                                         HANDLE *handle )
{
    struct wine_server_fd_to_handle_params params = { fd, access, attributes, handle };

    return WINE_UNIX_CALL( unix_wine_server_fd_to_handle, &params );
}


/***********************************************************************
 *           wine_server_handle_to_fd (NTDLL.@)
 */
NTSTATUS CDECL wine_server_handle_to_fd( HANDLE handle, unsigned int access, int *unix_fd,
                                         unsigned int *options )
{
    struct wine_server_handle_to_fd_params params = { handle, access, unix_fd, options };

    return WINE_UNIX_CALL( unix_wine_server_handle_to_fd, &params );
}

/******************************************************************
 *		LdrLoadDll (NTDLL.@)
 */
NTSTATUS WINAPI DECLSPEC_HOTPATCH LdrLoadDll(LPCWSTR path_name, DWORD flags,
                                             const UNICODE_STRING *libname, HMODULE* hModule)
{
    WINE_MODREF *wm;
    NTSTATUS nts;
    WCHAR *dllname = append_dll_ext( libname->Buffer );

    RtlEnterCriticalSection( &loader_section );

    nts = load_dll( path_name, dllname ? dllname : libname->Buffer, flags, &wm, FALSE );

    if (nts == STATUS_SUCCESS && !(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS))
    {
        nts = process_attach( wm->ldr.DdagNode, NULL );
        if (nts != STATUS_SUCCESS)
        {
            LdrUnloadDll(wm->ldr.DllBase);
            wm = NULL;
        }
    }
    *hModule = (wm) ? wm->ldr.DllBase : NULL;

    RtlLeaveCriticalSection( &loader_section );
    RtlFreeHeap( GetProcessHeap(), 0, dllname );
    return nts;
}


/******************************************************************
 *		LdrGetDllFullName (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllFullName( HMODULE module, UNICODE_STRING *name )
{
    WINE_MODREF *wm;
    NTSTATUS status;

    TRACE( "module %p, name %p.\n", module, name );

    if (!module) module = NtCurrentTeb()->Peb->ImageBaseAddress;

    RtlEnterCriticalSection( &loader_section );
    wm = get_modref( module );
    if (wm)
    {
        RtlCopyUnicodeString( name, &wm->ldr.FullDllName );
        if (name->MaximumLength < wm->ldr.FullDllName.Length + sizeof(WCHAR)) status = STATUS_BUFFER_TOO_SMALL;
        else status = STATUS_SUCCESS;
    } else status = STATUS_DLL_NOT_FOUND;
    RtlLeaveCriticalSection( &loader_section );

    return status;
}


/******************************************************************
 *		LdrGetDllHandleEx (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllHandleEx( ULONG flags, LPCWSTR load_path, ULONG *dll_characteristics,
                                           const UNICODE_STRING *name, HMODULE *base )
{
    static const ULONG supported_flags = LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                                         | LDR_GET_DLL_HANDLE_EX_FLAG_PIN;
    static const ULONG valid_flags = LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                                     | LDR_GET_DLL_HANDLE_EX_FLAG_PIN | 4;
    SECTION_IMAGE_INFORMATION image_info;
    UNICODE_STRING nt_name;
    struct file_id id;
    NTSTATUS status;
    WINE_MODREF *wm;
    WCHAR *dllname;
    HANDLE mapping;

    TRACE( "flags %#lx, load_path %p, dll_characteristics %p, name %p, base %p.\n",
            flags, load_path, dll_characteristics, name, base );

    if (flags & ~valid_flags) return STATUS_INVALID_PARAMETER;

    if ((flags & (LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | LDR_GET_DLL_HANDLE_EX_FLAG_PIN))
                 == (LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | LDR_GET_DLL_HANDLE_EX_FLAG_PIN))
        return STATUS_INVALID_PARAMETER;

    if (flags & ~supported_flags) FIXME( "Unsupported flags %#lx.\n", flags );
    if (dll_characteristics) FIXME( "dll_characteristics unsupported.\n" );

    dllname = append_dll_ext( name->Buffer );

    RtlEnterCriticalSection( &loader_section );

    status = find_dll_file( load_path, dllname ? dllname : name->Buffer,
                            &nt_name, &wm, &mapping, &image_info, &id, TRUE );

    if (wm) *base = wm->ldr.DllBase;
    else
    {
        if (status == STATUS_SUCCESS) NtClose( mapping );
        status = STATUS_DLL_NOT_FOUND;
    }
    RtlFreeUnicodeString( &nt_name );

    if (!status)
    {
        if (flags & LDR_GET_DLL_HANDLE_EX_FLAG_PIN)
            LdrAddRefDll( LDR_ADDREF_DLL_PIN, *base );
        else if (!(flags & LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
            LdrAddRefDll( 0, *base );
    }

    RtlLeaveCriticalSection( &loader_section );
    RtlFreeHeap( GetProcessHeap(), 0, dllname );
    TRACE( "%s -> %p (load path %s)\n", debugstr_us(name), status ? NULL : *base, debugstr_w(load_path) );
    return status;
}


/******************************************************************
 *		LdrGetDllHandle (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllHandle( LPCWSTR load_path, ULONG flags, const UNICODE_STRING *name, HMODULE *base )
{
    return LdrGetDllHandleEx( LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, load_path, NULL, name, base );
}


/******************************************************************
 *		LdrAddRefDll (NTDLL.@)
 */
NTSTATUS WINAPI LdrAddRefDll( ULONG flags, HMODULE module )
{
    NTSTATUS ret = STATUS_SUCCESS;
    WINE_MODREF *wm;

    if (flags & ~LDR_ADDREF_DLL_PIN) FIXME( "%p flags %lx not implemented\n", module, flags );

    RtlEnterCriticalSection( &loader_section );

    if ((wm = get_modref( module )))
    {
        if (flags & LDR_ADDREF_DLL_PIN)
            wm->ldr.LoadCount = -1;
        else
            if (wm->ldr.LoadCount != -1) wm->ldr.LoadCount++;
        TRACE( "(%s) ldr.LoadCount: %d\n", debugstr_w(wm->ldr.BaseDllName.Buffer), wm->ldr.LoadCount );
    }
    else ret = STATUS_INVALID_PARAMETER;

    RtlLeaveCriticalSection( &loader_section );
    return ret;
}


/***********************************************************************
 *           LdrProcessRelocationBlock  (NTDLL.@)
 *
 * Apply relocations to a given page of a mapped PE image.
 */
IMAGE_BASE_RELOCATION * WINAPI LdrProcessRelocationBlock( void *page, UINT count,
                                                          USHORT *relocs, INT_PTR delta )
{
    while (count--)
    {
        USHORT offset = *relocs & 0xfff;
        int type = *relocs >> 12;
        switch(type)
        {
        case IMAGE_REL_BASED_ABSOLUTE:
            break;
        case IMAGE_REL_BASED_HIGH:
            *(short *)((char *)page + offset) += HIWORD(delta);
            break;
        case IMAGE_REL_BASED_LOW:
            *(short *)((char *)page + offset) += LOWORD(delta);
            break;
        case IMAGE_REL_BASED_HIGHLOW:
            *(int *)((char *)page + offset) += delta;
            break;
#ifdef _WIN64
        case IMAGE_REL_BASED_DIR64:
            *(INT_PTR *)((char *)page + offset) += delta;
            break;
#elif defined(__arm__)
        case IMAGE_REL_BASED_THUMB_MOV32:
        {
            UINT *inst = (UINT *)((char *)page + offset);
            WORD lo = ((inst[0] << 1) & 0x0800) + ((inst[0] << 12) & 0xf000) +
                      ((inst[0] >> 20) & 0x0700) + ((inst[0] >> 16) & 0x00ff);
            WORD hi = ((inst[1] << 1) & 0x0800) + ((inst[1] << 12) & 0xf000) +
                      ((inst[1] >> 20) & 0x0700) + ((inst[1] >> 16) & 0x00ff);
            DWORD imm = MAKELONG( lo, hi ) + delta;

            lo = LOWORD( imm );
            hi = HIWORD( imm );

            if ((inst[0] & 0x8000fbf0) != 0x0000f240 || (inst[1] & 0x8000fbf0) != 0x0000f2c0)
                ERR("wrong Thumb2 instruction @%p %08x:%08x, expected MOVW/MOVT\n",
                    inst, inst[0], inst[1] );

            inst[0] = (inst[0] & 0x8f00fbf0) + ((lo >> 1) & 0x0400) + ((lo >> 12) & 0x000f) +
                                               ((lo << 20) & 0x70000000) + ((lo << 16) & 0xff0000);
            inst[1] = (inst[1] & 0x8f00fbf0) + ((hi >> 1) & 0x0400) + ((hi >> 12) & 0x000f) +
                                               ((hi << 20) & 0x70000000) + ((hi << 16) & 0xff0000);
            break;
        }
#endif
        default:
            FIXME("Unknown/unsupported fixup type %x.\n", type);
            return NULL;
        }
        relocs++;
    }
    return (IMAGE_BASE_RELOCATION *)relocs;  /* return address of next block */
}


/******************************************************************
 *		LdrQueryProcessModuleInformation
 *
 */
NTSTATUS WINAPI LdrQueryProcessModuleInformation(RTL_PROCESS_MODULES *smi,
                                                 ULONG buf_size, ULONG* req_size)
{
    RTL_PROCESS_MODULE_INFORMATION *sm = &smi->Modules[0];
    ULONG               size = sizeof(ULONG);
    NTSTATUS            nts = STATUS_SUCCESS;
    ANSI_STRING         str;
    char*               ptr;
    PLIST_ENTRY         mark, entry;
    LDR_DATA_TABLE_ENTRY *mod;
    WORD id = 0;

    smi->ModulesCount = 0;

    RtlEnterCriticalSection( &loader_section );
    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        size += sizeof(*sm);
        if (size <= buf_size)
        {
            sm->Section = 0; /* FIXME */
            sm->MappedBaseAddress = mod->DllBase;
            sm->ImageBaseAddress = mod->DllBase;
            sm->ImageSize = mod->SizeOfImage;
            sm->Flags = mod->Flags;
            sm->LoadOrderIndex = id++;
            sm->InitOrderIndex = 0; /* FIXME */
            sm->LoadCount = mod->LoadCount;
            str.Length = 0;
            str.MaximumLength = MAXIMUM_FILENAME_LENGTH;
            str.Buffer = (char*)sm->Name;
            RtlUnicodeStringToAnsiString(&str, &mod->FullDllName, FALSE);
            ptr = strrchr(str.Buffer, '\\');
            sm->NameOffset = (ptr != NULL) ? (ptr - str.Buffer + 1) : 0;

            smi->ModulesCount++;
            sm++;
        }
        else nts = STATUS_INFO_LENGTH_MISMATCH;
    }
    RtlLeaveCriticalSection( &loader_section );

    if (req_size) *req_size = size;

    return nts;
}


static NTSTATUS query_dword_option( HANDLE hkey, LPCWSTR name, LONG *value )
{
    NTSTATUS status;
    UNICODE_STRING str;
    ULONG size;
    WCHAR buffer[64];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

    RtlInitUnicodeString( &str, name );

    size = sizeof(buffer) - sizeof(WCHAR);
    if ((status = NtQueryValueKey( hkey, &str, KeyValuePartialInformation, buffer, size, &size )))
        return status;

    if (info->Type != REG_DWORD)
    {
        buffer[size / sizeof(WCHAR)] = 0;
        *value = wcstoul( (WCHAR *)info->Data, 0, 16 );
    }
    else memcpy( value, info->Data, sizeof(*value) );
    return status;
}

static NTSTATUS query_string_option( HANDLE hkey, LPCWSTR name, ULONG type,
                                     void *data, ULONG in_size, ULONG *out_size )
{
    NTSTATUS status;
    UNICODE_STRING str;
    ULONG size;
    char *buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    RtlInitUnicodeString( &str, name );

    size = info_size + in_size;
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, size ))) return STATUS_NO_MEMORY;
    info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    status = NtQueryValueKey( hkey, &str, KeyValuePartialInformation, buffer, size, &size );
    if (!status || status == STATUS_BUFFER_OVERFLOW)
    {
        if (out_size) *out_size = info->DataLength;
        if (data && !status) memcpy( data, info->Data, info->DataLength );
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return status;
}


/******************************************************************
 *		LdrQueryImageFileExecutionOptions  (NTDLL.@)
 */
NTSTATUS WINAPI LdrQueryImageFileExecutionOptions( const UNICODE_STRING *key, LPCWSTR value, ULONG type,
                                                   void *data, ULONG in_size, ULONG *out_size )
{
    static const WCHAR optionsW[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
    WCHAR path[MAX_PATH + ARRAY_SIZE( optionsW )];
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name_str;
    HANDLE hkey;
    NTSTATUS status;
    ULONG len;
    WCHAR *p;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name_str;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    p = key->Buffer + key->Length / sizeof(WCHAR);
    while (p > key->Buffer && p[-1] != '\\') p--;
    len = key->Length - (p - key->Buffer) * sizeof(WCHAR);
    name_str.Buffer = path;
    name_str.Length = sizeof(optionsW) - sizeof(WCHAR) + len;
    name_str.MaximumLength = name_str.Length;
    memcpy( path, optionsW, sizeof(optionsW) );
    memcpy( path + ARRAY_SIZE( optionsW ) - 1, p, len );
    if ((status = NtOpenKey( &hkey, KEY_QUERY_VALUE, &attr ))) return status;

    if (type == REG_DWORD)
    {
        if (out_size) *out_size = sizeof(ULONG);
        if (in_size >= sizeof(ULONG)) status = query_dword_option( hkey, value, data );
        else status = STATUS_BUFFER_OVERFLOW;
    }
    else status = query_string_option( hkey, value, type, data, in_size, out_size );

    NtClose( hkey );
    return status;
}


/******************************************************************
 *		RtlDllShutdownInProgress  (NTDLL.@)
 */
BOOLEAN WINAPI RtlDllShutdownInProgress(void)
{
    return process_detaching;
}

/****************************************************************************
 *              LdrResolveDelayLoadedAPI   (NTDLL.@)
 */
void* WINAPI LdrResolveDelayLoadedAPI( void* base, const IMAGE_DELAYLOAD_DESCRIPTOR* desc,
                                       PDELAYLOAD_FAILURE_DLL_CALLBACK dllhook,
                                       PDELAYLOAD_FAILURE_SYSTEM_ROUTINE syshook,
                                       IMAGE_THUNK_DATA* addr, ULONG flags )
{
    IMAGE_THUNK_DATA *pIAT, *pINT;
    DELAYLOAD_INFO delayinfo;
    UNICODE_STRING mod;
    const CHAR* name;
    HMODULE *phmod;
    NTSTATUS nts;
    FARPROC fp;
    DWORD id;

    TRACE( "(%p, %p, %p, %p, %p, 0x%08lx)\n", base, desc, dllhook, syshook, addr, flags );

    phmod = get_rva(base, desc->ModuleHandleRVA);
    pIAT = get_rva(base, desc->ImportAddressTableRVA);
    pINT = get_rva(base, desc->ImportNameTableRVA);
    name = get_rva(base, desc->DllNameRVA);
    id = addr - pIAT;

    if (!*phmod)
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&mod, name))
        {
            nts = STATUS_NO_MEMORY;
            goto fail;
        }
        nts = LdrLoadDll(NULL, 0, &mod, phmod);
        RtlFreeUnicodeString(&mod);
        if (nts) goto fail;
    }

    if (IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal))
        nts = LdrGetProcedureAddress(*phmod, NULL, LOWORD(pINT[id].u1.Ordinal), (void**)&fp);
    else
    {
        const IMAGE_IMPORT_BY_NAME* iibn = get_rva(base, pINT[id].u1.AddressOfData);
        ANSI_STRING fnc;

        RtlInitAnsiString(&fnc, (char*)iibn->Name);
        nts = LdrGetProcedureAddress(*phmod, &fnc, 0, (void**)&fp);
    }
    if (!nts)
    {
        pIAT[id].u1.Function = (ULONG_PTR)fp;
        return fp;
    }

fail:
    delayinfo.Size = sizeof(delayinfo);
    delayinfo.DelayloadDescriptor = desc;
    delayinfo.ThunkAddress = addr;
    delayinfo.TargetDllName = name;
    delayinfo.TargetApiDescriptor.ImportDescribedByName = !IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal);
    delayinfo.TargetApiDescriptor.Description.Ordinal = LOWORD(pINT[id].u1.Ordinal);
    delayinfo.TargetModuleBase = *phmod;
    delayinfo.Unused = NULL;
    delayinfo.LastError = nts;

    if (dllhook)
        return dllhook(4, &delayinfo);

    if (IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal))
    {
        DWORD_PTR ord = LOWORD(pINT[id].u1.Ordinal);
        return syshook(name, (const char *)ord);
    }
    else
    {
        const IMAGE_IMPORT_BY_NAME* iibn = get_rva(base, pINT[id].u1.AddressOfData);
        return syshook(name, (const char *)iibn->Name);
    }
}

/******************************************************************
 *		LdrShutdownProcess (NTDLL.@)
 *
 */
void WINAPI LdrShutdownProcess(void)
{
    BOOL detaching = process_detaching;

    TRACE("()\n");

    process_detaching = TRUE;
    if (!detaching)
        RtlProcessFlsData( NtCurrentTeb()->FlsSlots, 1 );

    process_detach();
}


/******************************************************************
 *		RtlExitUserProcess (NTDLL.@)
 */
void WINAPI RtlExitUserProcess( DWORD status )
{
    RtlEnterCriticalSection( &loader_section );
    RtlAcquirePebLock();
    NtTerminateProcess( 0, status );
    LdrShutdownProcess();
    for (;;) NtTerminateProcess( GetCurrentProcess(), status );
}

/******************************************************************
 *		LdrShutdownThread (NTDLL.@)
 *
 */
void WINAPI LdrShutdownThread(void)
{
    PLIST_ENTRY mark, entry;
    LDR_DATA_TABLE_ENTRY *mod;
    WINE_MODREF *wm;
    UINT i;
    void **pointers;

    TRACE("()\n");

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return;

    RtlProcessFlsData( NtCurrentTeb()->FlsSlots, 1 );

    RtlEnterCriticalSection( &loader_section );
    wm = get_modref( NtCurrentTeb()->Peb->ImageBaseAddress );

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = entry->Blink)
    {
        mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY,
                                InInitializationOrderLinks);
        if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
            continue;
        if ( mod->Flags & LDR_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), 
                        DLL_THREAD_DETACH, NULL );
    }

    if (wm->ldr.TlsIndex == -1) call_tls_callbacks( wm->ldr.DllBase, DLL_THREAD_DETACH );

    RtlAcquirePebLock();
    if ((pointers = NtCurrentTeb()->ThreadLocalStoragePointer))
    {
        NtCurrentTeb()->ThreadLocalStoragePointer = NULL;
#ifdef __x86_64__  /* macOS-specific hack */
        if (NtCurrentTeb()->Instrumentation[0])
            ((TEB *)NtCurrentTeb()->Instrumentation[0])->ThreadLocalStoragePointer = NULL;
#endif
        for (i = 0; i < tls_module_count; i++) RtlFreeHeap( GetProcessHeap(), 0, pointers[i] );
        RtlFreeHeap( GetProcessHeap(), 0, pointers );
    }
    RtlProcessFlsData( NtCurrentTeb()->FlsSlots, 2 );
    NtCurrentTeb()->FlsSlots = NULL;
    RtlFreeHeap( GetProcessHeap(), 0, NtCurrentTeb()->TlsExpansionSlots );
    NtCurrentTeb()->TlsExpansionSlots = NULL;
    RtlReleasePebLock();

    RtlLeaveCriticalSection( &loader_section );
    /* don't call DbgUiGetThreadDebugObject as some apps hook it and terminate if called */
    if (NtCurrentTeb()->DbgSsReserved[1]) NtClose( NtCurrentTeb()->DbgSsReserved[1] );
    RtlFreeThreadActivationContextStack();

    heap_thread_detach();
}


/***********************************************************************
 *           free_modref
 *
 */
static void free_modref( WINE_MODREF *wm )
{
    SINGLE_LIST_ENTRY *entry;
    LDR_DEPENDENCY *dep;

    RemoveEntryList(&wm->ldr.InLoadOrderLinks);
    RemoveEntryList(&wm->ldr.InMemoryOrderLinks);
    RemoveEntryList(&wm->ldr.HashLinks);
    RtlRbRemoveNode( &base_address_index_tree, &wm->ldr.BaseAddressIndexNode );
    if (wm->ldr.InInitializationOrderLinks.Flink)
        RemoveEntryList(&wm->ldr.InInitializationOrderLinks);

    while ((entry = wm->ldr.DdagNode->Dependencies.Tail))
    {
        dep = CONTAINING_RECORD( entry, LDR_DEPENDENCY, dependency_to_entry );
        assert( dep->dependency_from == wm->ldr.DdagNode );
        remove_module_dependency( dep );
    }

    while ((entry = wm->ldr.DdagNode->IncomingDependencies.Tail))
    {
        dep = CONTAINING_RECORD( entry, LDR_DEPENDENCY, dependency_from_entry );
        assert( dep->dependency_to == wm->ldr.DdagNode );
        remove_module_dependency( dep );
    }

    RemoveEntryList(&wm->ldr.NodeModuleLink);
    if (IsListEmpty(&wm->ldr.DdagNode->Modules))
        RtlFreeHeap( GetProcessHeap(), 0, wm->ldr.DdagNode );

    TRACE(" unloading %s\n", debugstr_w(wm->ldr.FullDllName.Buffer));
    if (!TRACE_ON(module))
        TRACE_(loaddll)("Unloaded module %s : %s\n",
                        debugstr_w(wm->ldr.FullDllName.Buffer),
                        (wm->ldr.Flags & LDR_WINE_INTERNAL) ? "builtin" : "native" );

    free_tls_slot( &wm->ldr );
    RtlReleaseActivationContext( wm->ldr.ActivationContext );
    NtUnmapViewOfSection( NtCurrentProcess(), wm->ldr.DllBase );
    if (cached_modref == wm) cached_modref = NULL;
    RtlFreeUnicodeString( &wm->ldr.FullDllName );
    RtlFreeHeap( GetProcessHeap(), 0, wm );
}

/***********************************************************************
 *           MODULE_FlushModrefs
 *
 * Remove all unused modrefs and call the internal unloading routines
 * for the library type.
 *
 * The loader_section must be locked while calling this function.
 */
static void MODULE_FlushModrefs(void)
{
    PLIST_ENTRY mark, entry, prev;
    LDR_DATA_TABLE_ENTRY *mod;
    WINE_MODREF*wm;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = prev)
    {
        mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
        prev = entry->Blink;
        if (!mod->LoadCount) free_modref( wm );
    }

    /* check load order list too for modules that haven't been initialized yet */
    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = prev)
    {
        mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
        prev = entry->Blink;
        if (!mod->LoadCount) free_modref( wm );
    }
}

/***********************************************************************
 *           MODULE_DecRefCount
 *
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS MODULE_DecRefCount( LDR_DDAG_NODE *node, void *context )
{
    LDR_DATA_TABLE_ENTRY *mod;
    WINE_MODREF *wm;

    mod = CONTAINING_RECORD( node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink );
    wm = CONTAINING_RECORD( mod, WINE_MODREF, ldr );

    if ( wm->ldr.Flags & LDR_UNLOAD_IN_PROGRESS )
        return STATUS_SUCCESS;

    if ( wm->ldr.LoadCount <= 0 )
        return STATUS_SUCCESS;

    --wm->ldr.LoadCount;
    TRACE("(%s) ldr.LoadCount: %d\n", debugstr_w(wm->ldr.BaseDllName.Buffer), wm->ldr.LoadCount );

    if ( wm->ldr.LoadCount == 0 )
    {
        wm->ldr.Flags |= LDR_UNLOAD_IN_PROGRESS;
        walk_node_dependencies( node, context, MODULE_DecRefCount );
        wm->ldr.Flags &= ~LDR_UNLOAD_IN_PROGRESS;
        module_push_unload_trace( wm );
    }
    return STATUS_SUCCESS;
}

/******************************************************************
 *		LdrUnloadDll (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrUnloadDll( HMODULE hModule )
{
    WINE_MODREF *wm;
    NTSTATUS retv = STATUS_SUCCESS;

    if (process_detaching) return retv;

    TRACE("(%p)\n", hModule);

    RtlEnterCriticalSection( &loader_section );

    free_lib_count++;
    if ((wm = get_modref( hModule )) != NULL)
    {
        TRACE("(%s) - START\n", debugstr_w(wm->ldr.BaseDllName.Buffer));

        /* Recursively decrement reference counts */
        MODULE_DecRefCount( wm->ldr.DdagNode, NULL );

        /* Call process detach notifications */
        if ( free_lib_count <= 1 )
        {
            process_detach();
            MODULE_FlushModrefs();
        }

        TRACE("END\n");
    }
    else
        retv = STATUS_DLL_NOT_FOUND;

    free_lib_count--;

    RtlLeaveCriticalSection( &loader_section );

    return retv;
}

/***********************************************************************
 *           RtlImageNtHeader   (NTDLL.@)
 */
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE hModule)
{
    IMAGE_NT_HEADERS *ret;

    __TRY
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)hModule;

        ret = NULL;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
            ret = (IMAGE_NT_HEADERS *)((char *)dos + dos->e_lfanew);
            if (ret->Signature != IMAGE_NT_SIGNATURE) ret = NULL;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        return NULL;
    }
    __ENDTRY
    return ret;
}

/***********************************************************************
 *           load_global_options
 */
static void load_global_options(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING bootstrap_mode_str = RTL_CONSTANT_STRING( L"WINEBOOTSTRAPMODE" );
    UNICODE_STRING session_manager_str =
        RTL_CONSTANT_STRING( L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager" );
    UNICODE_STRING val_str;
    HANDLE hkey;

    val_str.MaximumLength = 0;
    is_prefix_bootstrap =
        RtlQueryEnvironmentVariable_U( NULL, &bootstrap_mode_str, &val_str ) != STATUS_VARIABLE_NOT_FOUND;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &session_manager_str;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (!NtOpenKey( &hkey, KEY_QUERY_VALUE, &attr ))
    {
        query_dword_option( hkey, L"SafeProcessSearchMode", &path_safe_mode );
        query_dword_option( hkey, L"SafeDllSearchMode", &dll_safe_mode );
        NtClose( hkey );
    }
}

static BOOL needs_elevation(void)
{
    ACTIVATION_CONTEXT_RUN_LEVEL_INFORMATION run_level;

    if (!RtlQueryInformationActivationContext( 0, NULL, NULL, RunlevelInformationInActivationContext,
                                               &run_level, sizeof(run_level), NULL ))
    {
        TRACE( "image requested run level %#x\n", run_level.RunLevel );
        if (run_level.RunLevel == ACTCTX_RUN_LEVEL_HIGHEST_AVAILABLE
                || run_level.RunLevel == ACTCTX_RUN_LEVEL_REQUIRE_ADMIN)
            return TRUE;
    }
    return FALSE;
}

static void elevate_token(void)
{
    PROCESS_ACCESS_TOKEN token;
    TOKEN_ELEVATION_TYPE type;
    TOKEN_LINKED_TOKEN linked;

    NtQueryInformationToken( GetCurrentThreadEffectiveToken(),
                             TokenElevationType, &type, sizeof(type), NULL );

    if (type == TokenElevationTypeFull) return;

    NtQueryInformationToken( GetCurrentThreadEffectiveToken(),
                             TokenLinkedToken, &linked, sizeof(linked), NULL );
    NtDuplicateToken( linked.LinkedToken, 0, NULL, FALSE, TokenPrimary, &token.Token );

    token.Thread = NULL;
    NtSetInformationProcess( GetCurrentProcess(), ProcessAccessToken, &token, sizeof(token) );
    NtClose( token.Token );
    NtClose( linked.LinkedToken );
}

#ifdef __arm64ec__

static void load_arm64ec_module(void)
{
    ULONG buffer[16];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    UNICODE_STRING nameW = RTL_CONSTANT_STRING( L"\\Registry\\Machine\\Software\\Microsoft\\Wow64\\amd64" );
    WCHAR module[64] = L"C:\\windows\\system32\\xtajit64.dll";
    OBJECT_ATTRIBUTES attr;
    WINE_MODREF *wm;
    NTSTATUS status;
    HANDLE key;

    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (!NtOpenKey( &key, KEY_READ | KEY_WOW64_64KEY, &attr ))
    {
        UNICODE_STRING valueW = RTL_CONSTANT_STRING( L"" );
        ULONG dirlen = wcslen( L"C:\\windows\\system32\\" );
        ULONG size = sizeof(buffer);

        if (!NtQueryValueKey( key, &valueW, KeyValuePartialInformation, buffer, size, &size ) && info->Type == REG_SZ)
        {
            size = sizeof(module) - (dirlen + 1) * sizeof(WCHAR);
            memcpy( module + dirlen, info->Data, min( info->DataLength, size ));
        }
        NtClose( key );
    }

    if ((status = load_dll( NULL, module, 0, &wm, FALSE )) ||
        (status = arm64ec_process_init( wm->ldr.DllBase )))
    {
        ERR( "could not load %s, status %lx\n", debugstr_w(module), status );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
}

#endif

#ifdef _WIN64

static void build_wow64_main_module(void)
{
    UNICODE_STRING nt_name;
    WINE_MODREF *wm;
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
    void *module = NtCurrentTeb()->Peb->ImageBaseAddress;

    RtlDosPathNameToNtPathName_U_WithStatus( params->ImagePathName.Buffer, &nt_name, NULL, NULL );
    wm = alloc_module( module, &nt_name, FALSE );
    assert( wm );
    wm->ldr.LoadCount = -1;
    RtlFreeUnicodeString( &nt_name );
}

static void (WINAPI *pWow64LdrpInitialize)( CONTEXT *ctx );

void (WINAPI *pWow64PrepareForException)( EXCEPTION_RECORD *rec, CONTEXT *context ) = NULL;

static void init_wow64( CONTEXT *context )
{
    if (!imports_fixup_done)
    {
        HMODULE wow64;
        WINE_MODREF *wm;
        NTSTATUS status;
        static const WCHAR wow64_path[] = L"C:\\windows\\system32\\wow64.dll";

        build_wow64_main_module();
        build_ntdll_module();

        if ((status = load_dll( NULL, wow64_path, 0, &wm, FALSE )))
        {
            ERR( "could not load %s, status %lx\n", debugstr_w(wow64_path), status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }
        wow64 = wm->ldr.DllBase;
#define GET_PTR(name) \
        if (!(p ## name = RtlFindExportedRoutineByName( wow64, #name ))) ERR( "failed to load %s\n", #name )

        GET_PTR( Wow64LdrpInitialize );
        GET_PTR( Wow64PrepareForException );
#undef GET_PTR
        imports_fixup_done = TRUE;
    }

    RtlLeaveCriticalSection( &loader_section );
    pWow64LdrpInitialize( context );
}


#else

void *Wow64Transition = NULL;

static void map_wow64cpu(void)
{
    SIZE_T size = 0;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string = RTL_CONSTANT_STRING( L"\\??\\C:\\windows\\sysnative\\wow64cpu.dll" );
    HANDLE file, section;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    InitializeObjectAttributes( &attr, &string, 0, NULL, NULL );
    if ((status = NtOpenFile( &file, GENERIC_READ | SYNCHRONIZE, &attr, &io, FILE_SHARE_READ,
                              FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE )))
    {
        WARN("failed to open wow64cpu, status %#lx\n", status);
        return;
    }
    if (!NtCreateSection( &section, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                          SECTION_MAP_READ | SECTION_MAP_EXECUTE,
                          NULL, NULL, PAGE_EXECUTE_READ, SEC_COMMIT, file ))
    {
        NtMapViewOfSection( section, NtCurrentProcess(), &Wow64Transition, 0,
                            0, NULL, &size, ViewShare, 0, PAGE_EXECUTE_READ );
        NtClose( section );
    }
    NtClose( file );
}

static void init_wow64( CONTEXT *context )
{
    PEB *peb = NtCurrentTeb()->Peb;
    PEB64 *peb64 = UlongToPtr( NtCurrentTeb64()->Peb );

    if (Wow64Transition) return;  /* already initialized */

    peb64->OSMajorVersion   = peb->OSMajorVersion;
    peb64->OSMinorVersion   = peb->OSMinorVersion;
    peb64->OSBuildNumber    = peb->OSBuildNumber;
    peb64->OSPlatformId     = peb->OSPlatformId;

#define SET_INIT_BLOCK(func) LdrSystemDllInitBlock.p ## func = PtrToUlong( &func )
    SET_INIT_BLOCK( KiUserApcDispatcher );
    SET_INIT_BLOCK( KiUserExceptionDispatcher );
    SET_INIT_BLOCK( LdrInitializeThunk );
    SET_INIT_BLOCK( LdrSystemDllInitBlock );
    SET_INIT_BLOCK( RtlUserThreadStart );
    SET_INIT_BLOCK( KiUserCallbackDispatcher );
    /* SET_INIT_BLOCK( RtlpQueryProcessDebugInformationRemote ); */
    /* SET_INIT_BLOCK( RtlpFreezeTimeBias ); */
    /* LdrSystemDllInitBlock.ntdll_handle */
#undef SET_INIT_BLOCK

    map_wow64cpu();
}
#endif


/* release some address space once dlls are loaded*/
static void release_address_space(void)
{
#ifndef _WIN64
    void *addr = (void *)1;
    SIZE_T size = 0;

    NtFreeVirtualMemory( GetCurrentProcess(), &addr, &size, MEM_RELEASE );
#endif
}

/******************************************************************
 *		loader_init
 *
 * Attach to all the loaded dlls.
 * If this is the first time, perform the full process initialization.
 */
void loader_init( CONTEXT *context, void **entry )
{
    static int attach_done;
    NTSTATUS status;
    ULONG_PTR cookie, port = 0;
    WINE_MODREF *wm;

    if (process_detaching) NtTerminateThread( GetCurrentThread(), 0 );

    RtlEnterCriticalSection( &loader_section );

    if (!imports_fixup_done)
    {
        ANSI_STRING ctrl_routine = RTL_CONSTANT_STRING( "CtrlRoutine" );
        WINE_MODREF *kernel32;
        PEB *peb = NtCurrentTeb()->Peb;
        unsigned int i;

        peb->LdrData            = &ldr;
        peb->FastPebLock        = &peb_lock;
        peb->TlsBitmap          = &tls_bitmap;
        peb->TlsExpansionBitmap = &tls_expansion_bitmap;
        peb->LoaderLock         = &loader_section;
        peb->ProcessHeap        = RtlCreateHeap( HEAP_GROWABLE, NULL, 0, 0, NULL, NULL );

        RtlInitializeBitMap( &tls_bitmap, peb->TlsBitmapBits, sizeof(peb->TlsBitmapBits) * 8 );
        RtlInitializeBitMap( &tls_expansion_bitmap, peb->TlsExpansionBitmapBits,
                             sizeof(peb->TlsExpansionBitmapBits) * 8 );
        /* TLS index 0 is always reserved, and wow64 reserves extra TLS entries */
        RtlSetBits( peb->TlsBitmap, 0, NtCurrentTeb()->WowTebOffset ? WOW64_TLS_MAX_NUMBER : 1 );
        RtlSetBits( peb->TlsBitmap, NTDLL_TLS_ERRNO, 1 );

        if (!(tls_dirs = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, tls_module_count * sizeof(*tls_dirs) )))
            NtTerminateProcess( GetCurrentProcess(), STATUS_NO_MEMORY );

        for (i = 0; i < HASH_MAP_SIZE; i++)
            InitializeListHead( &hash_table[i] );

        init_user_process_params();
        load_global_options();
        version_init();

        default_load_path = peb->ProcessParameters->DllPath.Buffer;
        if (!default_load_path)
            get_dll_load_path( peb->ProcessParameters->ImagePathName.Buffer, NULL, dll_safe_mode, &default_load_path );

        if (NtCurrentTeb()->WowTebOffset) init_wow64( context );

        wm = build_main_module();
        build_ntdll_module();
#ifdef __arm64ec__
        load_arm64ec_module();
        update_load_config( wm->ldr.DllBase );
#endif

        if ((status = load_dll( NULL, L"kernel32.dll", 0, &kernel32, FALSE )) != STATUS_SUCCESS)
        {
            MESSAGE( "wine: could not load kernel32.dll, status %lx\n", status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }
        node_kernel32 = kernel32->ldr.DdagNode;
        pBaseThreadInitThunk = RtlFindExportedRoutineByName( kernel32->ldr.DllBase, "BaseThreadInitThunk" );
        LdrGetProcedureAddress( kernel32->ldr.DllBase, &ctrl_routine, 0, (void **)&pCtrlRoutine );

        actctx_init();
        locale_init();
        if (needs_elevation())
            elevate_token();
        get_env_var( L"WINESYSTEMDLLPATH", 0, &system_dll_path );
        if (wm->ldr.Flags & LDR_COR_ILONLY)
            status = fixup_imports_ilonly( wm, NULL, entry );
        else
            status = fixup_imports( wm, NULL );

        if (status)
        {
            ERR( "Importing dlls for %s failed, status %lx\n",
                 debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer), status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }
        imports_fixup_done = TRUE;
    }
    else
    {
#ifdef _WIN64
        if (NtCurrentTeb()->WowTebOffset) init_wow64( context );
#endif
#ifdef __arm64ec__
        arm64ec_thread_init();
#endif
        wm = get_modref( NtCurrentTeb()->Peb->ImageBaseAddress );
    }

    NtCurrentTeb()->FlsSlots = fls_alloc_data();

    if (!attach_done)  /* first time around */
    {
        attach_done = 1;
        if ((status = alloc_thread_tls()) != STATUS_SUCCESS)
        {
            ERR( "TLS init  failed when loading %s, status %lx\n",
                 debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer), status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }
        wm->ldr.Flags |= LDR_PROCESS_ATTACHED;  /* don't try to attach again */
        if (wm->ldr.ActivationContext)
            RtlActivateActivationContext( 0, wm->ldr.ActivationContext, &cookie );

        if ((status = process_attach( node_ntdll, context ))
             || (status = process_attach( node_kernel32, context )))
        {
            ERR( "Initializing system dll for %s failed, status %lx\n",
                 debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer), status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }

        if ((status = walk_node_dependencies( wm->ldr.DdagNode, context, process_attach )))
        {
            if (last_failed_modref)
                ERR( "%s failed to initialize, aborting\n",
                     debugstr_w(last_failed_modref->ldr.BaseDllName.Buffer) + 1 );
            ERR( "Initializing dlls for %s failed, status %lx\n",
                 debugstr_w(NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer), status );
            NtTerminateProcess( GetCurrentProcess(), status );
        }
        release_address_space();
        if (wm->ldr.TlsIndex == -1) call_tls_callbacks( wm->ldr.DllBase, DLL_PROCESS_ATTACH );
        if (wm->ldr.ActivationContext) RtlDeactivateActivationContext( 0, cookie );

        NtQueryInformationProcess( GetCurrentProcess(), ProcessDebugPort, &port, sizeof(port), NULL );
        if (port) process_breakpoint();
    }
    else
    {
        if ((status = alloc_thread_tls()) != STATUS_SUCCESS)
            NtTerminateThread( GetCurrentThread(), status );
        thread_attach();
        if (wm->ldr.TlsIndex == -1) call_tls_callbacks( wm->ldr.DllBase, DLL_THREAD_ATTACH );
    }

    RtlLeaveCriticalSection( &loader_section );
}


/***********************************************************************
 *           RtlImageDirectoryEntryToData   (NTDLL.@)
 */
PVOID WINAPI RtlImageDirectoryEntryToData( HMODULE module, BOOL image, WORD dir, ULONG *size )
{
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    if ((ULONG_PTR)module & 1) image = FALSE;  /* mapped as data file */
    module = (HMODULE)((ULONG_PTR)module & ~3);
    if (!(nt = RtlImageNtHeader( module ))) return NULL;
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64 *nt64 = (const IMAGE_NT_HEADERS64 *)nt;

        if (dir >= nt64->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt64->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt64->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt64->OptionalHeader.SizeOfHeaders) return (char *)module + addr;
    }
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        const IMAGE_NT_HEADERS32 *nt32 = (const IMAGE_NT_HEADERS32 *)nt;

        if (dir >= nt32->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt32->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt32->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt32->OptionalHeader.SizeOfHeaders) return (char *)module + addr;
    }
    else return NULL;

    /* not mapped as image, need to find the section containing the virtual address */
    return RtlImageRvaToVa( nt, module, addr, NULL );
}


/***********************************************************************
 *           RtlImageRvaToSection   (NTDLL.@)
 */
PIMAGE_SECTION_HEADER WINAPI RtlImageRvaToSection( const IMAGE_NT_HEADERS *nt,
                                                   HMODULE module, DWORD rva )
{
    int i;
    const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION( nt );

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            return (PIMAGE_SECTION_HEADER)sec;
    }
    return NULL;
}


/***********************************************************************
 *           RtlImageRvaToVa   (NTDLL.@)
 */
PVOID WINAPI RtlImageRvaToVa( const IMAGE_NT_HEADERS *nt, HMODULE module,
                              DWORD rva, IMAGE_SECTION_HEADER **section )
{
    IMAGE_SECTION_HEADER *sec;

    if (section && *section)  /* try this section first */
    {
        sec = *section;
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            goto found;
    }
    if (!(sec = RtlImageRvaToSection( nt, module, rva ))) return NULL;
 found:
    if (section) *section = sec;
    return (char *)module + sec->PointerToRawData + (rva - sec->VirtualAddress);
}


/***********************************************************************
 *           RtlAddressInSectionTable   (NTDLL.@)
 */
PVOID WINAPI RtlAddressInSectionTable( const IMAGE_NT_HEADERS *nt, HMODULE module,
                                       DWORD rva )
{
    return RtlImageRvaToVa( nt, module, rva, NULL );
}

/***********************************************************************
 *           RtlPcToFileHeader   (NTDLL.@)
 */
PVOID WINAPI RtlPcToFileHeader( PVOID pc, PVOID *address )
{
    LDR_DATA_TABLE_ENTRY *module;
    PVOID ret = NULL;

    RtlEnterCriticalSection( &loader_section );
    if (!LdrFindEntryForAddress( pc, &module )) ret = module->DllBase;
    RtlLeaveCriticalSection( &loader_section );
    *address = ret;
    return ret;
}


/****************************************************************************
 *		LdrGetDllDirectory  (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllDirectory( UNICODE_STRING *dir )
{
    NTSTATUS status = STATUS_SUCCESS;

    RtlEnterCriticalSection( &dlldir_section );
    dir->Length = dll_directory.Length + sizeof(WCHAR);
    if (dir->MaximumLength >= dir->Length) RtlCopyUnicodeString( dir, &dll_directory );
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
        if (dir->MaximumLength) dir->Buffer[0] = 0;
    }
    RtlLeaveCriticalSection( &dlldir_section );
    return status;
}


/****************************************************************************
 *		LdrSetDllDirectory  (NTDLL.@)
 */
NTSTATUS WINAPI LdrSetDllDirectory( const UNICODE_STRING *dir )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING new;

    if (!dir->Buffer) RtlInitUnicodeString( &new, NULL );
    else if ((status = RtlDuplicateUnicodeString( 1, dir, &new ))) return status;

    RtlEnterCriticalSection( &dlldir_section );
    RtlFreeUnicodeString( &dll_directory );
    dll_directory = new;
    RtlLeaveCriticalSection( &dlldir_section );
    return status;
}


/****************************************************************************
 *		LdrAddDllDirectory  (NTDLL.@)
 */
NTSTATUS WINAPI LdrAddDllDirectory( const UNICODE_STRING *dir, void **cookie )
{
    FILE_BASIC_INFORMATION info;
    UNICODE_STRING nt_name;
    NTSTATUS status;
    OBJECT_ATTRIBUTES attr;
    DWORD len;
    struct dll_dir_entry *ptr;
    DOS_PATHNAME_TYPE type = RtlDetermineDosPathNameType_U( dir->Buffer );

    if (type != ABSOLUTE_PATH && type != ABSOLUTE_DRIVE_PATH && type != UNC_PATH)
        return STATUS_INVALID_PARAMETER;

    status = RtlDosPathNameToNtPathName_U_WithStatus( dir->Buffer, &nt_name, NULL, NULL );
    if (status) return status;
    len = nt_name.Length / sizeof(WCHAR);
    if (!(ptr = RtlAllocateHeap( GetProcessHeap(), 0, offsetof(struct dll_dir_entry, dir[++len] ))))
        return STATUS_NO_MEMORY;
    memcpy( ptr->dir, nt_name.Buffer, len * sizeof(WCHAR) );

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &nt_name;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    status = NtQueryAttributesFile( &attr, &info );
    RtlFreeUnicodeString( &nt_name );

    if (!status)
    {
        TRACE( "%s\n", debugstr_w( ptr->dir ));
        RtlEnterCriticalSection( &dlldir_section );
        list_add_head( &dll_dir_list, &ptr->entry );
        RtlLeaveCriticalSection( &dlldir_section );
        *cookie = ptr;
    }
    else RtlFreeHeap( GetProcessHeap(), 0, ptr );
    return status;
}


/****************************************************************************
 *		LdrRemoveDllDirectory  (NTDLL.@)
 */
NTSTATUS WINAPI LdrRemoveDllDirectory( void *cookie )
{
    struct dll_dir_entry *ptr = cookie;

    TRACE( "%s\n", debugstr_w( ptr->dir ));

    RtlEnterCriticalSection( &dlldir_section );
    list_remove( &ptr->entry );
    RtlFreeHeap( GetProcessHeap(), 0, ptr );
    RtlLeaveCriticalSection( &dlldir_section );
    return STATUS_SUCCESS;
}


/*************************************************************************
 *		LdrSetDefaultDllDirectories  (NTDLL.@)
 */
NTSTATUS WINAPI LdrSetDefaultDllDirectories( ULONG flags )
{
    /* LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR doesn't make sense in default dirs */
    const ULONG load_library_search_flags = (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    if (!flags || (flags & ~load_library_search_flags)) return STATUS_INVALID_PARAMETER;
    default_search_flags = flags;
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrGetDllPath  (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllPath( PCWSTR module, ULONG flags, PWSTR *path, PWSTR *unknown )
{
    NTSTATUS status;
    const ULONG load_library_search_flags = (LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
                                             LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

    if (flags & LOAD_WITH_ALTERED_SEARCH_PATH)
    {
        if (flags & load_library_search_flags) return STATUS_INVALID_PARAMETER;
        if (default_search_flags) flags |= default_search_flags | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR;
    }
    else if (!(flags & load_library_search_flags)) flags |= default_search_flags;

    RtlEnterCriticalSection( &dlldir_section );

    if (flags & load_library_search_flags)
    {
        status = get_dll_load_path_search_flags( module, flags, path );
    }
    else
    {
        const WCHAR *dlldir = dll_directory.Length ? dll_directory.Buffer : NULL;
        if (!(flags & LOAD_WITH_ALTERED_SEARCH_PATH) || !wcschr( module, L'\\' ))
            module = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
        status = get_dll_load_path( module, dlldir, dll_safe_mode, path );
    }

    RtlLeaveCriticalSection( &dlldir_section );
    *unknown = NULL;
    return status;
}


/*************************************************************************
 *		RtlSetSearchPathMode (NTDLL.@)
 */
NTSTATUS WINAPI RtlSetSearchPathMode( ULONG flags )
{
    int val;

    switch (flags)
    {
    case BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE:
        val = 1;
        break;
    case BASE_SEARCH_PATH_DISABLE_SAFE_SEARCHMODE:
        val = 0;
        break;
    case BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE | BASE_SEARCH_PATH_PERMANENT:
        InterlockedExchange( &path_safe_mode, 2 );
        return STATUS_SUCCESS;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    for (;;)
    {
        LONG prev = path_safe_mode;
        if (prev == 2) break;  /* permanently set */
        if (InterlockedCompareExchange( &path_safe_mode, val, prev ) == prev) return STATUS_SUCCESS;
    }
    return STATUS_ACCESS_DENIED;
}


/******************************************************************
 *           RtlGetExePath   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetExePath( PCWSTR name, PWSTR *path )
{
    const WCHAR *dlldir = L".";
    const WCHAR *module = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;

    /* same check as NeedCurrentDirectoryForExePathW */
    if (!wcschr( name, '\\' ))
    {
        UNICODE_STRING name = RTL_CONSTANT_STRING( L"NoDefaultCurrentDirectoryInExePath" ), value = { 0 };

        if (RtlQueryEnvironmentVariable_U( NULL, &name, &value ) != STATUS_VARIABLE_NOT_FOUND)
            dlldir = L"";
    }
    return get_dll_load_path( module, dlldir, FALSE, path );
}


/******************************************************************
 *           RtlGetSearchPath   (NTDLL.@)
 */
NTSTATUS WINAPI RtlGetSearchPath( PWSTR *path )
{
    const WCHAR *module = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    return get_dll_load_path( module, NULL, path_safe_mode, path );
}


/******************************************************************
 *           RtlReleasePath   (NTDLL.@)
 */
void WINAPI RtlReleasePath( PWSTR path )
{
    RtlFreeHeap( GetProcessHeap(), 0, path );
}


/*********************************************************************
 *           ApiSetQueryApiSetPresence   (NTDLL.@)
 */
NTSTATUS WINAPI ApiSetQueryApiSetPresence( const UNICODE_STRING *name, BOOLEAN *present )
{
    const API_SET_NAMESPACE *map = NtCurrentTeb()->Peb->ApiSetMap;
    const API_SET_NAMESPACE_ENTRY *entry;
    UNICODE_STRING str;

    *present = (!get_apiset_entry( map, name->Buffer, name->Length / sizeof(WCHAR), &entry ) &&
                !get_apiset_target( map, entry, NULL, &str ));
    return STATUS_SUCCESS;
}


/*********************************************************************
 *           ApiSetQueryApiSetPresenceEx   (NTDLL.@)
 */
NTSTATUS WINAPI ApiSetQueryApiSetPresenceEx( const UNICODE_STRING *name, BOOLEAN *in_schema, BOOLEAN *present )
{
    const API_SET_NAMESPACE *map = NtCurrentTeb()->Peb->ApiSetMap;
    const API_SET_NAMESPACE_ENTRY *entry;
    NTSTATUS status;
    UNICODE_STRING str;
    ULONG i, len = name->Length / sizeof(WCHAR);

    /* extension not allowed */
    for (i = 0; i < len; i++) if (name->Buffer[i] == '.') return STATUS_INVALID_PARAMETER;

    status = get_apiset_entry( map, name->Buffer, len, &entry );
    if (status == STATUS_APISET_NOT_PRESENT)
    {
        *in_schema = *present = FALSE;
        return STATUS_SUCCESS;
    }
    if (status) return status;

    /* the name must match exactly */
    *in_schema = (entry->NameLength == name->Length &&
                  !wcsnicmp( (WCHAR *)((char *)map + entry->NameOffset), name->Buffer, len ));
    *present = *in_schema && !get_apiset_target( map, entry, NULL, &str );
    return STATUS_SUCCESS;
}


/******************************************************************
 *		DllMain   (NTDLL.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}
 