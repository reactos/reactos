
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-DLANGPACK)

include_directories(include)

file(GLOB_RECURSE SOURCE *.c)

add_library(gdi32 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/gdi32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/gdi32_precomp.h.gch)

set_target_properties(gdi32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(gdi32
    ${CMAKE_CURRENT_SOURCE_DIR}/gdi32.def
    ${REACTOS_SOURCE_DIR}/dll/win32/user32/libuser32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
    win32ksys
    dxguid
    pseh
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)
                      
add_pch(gdi32 ${CMAKE_CURRENT_SOURCE_DIR}/include/precomp.h ${SOURCE})
add_dependencies(gdi32 psdk)
