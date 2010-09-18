


add_definitions(-DUNICODE -D_UNICODE)

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

set_target_properties(msafd PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(msafd
    ${CMAKE_CURRENT_BINARY_DIR}/msafd.def
    -ladvapi32
    -lkernel32
    -lntdll)

add_pch(msafd ${CMAKE_CURRENT_SOURCE_DIR}/msafd.h ${SOURCE})
add_dependencies(msafd msafd_def psdk)
