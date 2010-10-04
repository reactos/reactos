
include_directories(${REACTOS_SOURCE_DIR}/include/wine)
add_definitions(-D__WINESRC__)

add_executable(msiexec
    msiexec.c
    service.c
    rsrc.rc)

set_module_type(msiexec win32gui)

add_importlibs(msiexec user32 advapi32 ole32 msi msvcrt)
target_link_libraries(msiexec uuid wine)
