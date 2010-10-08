
set_unicode()

include_directories(BEFORE include)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/drivers)

spec2def(msafd ${CMAKE_CURRENT_SOURCE_DIR}/msafd.spec ${CMAKE_CURRENT_BINARY_DIR}/msafd.def)

list(APPEND SOURCE
    msafd.rc
    misc/dllmain.c
    misc/event.c
    misc/helpers.c
    misc/sndrcv.c
    misc/stubs.c)

add_library(msafd SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/msafd_msafd.h.gch
    ${SOURCE})

set_module_type(msafd win32dll)

target_link_libraries(msafd ${CMAKE_CURRENT_BINARY_DIR}/msafd.def)

add_pch(msafd ${CMAKE_CURRENT_SOURCE_DIR}/msafd.h ${SOURCE})

add_importlibs(msafd advapi32 kernel32 ntdll)
add_dependencies(msafd msafd_def)
