
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(updspapi.dll updspapi.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/updspapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/updspapi.def)

add_library(updspapi MODULE ${SOURCE})
set_module_type(updspapi win32dll)
target_link_libraries(updspapi wine)
add_importlibs(updspapi setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET updspapi DESTINATION reactos/system32 FOR all)
