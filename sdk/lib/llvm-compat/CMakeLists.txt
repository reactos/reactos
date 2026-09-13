list(APPEND SOURCE
    c99_printf_hacks.c
    imp_alias_hacks.c
    k32_psapi_hacks.c
    sync_static.c)

add_library(llvmcompat STATIC ${SOURCE})
add_dependencies(llvmcompat psdk)
# rtl_vista: the static RTL SRW/condvar/run-once implementation; libntdll: RtlRaiseStatus, keyed events
target_link_libraries(llvmcompat INTERFACE libpsapi rtl_vista libntdll)
