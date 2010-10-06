
add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(rsaenh ${CMAKE_CURRENT_SOURCE_DIR}/rsaenh.spec ${CMAKE_CURRENT_BINARY_DIR}/rsaenh.def)

add_library(rsaenh SHARED
    aes.c
    des.c
    handle.c
    implglue.c
    md2.c
    mpi.c
    rc2.c
    rc4.c
    rsa.c
    rsaenh.c
    sha2.c
    version.rc)

set_module_type(rsaenh win32dll)

target_link_libraries(rsaenh
    ${CMAKE_CURRENT_BINARY_DIR}/rsaenh.def
    wine)

add_importlibs(rsaenh msvcrt crypt32 advapi32 kernel32 ntdll)

add_dependencies(rsaenh rsaenh_def)
