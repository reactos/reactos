/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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

#ifndef __MSCOREE_PRIVATE__
#define __MSCOREE_PRIVATE__

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <objbase.h>
#include <cor.h>
#include <cordebug.h>
#include <metahost.h>

#include <wine/list.h>
#include <wine/unicode.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

extern char *WtoA(LPCWSTR wstr) DECLSPEC_HIDDEN;

extern HRESULT CLRMetaHost_CreateInstance(REFIID riid, void **ppobj) DECLSPEC_HIDDEN;
extern HRESULT CLRMetaHostPolicy_CreateInstance(REFIID riid, void **ppobj) DECLSPEC_HIDDEN;

extern HRESULT WINAPI CLRMetaHost_GetVersionFromFile(ICLRMetaHost* iface,
    LPCWSTR pwzFilePath, LPWSTR pwzBuffer, DWORD *pcchBuffer) DECLSPEC_HIDDEN;

typedef struct _VTableFixup {
    DWORD rva;
    WORD count;
    WORD type;
} VTableFixup;

typedef struct tagASSEMBLY ASSEMBLY;

typedef BOOL (WINAPI *NativeEntryPointFunc)(HINSTANCE, DWORD, LPVOID);

extern HRESULT assembly_create(ASSEMBLY **out, LPCWSTR file) DECLSPEC_HIDDEN;
extern HRESULT assembly_from_hmodule(ASSEMBLY **out, HMODULE hmodule) DECLSPEC_HIDDEN;
extern HRESULT assembly_release(ASSEMBLY *assembly) DECLSPEC_HIDDEN;
extern HRESULT assembly_get_runtime_version(ASSEMBLY *assembly, LPSTR *version) DECLSPEC_HIDDEN;
extern HRESULT assembly_get_vtable_fixups(ASSEMBLY *assembly, VTableFixup **fixups, DWORD *count) DECLSPEC_HIDDEN;
extern HRESULT assembly_get_native_entrypoint(ASSEMBLY *assembly, NativeEntryPointFunc *func) DECLSPEC_HIDDEN;

#define WINE_MONO_VERSION "7.4.0"

/* Mono embedding */
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoAssemblyName MonoAssemblyName;
typedef struct _MonoType MonoType;
typedef struct _MonoImage MonoImage;
typedef struct _MonoClass MonoClass;
typedef struct _MonoObject MonoObject;
typedef struct _MonoString MonoString;
typedef struct _MonoMethod MonoMethod;
typedef struct _MonoProfiler MonoProfiler;
typedef struct _MonoThread MonoThread;

typedef struct RuntimeHost RuntimeHost;

typedef struct CLRRuntimeInfo
{
    ICLRRuntimeInfo ICLRRuntimeInfo_iface;
    DWORD major;
    DWORD minor;
    DWORD build;
    struct RuntimeHost *loaded_runtime;
} CLRRuntimeInfo;

struct RuntimeHost
{
    ICorRuntimeHost ICorRuntimeHost_iface;
    ICLRRuntimeHost ICLRRuntimeHost_iface;
    CLRRuntimeInfo *version;
    CRITICAL_SECTION lock;
    LONG ref;
};

typedef struct CorProcess
{
    struct list entry;
    ICorDebugProcess *pProcess;
} CorProcess;

typedef struct CorDebug
{
    ICorDebug ICorDebug_iface;
    ICorDebugProcessEnum ICorDebugProcessEnum_iface;
    LONG ref;

    ICLRRuntimeHost *runtimehost;

    /* ICorDebug Callback */
    ICorDebugManagedCallback *pCallback;
    ICorDebugManagedCallback2 *pCallback2;

    /* Debug Processes */
    struct list processes;
} CorDebug;

extern HRESULT get_runtime_info(LPCWSTR exefile, LPCWSTR version, LPCWSTR config_file,
    IStream *config_stream, DWORD startup_flags, DWORD runtimeinfo_flags, BOOL legacy,
    ICLRRuntimeInfo **result) DECLSPEC_HIDDEN;

extern BOOL get_mono_path(LPWSTR path, BOOL skip_local) DECLSPEC_HIDDEN;

extern MonoDomain* get_root_domain(void);

