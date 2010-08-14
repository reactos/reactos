
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUSE_MSVCRT_PREFIX)
add_definitions(-D_MSVCRT_)
add_definitions(-D_MSVCRT_LIB_)
add_definitions(-D_MT)
add_definitions(-D_CTYPE_DISABLE_MACROS)
add_definitions(-D_NO_INLINING)
add_definitions(-DCRTDLL)
add_definitions(-D__MINGW_IMPORT="")

include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

file(GLOB_RECURSE SOURCE *.c)

add_library(msvcrt SHARED
            ${SOURCE}
            ${CMAKE_CURRENT_SOURCE_DIR}/msvcrt.rc
            ${CMAKE_CURRENT_BINARY_DIR}/msvcrt_precomp.h.gch)

set_target_properties(msvcrt PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12 -u __seh_longjmp_unwind@4")

target_link_libraries(msvcrt ${CMAKE_CURRENT_SOURCE_DIR}/msvcrt.def
                      crt
                      wine
                      pseh
                      ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
                      ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(msvcrt ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(msvcrt psdk)
