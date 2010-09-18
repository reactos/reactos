


add_definitions(-DUSE_MSVCRT_PREFIX)
add_definitions(-D_MSVCRT_LIB_)
add_definitions(-D_MSVCRT_)
add_definitions(-DCRTDLL)
add_definitions(-D_CTYPE_DISABLE_MACROS)
add_definitions(-D_NO_INLINING)
add_definitions(-D__MINGW_IMPORT="")

include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

spec2def(crtdll ${CMAKE_CURRENT_SOURCE_DIR}/crtdll.spec ${CMAKE_CURRENT_BINARY_DIR}/crtdll.def)

list(APPEND SOURCE dllmain.c crtdll.rc)

add_library(crtdll SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll_precomp.h.gch
    ${SOURCE})

set_target_properties(crtdll PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(crtdll
    ${CMAKE_CURRENT_BINARY_DIR}/crtdll.def
    wine
    crt
    -lkernel32
    -lntdll)

add_pch(crtdll ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(crtdll crtdll_def psdk)
