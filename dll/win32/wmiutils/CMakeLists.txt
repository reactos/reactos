
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c
    path.c
    statuscode.c)

add_library(wmiutils MODULE ${SOURCE} wmiutils.rc)
spec2def(wmiutils wmiutils.dll wmiutils.spec)

set_module_type(wmiutils win32dll)
target_link_libraries(wmiutils wine)
add_importlibs(wmiutils oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wmiutils DESTINATION reactos/system32/wbem FOR all)
