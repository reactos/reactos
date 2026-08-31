
spec2def(rsaenh.dll rsaenh.spec ADD_IMPORTLIB NO_PRIVATE_WARNINGS)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/symcrypt/inc)

list(APPEND SOURCE
    handle.c
	implglue.c
	rsaenh.c)

add_library(rsaenh MODULE
    ${SOURCE}
    rsrc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rsaenh.def)

set_module_type(rsaenh win32dll)
target_link_libraries(rsaenh wine wine_dll_register symcrypt)
add_importlibs(rsaenh msvcrt crypt32 advapi32 kernel32 kernel32_vista ntdll)
if(MSVC)
	# warning C4068: unknown pragma 'GCC'
    target_compile_options(rsaenh PRIVATE /wd4068)
endif()
add_cd_file(TARGET rsaenh DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(rsaenh)
