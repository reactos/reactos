
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-DLANGPACK)
add_definitions(-DUNICODE -D_UNICODE)

include_directories(include)

spec2def(lpk ${CMAKE_CURRENT_SOURCE_DIR}/lpk.spec ${CMAKE_CURRENT_BINARY_DIR}/lpk.def)

add_library(lpk SHARED dllmain.c stub.c lpk.rc)

set_target_properties(lpk PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(lpk
    ${CMAKE_CURRENT_BINARY_DIR}/lpk.def
    -luser32
    -lusp10
    -lkernel32
    -lntdll)

add_dependencies(lpk lpk_def psdk)
