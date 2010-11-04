
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(imaadp32.acm SHARED imaadp32.c imaadp32.def)

if(NOT MSVC)
    set_source_files_properties(imaadp32.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(imaadp32.acm 0)
set_target_properties(imaadp32.acm PROPERTIES SUFFIX "")

target_link_libraries(imaadp32.acm wine)

add_importlibs(imaadp32.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(imaadp32.acm psdk buildno_header)
add_cab_target(imaadp32.acm 1)
