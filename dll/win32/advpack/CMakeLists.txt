
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    advpack.c
    files.c
    install.c
    reg.c
    advpack.def)

if(NOT MSVC)
    set_source_files_properties(advpack.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(advpack SHARED ${SOURCE})

set_module_type(advpack win32dll)

target_link_libraries(advpack wine)

add_importlibs(advpack ole32 setupapi version advapi32 msvcrt kernel32 ntdll)

add_cab_target(advpack 1)
