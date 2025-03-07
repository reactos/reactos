//
// winapi_thunks.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definitions of wrappers for Windows API functions that cannot be called
// directly because they are not available on all supported operating systems.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>
#include <corecrt_internal.h>
#include <appmodel.h>
#include <roapi.h>

// This is simlar to msvcrt.
#if _M_AMD64 || _M_ARM || _M_ARM64 || _M_HYBRID
#define FLS_ALWAYS_AVAILABLE 1
#endif

 WINBASEAPI
 _Success_(return > 0 && return < BufferLength)
 DWORD
 WINAPI
 GetTempPath2W(
     _In_ DWORD BufferLength,
     _Out_writes_to_opt_(BufferLength,return + 1) LPWSTR Buffer
     );

// The XState APIs are declared by the Windows headers only when building for
// x86 and x64.  We declare them here unconditionally so that we can share the
// same code for all architectures (we simply avoid use of these functions on
// other architectures).
extern "C" WINBASEAPI DWORD64 WINAPI GetEnabledXStateFeatures();

_Must_inspect_result_
extern "C" WINBASEAPI BOOL WINAPI GetXStateFeaturesMask(
    _In_  PCONTEXT context,
    _Out_ PDWORD64 feature_mask
    );

_Success_(return != NULL)
extern "C" WINBASEAPI PVOID WINAPI LocateXStateFeature(
    _In_      PCONTEXT context,
    _In_      DWORD    feature_id,
    _Out_opt_ PDWORD   length
    );

#define _ACRT_APPLY_TO_LATE_BOUND_MODULES_0                                                              \
    _APPLY(api_ms_win_core_datetime_l1_1_1,              "api-ms-win-core-datetime-l1-1-1"             ) \
    _APPLY(api_ms_win_core_file_l1_2_4,                  "api-ms-win-core-file-l1-2-4"                 ) \
    _APPLY(api_ms_win_core_file_l1_2_2,                  "api-ms-win-core-file-l1-2-2"                 ) \
    _APPLY(api_ms_win_core_localization_l1_2_1,          "api-ms-win-core-localization-l1-2-1"         ) \
    _APPLY(api_ms_win_core_localization_obsolete_l1_2_0, "api-ms-win-core-localization-obsolete-l1-2-0") \
    _APPLY(api_ms_win_core_processthreads_l1_1_2,        "api-ms-win-core-processthreads-l1-1-2"       ) \
    _APPLY(api_ms_win_core_string_l1_1_0,                "api-ms-win-core-string-l1-1-0"               ) \
    _APPLY(api_ms_win_core_synch_l1_2_0,                 "api-ms-win-core-synch-l1-2-0"                ) \
    _APPLY(api_ms_win_core_sysinfo_l1_2_1,               "api-ms-win-core-sysinfo-l1-2-1"              ) \
    _APPLY(api_ms_win_core_winrt_l1_1_0,                 "api-ms-win-core-winrt-l1-1-0"                ) \
    _APPLY(api_ms_win_core_xstate_l2_1_0,                "api-ms-win-core-xstate-l2-1-0"               ) \
    _APPLY(api_ms_win_rtcore_ntuser_window_l1_1_0,       "api-ms-win-rtcore-ntuser-window-l1-1-0"      ) \
    _APPLY(api_ms_win_security_systemfunctions_l1_1_0,   "api-ms-win-security-systemfunctions-l1-1-0"  ) \
    _APPLY(ext_ms_win_ntuser_dialogbox_l1_1_0,           "ext-ms-win-ntuser-dialogbox-l1-1-0"          ) \
    _APPLY(ext_ms_win_ntuser_windowstation_l1_1_0,       "ext-ms-win-ntuser-windowstation-l1-1-0"      ) \
    _APPLY(advapi32,                                     "advapi32"                                    ) \
    _APPLY(kernel32,                                     "kernel32"                                    ) \
    _APPLY(kernelbase,                                   "kernelbase"                                  ) \
    _APPLY(ntdll,                                        "ntdll"                                       ) \
    _APPLY(api_ms_win_appmodel_runtime_l1_1_2,           "api-ms-win-appmodel-runtime-l1-1-2"          ) \
    _APPLY(user32,                                       "user32"                                      )

#if FLS_ALWAYS_AVAILABLE

#define _ACRT_APPLY_TO_LATE_BOUND_MODULES_1 /* nothing */

#else

