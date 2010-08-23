
SET(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)
include_directories(include)

get_directory_property(defines COMPILE_DEFINITIONS)
get_directory_property(includes INCLUDE_DIRECTORIES)

foreach(arg ${defines})
  set(result_defs "${result_defs} -D${arg}")
endforeach(arg ${defines})

foreach(arg ${includes})
  set(result_incs "-I${arg} ${result_incs}")
endforeach(arg ${includes})

SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${result_defs} ${result_incs} -i <SOURCE> -O coff -o <OBJECT>")

file(GLOB_RECURSE SOURCE *.c)

add_library(user32 SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/user32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/user32_user32.h.gch)

set_target_properties(user32 PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(user32
    ${CMAKE_CURRENT_SOURCE_DIR}/user32.def
    wine
    ${REACTOS_SOURCE_DIR}/dll/win32/gdi32/libgdi32.a
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
    -limm32
    win32ksys
    pseh
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)
                      
add_pch(user32 ${CMAKE_CURRENT_SOURCE_DIR}/include/user32.h ${SOURCE})
add_dependencies(user32 psdk)
