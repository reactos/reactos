
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(jsproxy MODULE ${SOURCE} rsrc.rc)
spec2def(jsproxy jsproxy.dll jsproxy.spec ADD_IMPORTLIB)

set_module_type(jsproxy win32dll)
target_link_libraries(jsproxy uuid wine)
add_importlibs(jsproxy oleaut32 ole32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET jsproxy DESTINATION reactos/system32 FOR all)
