
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(mcicda SHARED mcicda.c mcicda.def)

if(NOT MSVC)
    set_source_files_properties(mcicda.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(mcicda 0)

target_link_libraries(mcicda wine)

add_importlibs(mcicda winmm user32 kernel32 ntdll)
add_dependencies(mcicda psdk buildno_header)
add_cab_target(mcicda 1)