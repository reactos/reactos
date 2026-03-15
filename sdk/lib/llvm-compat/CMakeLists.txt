list(APPEND SOURCE
    win32_shims.c
    win32_vista_shims.c)

add_library(llvmcompat_static ${SOURCE})
add_dependencies(llvmcompat_static psdk)
target_link_libraries(llvmcompat_static PUBLIC rtl_vista psapi_static libntdll)
target_link_libraries(llvmcompat INTERFACE llvmcompat_static)
