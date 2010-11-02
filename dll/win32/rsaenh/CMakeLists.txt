
add_definitions(
    -D__WINESRC__
    -D_DLL -D__USE_CRTIMP)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

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
    version.rc
    rsaenh.def)

if(NOT MSVC)
    set_source_files_properties(rsaenh.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(rsaenh win32dll)

target_link_libraries(rsaenh wine)

add_importlibs(rsaenh msvcrt crypt32 advapi32 kernel32 ntdll)

add_cab_target(rsaenh 1)
