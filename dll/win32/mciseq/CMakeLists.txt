
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(mciseq SHARED mcimidi.c mciseq.def)

if(NOT MSVC)
    set_source_files_properties(mciseq.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(mciseq 0)

target_link_libraries(mciseq wine)

add_importlibs(mciseq winmm user32 kernel32 ntdll)
add_dependencies(mciseq psdk buildno_header)
add_cab_target(mciseq 1)