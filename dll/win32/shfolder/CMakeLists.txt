
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

add_library(shfolder SHARED version.rc shfolder.def)

if(NOT MSVC)
    set_source_files_properties(shfolder.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(shfolder 0)

target_link_libraries(shfolder wine)

add_importlibs(shfolder shell32 kernel32 ntdll)
add_dependencies(shfolder psdk buildno_header)
