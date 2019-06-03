
add_definitions(-D__WINESRC__)
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    xolehlp.c)

add_library(xolehlp MODULE ${SOURCE})
spec2def(xolehlp xolehlp.dll xolehlp.spec)

set_module_type(xolehlp win32dll)
target_link_libraries(xolehlp adsiid uuid wine)
add_importlibs(xolehlp msvcrt kernel32 ntdll)
add_cd_file(TARGET xolehlp DESTINATION reactos/system32 FOR all)
