
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
