
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(softpub.dll softpub.spec)

add_library(softpub MODULE
    softpub.rc
    ${CMAKE_CURRENT_BINARY_DIR}/softpub.def)

set_module_type(softpub win32dll ENTRYPOINT 0 )
target_link_libraries(softpub wine)
add_importlibs(softpub wintrust)
add_cd_file(TARGET softpub DESTINATION reactos/system32 FOR all)
