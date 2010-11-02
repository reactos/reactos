
add_definitions(
    -D_DLL -D__USE_CRTIMP
    -D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(msg711.acm SHARED msg711.c msg711.def)

if(NOT MSVC)
    set_source_files_properties(msg711.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(msg711.acm 0)
set_target_properties(msg711.acm PROPERTIES SUFFIX "")

target_link_libraries(msg711.acm wine)

add_importlibs(msg711.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(msg711.acm psdk buildno_header)
add_cab_target(msg711.acm 1)
