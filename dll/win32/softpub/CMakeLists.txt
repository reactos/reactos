
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(softpub MODULE
    softpub.rc)
spec2def(softpub softpub.dll softpub.spec)

set_module_type(softpub win32dll ENTRYPOINT 0)
target_link_libraries(softpub wine)
add_importlibs(softpub wintrust ntdll)
add_cd_file(TARGET softpub DESTINATION reactos/system32 FOR all)