extern HRESULT ICLRRuntimeInfo_GetRuntimeHost(ICLRRuntimeInfo *iface, RuntimeHost **result) DECLSPEC_HIDDEN;

extern HRESULT MetaDataDispenser_CreateInstance(IUnknown **ppUnk) DECLSPEC_HIDDEN;

typedef struct parsed_config_file
{
    struct list supported_runtimes;
    LPWSTR private_path;
} parsed_config_file;

typedef struct supported_runtime
{
    struct list entry;
    LPWSTR version;
} supported_runtime;

extern WCHAR **private_path;

extern HRESULT parse_config_file(LPCWSTR filename, parsed_config_file *result) DECLSPEC_HIDDEN;

extern HRESULT parse_config_stream(IStream *stream, parsed_config_file *result) DECLSPEC_HIDDEN;

extern void free_parsed_config_file(parsed_config_file *file) DECLSPEC_HIDDEN;

typedef enum {
	MONO_IMAGE_OK,
	MONO_IMAGE_ERROR_ERRNO,
	MONO_IMAGE_MISSING_ASSEMBLYREF,
	MONO_IMAGE_IMAGE_INVALID
} MonoImageOpenStatus;

typedef MonoAssembly* (CDECL *MonoAssemblyPreLoadFunc)(MonoAssemblyName *aname, char **assemblies_path, void *user_data);

typedef MonoAssembly* (CDECL *WineMonoAssemblyPreLoadFunc)(MonoAssemblyName *aname, char **assemblies_path, int *halt_search, void *user_data);

typedef void (CDECL *MonoProfileFunc)(MonoProfiler *prof);

typedef void (CDECL *MonoPrintCallback) (const char *string, INT is_stdout);

typedef enum {
    MONO_AOT_MODE_NONE,
    MONO_AOT_MODE_NORMAL,
    MONO_AOT_MODE_HYBRID,
    MONO_AOT_MODE_FULL,
    MONO_AOT_MODE_LLVMONLY,
    MONO_AOT_MODE_INTERP,
    MONO_AOT_MODE_INTERP_LLVMONLY,
    MONO_AOT_MODE_LLVMONLY_INTERP,
    MONO_AOT_MODE_INTERP_ONLY
} MonoAotMode;

extern BOOL is_mono_started DECLSPEC_HIDDEN;

