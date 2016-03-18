
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(shfolder.dll shfolder.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shfolder.def)

add_library(shfolder SHARED ${SOURCE})
set_module_type(shfolder win32dll ENTRYPOINT 0 )
target_link_libraries(shfolder wine)
add_importlibs(shfolder shell32 kernel32 ntdll)
add_dependencies(shfolder psdk)
add_cd_file(TARGET shfolder DESTINATION reactos/system32 FOR all)
