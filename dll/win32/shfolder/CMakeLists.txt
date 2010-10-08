
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(shfolder ${CMAKE_CURRENT_SOURCE_DIR}/shfolder.spec ${CMAKE_CURRENT_BINARY_DIR}/shfolder.def)

add_library(shfolder SHARED version.rc)

set_entrypoint(shfolder 0)

target_link_libraries(shfolder
    ${CMAKE_CURRENT_BINARY_DIR}/shfolder.def
    wine)

add_importlibs(shfolder shell32 kernel32 ntdll)
add_dependencies(shfolder shfolder_def psdk buildno_header)