extern MonoImage* (CDECL *mono_assembly_get_image)(MonoAssembly *assembly) DECLSPEC_HIDDEN;
extern MonoAssembly* (CDECL *mono_assembly_load_from)(MonoImage *image, const char *fname, MonoImageOpenStatus *status) DECLSPEC_HIDDEN;
extern const char* (CDECL *mono_assembly_name_get_name)(MonoAssemblyName *aname) DECLSPEC_HIDDEN;
extern MonoAssembly* (CDECL *mono_assembly_open)(const char *filename, MonoImageOpenStatus *status) DECLSPEC_HIDDEN;
extern void (CDECL *mono_callspec_set_assembly)(MonoAssembly *assembly) DECLSPEC_HIDDEN;
extern MonoClass* (CDECL *mono_class_from_mono_type)(MonoType *type) DECLSPEC_HIDDEN;
extern MonoClass* (CDECL *mono_class_from_name)(MonoImage *image, const char* name_space, const char *name) DECLSPEC_HIDDEN;
extern MonoMethod* (CDECL *mono_class_get_method_from_name)(MonoClass *klass, const char *name, int param_count) DECLSPEC_HIDDEN;
extern MonoDomain* (CDECL *mono_domain_get)(void) DECLSPEC_HIDDEN;
extern MonoDomain* (CDECL *mono_domain_get_by_id)(int id) DECLSPEC_HIDDEN;
extern BOOL (CDECL *mono_domain_set)(MonoDomain *domain, BOOL force) DECLSPEC_HIDDEN;
extern void (CDECL *mono_domain_set_config)(MonoDomain *domain,const char *base_dir,const char *config_file_name) DECLSPEC_HIDDEN;
extern MonoImage* (CDECL *mono_get_corlib)(void) DECLSPEC_HIDDEN;
extern int (CDECL *mono_jit_exec)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]) DECLSPEC_HIDDEN;
extern MonoDomain* (CDECL *mono_jit_init_version)(const char *domain_name, const char *runtime_version) DECLSPEC_HIDDEN;
extern MonoImage* (CDECL *mono_image_open_from_module_handle)(HMODULE module_handle, char* fname, UINT has_entry_point, MonoImageOpenStatus* status) DECLSPEC_HIDDEN;
extern void* (CDECL *mono_marshal_get_vtfixup_ftnptr)(MonoImage *image, DWORD token, WORD type) DECLSPEC_HIDDEN;
extern MonoDomain* (CDECL *mono_object_get_domain)(MonoObject *obj) DECLSPEC_HIDDEN;
extern MonoMethod* (CDECL *mono_object_get_virtual_method)(MonoObject *obj, MonoMethod *method) DECLSPEC_HIDDEN;
extern MonoObject* (CDECL *mono_object_new)(MonoDomain *domain, MonoClass *klass) DECLSPEC_HIDDEN;
extern void* (CDECL *mono_object_unbox)(MonoObject *obj) DECLSPEC_HIDDEN;
extern MonoType* (CDECL *mono_reflection_type_from_name)(char *name, MonoImage *image) DECLSPEC_HIDDEN;
extern MonoObject* (CDECL *mono_runtime_invoke)(MonoMethod *method, void *obj, void **params, MonoObject **exc) DECLSPEC_HIDDEN;
extern void (CDECL *mono_runtime_object_init)(MonoObject *this_obj) DECLSPEC_HIDDEN;
extern void (CDECL *mono_runtime_quit)(void) DECLSPEC_HIDDEN;
extern MonoString* (CDECL *mono_string_new)(MonoDomain *domain, const char *str) DECLSPEC_HIDDEN;
extern MonoThread* (CDECL *mono_thread_attach)(MonoDomain *domain) DECLSPEC_HIDDEN;
extern void (CDECL *mono_thread_manage)(void) DECLSPEC_HIDDEN;
extern void (CDECL *mono_trace_set_print_handler)(MonoPrintCallback callback) DECLSPEC_HIDDEN;
extern void (CDECL *mono_trace_set_printerr_handler)(MonoPrintCallback callback) DECLSPEC_HIDDEN;

/* loaded runtime interfaces */
extern void expect_no_runtimes(void) DECLSPEC_HIDDEN;

extern HRESULT RuntimeHost_Construct(CLRRuntimeInfo *runtime_version, RuntimeHost** result) DECLSPEC_HIDDEN;

extern void RuntimeHost_ExitProcess(RuntimeHost *This, INT exitcode) DECLSPEC_HIDDEN;

extern HRESULT RuntimeHost_GetInterface(RuntimeHost *This, REFCLSID clsid, REFIID riid, void **ppv) DECLSPEC_HIDDEN;

extern HRESULT RuntimeHost_GetIUnknownForObject(RuntimeHost *This, MonoObject *obj, IUnknown **ppUnk) DECLSPEC_HIDDEN;

extern HRESULT RuntimeHost_CreateManagedInstance(RuntimeHost *This, LPCWSTR name,
    MonoDomain *domain, MonoObject **result) DECLSPEC_HIDDEN;

HRESULT WINAPI CLRMetaHost_ExitProcess(ICLRMetaHost* iface, INT32 iExitCode) DECLSPEC_HIDDEN;

HRESULT WINAPI CLRMetaHost_GetRuntime(ICLRMetaHost* iface, LPCWSTR pwzVersion, REFIID iid, LPVOID *ppRuntime) DECLSPEC_HIDDEN;

extern HRESULT CorDebug_Create(ICLRRuntimeHost *runtimehost, IUnknown** ppUnk) DECLSPEC_HIDDEN;

extern HRESULT create_monodata(REFCLSID clsid, LPVOID *ppObj) DECLSPEC_HIDDEN;

extern HRESULT get_file_from_strongname(WCHAR* stringnameW, WCHAR* assemblies_path, int path_length) DECLSPEC_HIDDEN;

extern void runtimehost_init(void) DECLSPEC_HIDDEN;
extern void runtimehost_uninit(void) DECLSPEC_HIDDEN;

#ifdef __REACTOS__
#define HOST_E_INVALIDOPERATION EMAKEHR(0x1022)
#endif

#endif   /* __MSCOREE_PRIVATE__ */
