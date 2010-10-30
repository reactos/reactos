
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(shfolder.dll shfolder.spec)

add_library(shfolder SHARED version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shfolder.def)

set_entrypoint(shfolder 0)

target_link_libraries(shfolder wine)

add_importlibs(shfolder shell32 kernel32 ntdll)
add_dependencies(shfolder psdk buildno_header)