#define _ACRT_APPLY_TO_LATE_BOUND_MODULES_1                                                              \
    _APPLY(api_ms_win_core_fibers_l1_1_0,                "api-ms-win-core-fibers-l1-1-0"               )

#endif

#define _ACRT_APPLY_TO_LATE_BOUND_MODULES  \
    _ACRT_APPLY_TO_LATE_BOUND_MODULES_0 \
    _ACRT_APPLY_TO_LATE_BOUND_MODULES_1 \

#define _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS_0                                                                                                           \
    _APPLY(AreFileApisANSI,                             ({ /* api_ms_win_core_file_l1_2_2, */            kernel32                                   })) \
    _APPLY(CompareStringEx,                             ({ api_ms_win_core_string_l1_1_0,                kernel32                                   })) \
    _APPLY(EnumSystemLocalesEx,                         ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(GetActiveWindow,                             ({ api_ms_win_rtcore_ntuser_window_l1_1_0,       user32                                     })) \
    _APPLY(GetDateFormatEx,                             ({ api_ms_win_core_datetime_l1_1_1,              kernel32                                   })) \
    _APPLY(GetTempPath2W,                               ({ api_ms_win_core_file_l1_2_4,                  kernelbase                                 })) \
    _APPLY(GetEnabledXStateFeatures,                    ({ api_ms_win_core_xstate_l2_1_0,                kernel32                                   })) \
    _APPLY(GetLastActivePopup,                          ({ ext_ms_win_ntuser_dialogbox_l1_1_0,           user32                                     })) \
    _APPLY(GetLocaleInfoEx,                             ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(GetProcessWindowStation,                     ({ ext_ms_win_ntuser_windowstation_l1_1_0,       user32                                     })) \
    _APPLY(GetSystemTimePreciseAsFileTime,              ({ api_ms_win_core_sysinfo_l1_2_1                                                           })) \
    _APPLY(GetTimeFormatEx,                             ({ api_ms_win_core_datetime_l1_1_1,              kernel32                                   })) \
    _APPLY(GetUserDefaultLocaleName,                    ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(GetUserObjectInformationW,                   ({ ext_ms_win_ntuser_windowstation_l1_1_0,       user32                                     })) \
    _APPLY(GetXStateFeaturesMask,                       ({ api_ms_win_core_xstate_l2_1_0,                kernel32                                   })) \
    _APPLY(InitializeCriticalSectionEx,                 ({ api_ms_win_core_synch_l1_2_0,                 kernel32                                   })) \
    _APPLY(IsValidLocaleName,                           ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(LCMapStringEx,                               ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(LCIDToLocaleName,                            ({ api_ms_win_core_localization_obsolete_l1_2_0, kernel32                                   })) \
    _APPLY(LocaleNameToLCID,                            ({ api_ms_win_core_localization_l1_2_1,          kernel32                                   })) \
    _APPLY(LocateXStateFeature,                         ({ api_ms_win_core_xstate_l2_1_0,                kernel32                                   })) \
    _APPLY(MessageBoxA,                                 ({ ext_ms_win_ntuser_dialogbox_l1_1_0,           user32                                     })) \
    _APPLY(MessageBoxW,                                 ({ ext_ms_win_ntuser_dialogbox_l1_1_0,           user32                                     })) \
    _APPLY(RoInitialize,                                ({ api_ms_win_core_winrt_l1_1_0                                                             })) \
    _APPLY(RoUninitialize,                              ({ api_ms_win_core_winrt_l1_1_0                                                             })) \
    _APPLY(AppPolicyGetProcessTerminationMethod,        ({ api_ms_win_appmodel_runtime_l1_1_2                                                       })) \
    _APPLY(AppPolicyGetThreadInitializationType,        ({ api_ms_win_appmodel_runtime_l1_1_2                                                       })) \
    _APPLY(AppPolicyGetShowDeveloperDiagnostic,         ({ api_ms_win_appmodel_runtime_l1_1_2                                                       })) \
    _APPLY(AppPolicyGetWindowingModel,                  ({ api_ms_win_appmodel_runtime_l1_1_2                                                       })) \
    _APPLY(SetThreadStackGuarantee,                     ({ api_ms_win_core_processthreads_l1_1_2,        kernel32                                   })) \
    _APPLY(SystemFunction036,                           ({ api_ms_win_security_systemfunctions_l1_1_0,   advapi32                                   }))

#if FLS_ALWAYS_AVAILABLE

#define _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS_1 /* nothing */

#else

#define _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS_1                                                                                                           \
    _APPLY(FlsAlloc,                                    ({ api_ms_win_core_fibers_l1_1_0,                kernel32                                   })) \
    _APPLY(FlsFree,                                     ({ api_ms_win_core_fibers_l1_1_0,                kernel32                                   })) \
    _APPLY(FlsGetValue,                                 ({ api_ms_win_core_fibers_l1_1_0,                kernel32                                   })) \
    _APPLY(FlsSetValue,                                 ({ api_ms_win_core_fibers_l1_1_0,                kernel32                                   }))

#endif

#define _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS \
    _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS_0 \
    _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS_1 \

namespace
{
    // Generate enumerators for each of the modules:
    enum module_id : unsigned
    {
        #define _APPLY(_SYMBOL, _NAME) _SYMBOL,
        _ACRT_APPLY_TO_LATE_BOUND_MODULES
        #undef _APPLY

        module_id_count
    };

    // Generate a table of module names that can be indexed by the module_id
    // enumerators:
    static wchar_t const* const module_names[module_id_count] =
    {
        #define _APPLY(_SYMBOL, _NAME) _CRT_WIDE(_NAME),
        _ACRT_APPLY_TO_LATE_BOUND_MODULES
        #undef _APPLY
    };

    // Generate enumerators for each of the functions:
    enum function_id : unsigned
    {
        #define _APPLY(_FUNCTION, _MODULES) _CRT_CONCATENATE(_FUNCTION, _id),
        _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS
        #undef _APPLY

        function_id_count
    };

    // Generate a typedef for each function of the form function_pft.
    #define _APPLY(_FUNCTION, _MODULES) \
        using _CRT_CONCATENATE(_FUNCTION, _pft) = decltype(_FUNCTION)*;
    _ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS
    #undef _APPLY
}

// This table stores the module handles that we have obtained via LoadLibrary.
// If a handle is null, we have not yet attempted to load that module.  If a
// handle is -1 (INVALID_HANDLE_VALUE), we have attempted to load the module
// but the attempt failed.
static HMODULE module_handles[module_id_count];

// This table stores the function pointers that we have loaded dynamically.  The
// function pointers are stored in encoded form via __crt_fast_encode_ponter.  If
// a function pointer is an encoded null pointer, we have not yet attempted to
// get that function pointer.  If a function pointer is an encoded -1, we have
// attempted to get that function pointer but the attempt failed.
static void* encoded_function_pointers[function_id_count];

extern "C" bool __cdecl __acrt_initialize_winapi_thunks()
{
    void* const encoded_nullptr = __crt_fast_encode_pointer(nullptr);

    for (void*& p : encoded_function_pointers)
    {
        p = encoded_nullptr;
    }

    return true;
}

extern "C" bool __cdecl __acrt_uninitialize_winapi_thunks(bool const terminating)
{
    // If the process is terminating, there's no need to free any module handles
    if (terminating)
    {
        return true;
    }

    for (HMODULE& module : module_handles)
    {
        if (module)
        {
            if (module != INVALID_HANDLE_VALUE)
            {
                FreeLibrary(module);
            }

            module = nullptr;
        }
    }

    return true;
}

static __forceinline void* __cdecl invalid_function_sentinel() throw()
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(-1));
}

static HMODULE __cdecl try_load_library_from_system_directory(wchar_t const* const name) throw()
{
    HMODULE const handle = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (handle)
    {
        return handle;
    }

    // LOAD_LIBRARY_SEARCH_SYSTEM32 is only supported by Windows 7 and above; if
    // the OS does not support this flag, try again without it.  On these OSes,
    // all APISets will be forwarders.  To prevent DLL hijacking, do not attempt
    // to load the APISet forwarders dynamically.  This will cause our caller to
    // fall back to the real DLL (e.g. kernel32).  All of those are known DLLs.
    if (GetLastError() == ERROR_INVALID_PARAMETER &&
        wcsncmp(name, L"api-ms-", 7) != 0 &&
        wcsncmp(name, L"ext-ms-", 7) != 0)
    {
        return LoadLibraryExW(name, nullptr, 0);
    }

    return nullptr;
}

static HMODULE __cdecl try_get_module(module_id const id) throw()
{
    // First check to see if we've cached the module handle:
    if (HMODULE const cached_handle = __crt_interlocked_read_pointer(module_handles + id))
    {
        if (cached_handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }

        return cached_handle;
    }

    // If we haven't yet cached the module handle, try to load the library.  If
    // this fails, cache the sentinel handle value INVALID_HANDLE_VALUE so that
    // we don't attempt to load the module again:
    HMODULE const new_handle = try_load_library_from_system_directory(module_names[id]);
    if (!new_handle)
    {
        if (HMODULE const cached_handle = __crt_interlocked_exchange_pointer(module_handles + id, INVALID_HANDLE_VALUE))
        {
            _ASSERTE(cached_handle == INVALID_HANDLE_VALUE);
        }

        return nullptr;
    }

    // Swap the new handle into the cache.  If the cache no longer contained a
    // null handle, then some other thread loaded the module and cached the
    // handle while we were doing the same.  In that case, we free the handle
    // once to maintain the reference count:
    if (HMODULE const cached_handle = __crt_interlocked_exchange_pointer(module_handles + id, new_handle))
    {
        _ASSERTE(cached_handle == new_handle);
        FreeLibrary(new_handle);
    }

    return new_handle;
}

static HMODULE __cdecl try_get_first_available_module(
    module_id const* const first,
    module_id const* const last
    ) throw()
{
    for (module_id const* it = first; it != last; ++it)
    {
        HMODULE const handle = try_get_module(*it);
        if (handle)
        {
            return handle;
        }
    }

    return nullptr;
}

static __forceinline void* __cdecl try_get_proc_address_from_first_available_module(
    char      const* const name,
    module_id const* const first_module_id,
    module_id const* const last_module_id
    ) throw()
{
    HMODULE const module_handle = try_get_first_available_module(first_module_id, last_module_id);
    if (!module_handle)
    {
        return nullptr;
    }

    return reinterpret_cast<void*>(GetProcAddress(module_handle, name));
}

static void* __cdecl try_get_function(
    function_id      const id,
    char      const* const name,
    module_id const* const first_module_id,
    module_id const* const last_module_id
    ) throw()
{
    // First check to see if we've cached the function pointer:
    {
        void* const cached_fp = __crt_fast_decode_pointer(
            __crt_interlocked_read_pointer(encoded_function_pointers + id));

        if (cached_fp == invalid_function_sentinel())
        {
            return nullptr;
        }

        if (cached_fp)
        {
            return cached_fp;
        }
    }

    // If we haven't yet cached the function pointer, try to import it from any
    // of the modules in which it might be defined.  If this fails, cache the
    // sentinel pointer so that we don't attempt to load this function again:
    void* const new_fp = try_get_proc_address_from_first_available_module(name, first_module_id, last_module_id);
    if (!new_fp)
    {
        void* const cached_fp = __crt_fast_decode_pointer(
            __crt_interlocked_exchange_pointer(
                encoded_function_pointers + id,
                __crt_fast_encode_pointer(invalid_function_sentinel())));

        if (cached_fp)
        {
            _ASSERTE(cached_fp == invalid_function_sentinel());
        }

        return nullptr;
    }

    // Swap the newly obtained function pointer into the cache.  The cache may
    // no longer contain an encoded null pointer if another thread obtained the
    // function address while we were doing the same (both threads should have
    // gotten the same function pointer):
    {
        void* const cached_fp = __crt_fast_decode_pointer(
            __crt_interlocked_exchange_pointer(
                encoded_function_pointers + id,
                __crt_fast_encode_pointer(new_fp)));

        if (cached_fp)
        {
            _ASSERTE(cached_fp == new_fp);
        }
    }

    return new_fp;
}

// Generate accessors that wrap the general try_get_function for each function,
// passing the correct set of candidate modules and returning a function pointer
// of the correct type:
#define _APPLY(_FUNCTION, _MODULES)                                                                   \
    static _CRT_CONCATENATE(_FUNCTION, _pft) __cdecl _CRT_CONCATENATE(try_get_, _FUNCTION)() throw()  \
    {                                                                                                 \
        static module_id const candidate_modules[] = _CRT_UNPARENTHESIZE(_MODULES);                   \
                                                                                                      \
        return reinterpret_cast<_CRT_CONCATENATE(_FUNCTION, _pft)>(try_get_function(                  \
            _CRT_CONCATENATE(_FUNCTION, _id),                                                         \
            _CRT_STRINGIZE(_FUNCTION),                                                                \
            candidate_modules,                                                                        \
            candidate_modules + _countof(candidate_modules)));                                        \
    }
_ACRT_APPLY_TO_LATE_BOUND_FUNCTIONS
#undef _APPLY

extern "C" BOOL WINAPI __acrt_AreFileApisANSI()
{
    if (auto const are_file_apis_ansi = try_get_AreFileApisANSI())
    {
        return are_file_apis_ansi();
    }

    // If we were unable to get the AreFileApisANSI function, we can safely
    // assume that the file APIs are, in fact, ANSI:
    return TRUE;
}

extern "C" int WINAPI __acrt_CompareStringEx(
    LPCWSTR          const locale_name,
    DWORD            const flags,
    LPCWCH           const string1,
    int              const string1_count,
    LPCWCH           const string2,
    int              const string2_count,
    LPNLSVERSIONINFO const version,
    LPVOID           const reserved,
    LPARAM           const param
    )
{
    if (auto const compare_string_ex = try_get_CompareStringEx())
    {
        // On WCOS devices, CompareStringEx may calls into icu.dll which is an OS component using the UCRT.
        // If icu.dll calls any UCRT export under OS mode (ex: malloc), then CompareStringEx will return under Prog Mode even if
        // we started in OS mode. To prevent this, an OS mode guard is in place.
        __crt_state_management::scoped_global_state_reset os_mode_guard;
        return compare_string_ex(locale_name, flags, string1, string1_count, string2, string2_count, version, reserved, param);
    }

    return CompareStringW(__acrt_LocaleNameToLCID(locale_name, 0), flags, string1, string1_count, string2, string2_count);
}

#ifdef __clang__
static LOCALE_ENUMPROCEX static_enum_proc;
static BOOL CALLBACK LocaleEnumProcW(LPWSTR locale_string)
{
    return __crt_fast_decode_pointer(static_enum_proc)(locale_string, 0, 0);
}
#endif

// This has been split into its own function to work around a bug in the Dev12
// C++ compiler where nested captureless lambdas are not convertible to the
// required function pointer type.
static BOOL enum_system_locales_ex_nolock(
    LOCALE_ENUMPROCEX const enum_proc
    ) throw()
{
#ifndef __clang__
    static LOCALE_ENUMPROCEX static_enum_proc;
#endif

    static_enum_proc = __crt_fast_encode_pointer(enum_proc);
    BOOL const result = EnumSystemLocalesW((LOCALE_ENUMPROCW)
#ifdef __clang__
        LocaleEnumProcW,
#else
        [](LPWSTR locale_string)
        #if defined(__GNUC__) && !defined(__clang__)
        __stdcall
        #endif // __GNUC__
        { return __crt_fast_decode_pointer(static_enum_proc)(locale_string, 0, 0); },
#endif
        LCID_INSTALLED);
    static_enum_proc = __crt_fast_encode_pointer(nullptr);

    return result;
}

extern "C" BOOL WINAPI __acrt_EnumSystemLocalesEx(
    LOCALE_ENUMPROCEX const enum_proc,
    DWORD             const flags,
    LPARAM            const param,
    LPVOID            const reserved
    )
{
    if (auto const enum_system_locales_ex = try_get_EnumSystemLocalesEx())
    {
        return enum_system_locales_ex(enum_proc, flags, param, reserved);
    }

    return __acrt_lock_and_call(__acrt_locale_lock, [&]() -> BOOL
    {
        return enum_system_locales_ex_nolock(enum_proc);
    });
}

extern "C" DWORD WINAPI __acrt_FlsAlloc(PFLS_CALLBACK_FUNCTION const callback)
{
#if FLS_ALWAYS_AVAILABLE
    return FlsAlloc(callback);
#else
    if (auto const fls_alloc = try_get_FlsAlloc())
    {
        return fls_alloc(callback);
    }

    return TlsAlloc();
#endif
}

extern "C" BOOL WINAPI __acrt_FlsFree(DWORD const fls_index)
{
#if FLS_ALWAYS_AVAILABLE
    return FlsFree(fls_index);
#else
    if (auto const fls_free = try_get_FlsFree())
    {
        return fls_free(fls_index);
    }

    return TlsFree(fls_index);
#endif
}

extern "C" PVOID WINAPI __acrt_FlsGetValue(DWORD const fls_index)
{
#if FLS_ALWAYS_AVAILABLE
    return FlsGetValue(fls_index);
#else
    if (auto const fls_get_value = try_get_FlsGetValue())
    {
        return fls_get_value(fls_index);
    }

    return TlsGetValue(fls_index);
#endif
}

extern "C" BOOL WINAPI __acrt_FlsSetValue(DWORD const fls_index, PVOID const fls_data)
{
#if FLS_ALWAYS_AVAILABLE
    return FlsSetValue(fls_index, fls_data);
#else
    if (auto const fls_set_value = try_get_FlsSetValue())
    {
        return fls_set_value(fls_index, fls_data);
    }

    return TlsSetValue(fls_index, fls_data);
#endif
}

extern "C" int WINAPI __acrt_GetDateFormatEx(
    LPCWSTR           const locale_name,
    DWORD             const flags,
    SYSTEMTIME CONST* const date,
    LPCWSTR           const format,
    LPWSTR            const buffer,
    int               const buffer_count,
    LPCWSTR           const calendar
    )
{
    if (auto const get_date_format_ex = try_get_GetDateFormatEx())
    {
        return get_date_format_ex(locale_name, flags, date, format, buffer, buffer_count, calendar);
    }

    return GetDateFormatW(__acrt_LocaleNameToLCID(locale_name, 0), flags, date, format, buffer, buffer_count);
}

extern "C" int WINAPI __acrt_GetTempPath2W(
    DWORD nBufferLength,
    LPWSTR lpBuffer
)
{
    if (auto const get_temp_path2w = try_get_GetTempPath2W())
    {
        return get_temp_path2w(nBufferLength, lpBuffer);
    }
    return GetTempPathW(nBufferLength, lpBuffer);
}

extern "C" DWORD64 WINAPI __acrt_GetEnabledXStateFeatures()
{
    if (auto const get_enabled_xstate_features = try_get_GetEnabledXStateFeatures())
    {
        return get_enabled_xstate_features();
    }

    abort(); // No fallback; callers should check availablility before calling
}

extern "C" int WINAPI __acrt_GetLocaleInfoEx(
    LPCWSTR const locale_name,
    LCTYPE  const lc_type,
    LPWSTR  const data,
    int     const data_count
    )
{
    if (auto const get_locale_info_ex = try_get_GetLocaleInfoEx())
    {
        return get_locale_info_ex(locale_name, lc_type, data, data_count);
    }

    return GetLocaleInfoW(__acrt_LocaleNameToLCID(locale_name, 0), lc_type, data, data_count);
}

extern "C" VOID WINAPI __acrt_GetSystemTimePreciseAsFileTime(LPFILETIME const system_time)
{
    if (auto const get_system_time_precise_as_file_time = try_get_GetSystemTimePreciseAsFileTime())
    {
        return get_system_time_precise_as_file_time(system_time);
    }

    return GetSystemTimeAsFileTime(system_time);
}

extern "C" int WINAPI __acrt_GetTimeFormatEx(
    LPCWSTR           const locale_name,
    DWORD             const flags,
    SYSTEMTIME CONST* const time,
    LPCWSTR           const format,
    LPWSTR            const buffer,
    int               const buffer_count
    )
{
    if (auto const get_time_format_ex = try_get_GetTimeFormatEx())
    {
        return get_time_format_ex(locale_name, flags, time, format, buffer, buffer_count);
    }

    return GetTimeFormatW(__acrt_LocaleNameToLCID(locale_name, 0), flags, time, format, buffer, buffer_count);
}

extern "C" int WINAPI __acrt_GetUserDefaultLocaleName(
    LPWSTR const locale_name,
    int    const locale_name_count
    )
{
    if (auto const get_user_default_locale_name = try_get_GetUserDefaultLocaleName())
    {
        return get_user_default_locale_name(locale_name, locale_name_count);
    }

    return __acrt_LCIDToLocaleName(GetUserDefaultLCID(), locale_name, locale_name_count, 0);
}

extern "C" BOOL WINAPI __acrt_GetXStateFeaturesMask(
    PCONTEXT const context,
    PDWORD64 const feature_mask
    )
{
    if (auto const get_xstate_features_mask = try_get_GetXStateFeaturesMask())
    {
        return get_xstate_features_mask(context, feature_mask);
    }

    abort(); // No fallback; callers should check availablility before calling
}

extern "C" BOOL WINAPI __acrt_InitializeCriticalSectionEx(
    LPCRITICAL_SECTION const critical_section,
    DWORD              const spin_count,
    DWORD              const flags
    )
{
    if (auto const initialize_critical_section_ex = try_get_InitializeCriticalSectionEx())
    {
        return initialize_critical_section_ex(critical_section, spin_count, flags);
    }

    return InitializeCriticalSectionAndSpinCount(critical_section, spin_count);
}

extern "C" BOOL WINAPI __acrt_IsValidLocaleName(LPCWSTR const locale_name)
{
    if (auto const is_valid_locale_name = try_get_IsValidLocaleName())
    {
        return is_valid_locale_name(locale_name);
    }

    return IsValidLocale(__acrt_LocaleNameToLCID(locale_name, 0), LCID_INSTALLED);
}

extern "C" int WINAPI __acrt_LCMapStringEx(
    LPCWSTR          const locale_name,
    DWORD            const flags,
    LPCWSTR          const source,
    int              const source_count,
    LPWSTR           const destination,
    int              const destination_count,
    LPNLSVERSIONINFO const version,
    LPVOID           const reserved,
    LPARAM           const sort_handle
    )
{
    if (auto const lc_map_string_ex = try_get_LCMapStringEx())
    {
        return lc_map_string_ex(locale_name, flags, source, source_count, destination, destination_count, version, reserved, sort_handle);
    }
#pragma warning(disable:__WARNING_PRECONDITION_NULLTERMINATION_VIOLATION) // 26035 LCMapStringW annotation is presently incorrect 11/26/2014 Jaykrell
    return LCMapStringW(__acrt_LocaleNameToLCID(locale_name, 0), flags, source, source_count, destination, destination_count);
}

extern "C" int WINAPI __acrt_LCIDToLocaleName(
    LCID   const locale,
    LPWSTR const name,
    int    const name_count,
    DWORD  const flags
    )
{
    if (auto const lcid_to_locale_name = try_get_LCIDToLocaleName())
    {
        return lcid_to_locale_name(locale, name, name_count, flags);
    }

    return __acrt_DownlevelLCIDToLocaleName(locale, name, name_count);
}

extern "C" LCID WINAPI __acrt_LocaleNameToLCID(
    LPCWSTR const name,
    DWORD   const flags
    )
{
    if (auto const locale_name_to_lcid = try_get_LocaleNameToLCID())
    {
        return locale_name_to_lcid(name, flags);
    }

    return __acrt_DownlevelLocaleNameToLCID(name);
}

extern "C" PVOID WINAPI __acrt_LocateXStateFeature(
    PCONTEXT const content,
    DWORD    const feature_id,
    PDWORD   const length
    )
{
    if (auto const locate_xstate_feature = try_get_LocateXStateFeature())
    {
        return locate_xstate_feature(content, feature_id, length);
    }

    abort(); // No fallback; callers should check availablility before calling
}

extern "C" int WINAPI __acrt_MessageBoxA(
    HWND   const hwnd,
    LPCSTR const text,
    LPCSTR const caption,
    UINT   const type
    )
{
    if (auto const message_box_a = try_get_MessageBoxA())
    {
        return message_box_a(hwnd, text, caption, type);
    }

    abort(); // No fallback; callers should check availablility before calling
}

extern "C" int WINAPI __acrt_MessageBoxW(
    HWND    const hwnd,
    LPCWSTR const text,
    LPCWSTR const caption,
    UINT    const type
    )
{
    if (auto const message_box_w = try_get_MessageBoxW())
    {
        return message_box_w(hwnd, text, caption, type);
    }

    abort(); // No fallback; callers should check availablility before calling
}

extern "C" BOOLEAN WINAPI __acrt_RtlGenRandom(
    PVOID const buffer,
    ULONG const buffer_count
    )
{
    if (auto const rtl_gen_random = try_get_SystemFunction036())
    {
        return rtl_gen_random(buffer, buffer_count);
    }

    abort(); // No fallback (this function should exist)
}

extern "C" HRESULT WINAPI __acrt_RoInitialize(RO_INIT_TYPE const init_type)
{
    if (auto const ro_initialize = try_get_RoInitialize())
    {
        return ro_initialize(init_type);
    }

    return S_OK; // No fallback (this is a best-effort wrapper)
}

extern "C" void WINAPI __acrt_RoUninitialize()
{
    if (auto const ro_uninitialize = try_get_RoUninitialize())
    {
        return ro_uninitialize();
    }

    // No fallback (this is a best-effort wrapper)
}

LONG WINAPI __acrt_AppPolicyGetProcessTerminationMethodInternal(_Out_ AppPolicyProcessTerminationMethod* policy)
{
    if (auto const app_policy_get_process_terminaton_method_claims = try_get_AppPolicyGetProcessTerminationMethod())
    {
        return app_policy_get_process_terminaton_method_claims(GetCurrentThreadEffectiveToken(), policy);
    }

    return STATUS_NOT_FOUND;
}

LONG WINAPI __acrt_AppPolicyGetThreadInitializationTypeInternal(_Out_ AppPolicyThreadInitializationType* policy)
{
    if (auto const app_policy_get_thread_initialization_type_claims = try_get_AppPolicyGetThreadInitializationType())
    {
        return app_policy_get_thread_initialization_type_claims(GetCurrentThreadEffectiveToken(), policy);
    }

    return STATUS_NOT_FOUND;
}

LONG WINAPI __acrt_AppPolicyGetShowDeveloperDiagnosticInternal(_Out_ AppPolicyShowDeveloperDiagnostic* policy)
{
    if (auto const app_policy_get_show_developer_diagnostic_claims = try_get_AppPolicyGetShowDeveloperDiagnostic())
    {
        return app_policy_get_show_developer_diagnostic_claims(GetCurrentThreadEffectiveToken(), policy);
    }

    return STATUS_NOT_FOUND;
}

LONG WINAPI __acrt_AppPolicyGetWindowingModelInternal(_Out_ AppPolicyWindowingModel* policy)
{
    if (auto const app_policy_get_windowing_model_claims = try_get_AppPolicyGetWindowingModel())
    {
        return app_policy_get_windowing_model_claims(GetCurrentThreadEffectiveToken(), policy);
    }

    return STATUS_NOT_FOUND;
}

extern "C" BOOL WINAPI __acrt_SetThreadStackGuarantee(PULONG const stack_size_in_bytes)
{
    if (auto const set_thread_stack_guarantee = try_get_SetThreadStackGuarantee())
    {
        return set_thread_stack_guarantee(stack_size_in_bytes);
    }

    return FALSE;
}

extern "C" bool __cdecl __acrt_can_show_message_box()
{
    bool can_show_message_box = false;
    if (__acrt_get_windowing_model_policy() == windowing_model_policy_hwnd
        && try_get_MessageBoxA() != nullptr
        && try_get_MessageBoxW() != nullptr)
    {
        can_show_message_box = true;
    }
    return can_show_message_box;
}

extern "C" bool __cdecl __acrt_can_use_vista_locale_apis()
{
    return try_get_CompareStringEx() != nullptr;
}

// This function simply attempts to get each of the locale-related APIs.  This
// allows a caller to "pre-load" the modules in which these APIs are hosted.  We
// use this in the _wsetlocale implementation to avoid calls to LoadLibrary while
// the locale lock is held.
extern "C" void __cdecl __acrt_eagerly_load_locale_apis()
{
    try_get_AreFileApisANSI();
    try_get_CompareStringEx();
    try_get_EnumSystemLocalesEx();
    try_get_GetDateFormatEx();
    try_get_GetLocaleInfoEx();
    try_get_GetTimeFormatEx();
    try_get_GetUserDefaultLocaleName();
    try_get_IsValidLocaleName();
    try_get_LCMapStringEx();
    try_get_LCIDToLocaleName();
    try_get_LocaleNameToLCID();
}

extern "C" bool __cdecl __acrt_can_use_xstate_apis()
{
    return try_get_LocateXStateFeature() != nullptr;
}

extern "C" HWND __cdecl __acrt_get_parent_window()
{
    auto const get_active_window = try_get_GetActiveWindow();
    if (!get_active_window)
    {
        return nullptr;
    }

    HWND const active_window = get_active_window();
    if (!active_window)
    {
        return nullptr;
    }

    auto const get_last_active_popup = try_get_GetLastActivePopup();
    if (!get_last_active_popup)
    {
        return active_window;
    }

    return get_last_active_popup(active_window);
}

extern "C" bool __cdecl __acrt_is_interactive()
{
    auto const get_process_window_station = try_get_GetProcessWindowStation();
    if (!get_process_window_station)
    {
        return true;
    }

    auto const get_user_object_information = try_get_GetUserObjectInformationW();
    if (!get_user_object_information)
    {
        return true;
    }

    HWINSTA const hwinsta = get_process_window_station();
    if (!hwinsta)
    {
        return false;
    }

    USEROBJECTFLAGS uof{};
    if (!get_user_object_information(hwinsta, UOI_FLAGS, &uof, sizeof(uof), nullptr))
    {
        return false;
    }

    if ((uof.dwFlags & WSF_VISIBLE) == 0)
    {
        return false;
    }

    return true;
}
