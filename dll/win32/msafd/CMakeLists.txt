
set_unicode()

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/include/reactos/drivers)

list(APPEND SOURCE
    msafd.rc
    misc/dllmain.c
    misc/event.c
    misc/helpers.c
    misc/sndrcv.c
    misc/stubs.c
    msafd.def)

if(NOT MSVC)
    set_source_files_properties(msafd.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(msafd SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/msafd_msafd.h.gch
    ${SOURCE})

set_module_type(msafd win32dll)

add_pch(msafd ${CMAKE_CURRENT_SOURCE_DIR}/msafd.h ${SOURCE})

add_importlibs(msafd advapi32 msvcrt kernel32 ntdll)

add_cab_target(msafd 1)
