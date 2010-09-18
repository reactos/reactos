


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
