
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(dpnhpast MODULE ${SOURCE} version.rc)
spec2def(dpnhpast dpnhpast.dll dpnhpast.spec)

set_module_type(dpnhpast win32dll)
target_link_libraries(dpnhpast dxguid uuid wine)
add_importlibs(dpnhpast msvcrt kernel32 ntdll)
add_cd_file(TARGET dpnhpast DESTINATION reactos/system32 FOR all)
