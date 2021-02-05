
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(clusapi.dll clusapi.spec)

list(APPEND SOURCE
    clusapi.c
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/clusapi.def)

add_library(clusapi MODULE ${SOURCE})

if(MSVC)
    # error C4312: 'type cast': conversion from 'unsigned int' to 'HCLUSTER' of greater size
    remove_target_compile_option(clusapi "/we4312")
endif()

set_module_type(clusapi win32dll)
target_link_libraries(clusapi wine)
add_importlibs(clusapi msvcrt kernel32 ntdll)
add_cd_file(TARGET clusapi DESTINATION reactos/system32 FOR all)
