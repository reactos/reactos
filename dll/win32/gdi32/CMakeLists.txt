


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
    -luser32
    -ladvapi32
    win32ksys
    dxguid
    pseh
    -lkernel32
    -lntdll)
                      
add_pch(gdi32 ${CMAKE_CURRENT_SOURCE_DIR}/include/precomp.h ${SOURCE})
add_dependencies(gdi32 psdk)
