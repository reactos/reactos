
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    main.c)

add_library(updspapi MODULE ${SOURCE})
spec2def(updspapi updspapi.dll updspapi.spec)

set_module_type(updspapi win32dll)
target_link_libraries(updspapi wine)
add_importlibs(updspapi setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET updspapi DESTINATION reactos/system32 FOR all)
