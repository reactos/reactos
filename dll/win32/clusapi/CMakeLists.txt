
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

if(MSVC)
    # error C4312: 'type cast': conversion from 'unsigned int' to 'HCLUSTER' of greater size
    replace_compile_flags("/we4312" " ")
endif()

list(APPEND SOURCE
    clusapi.c)

add_library(clusapi MODULE ${SOURCE})
spec2def(clusapi clusapi.dll clusapi.spec)

set_module_type(clusapi win32dll)
target_link_libraries(clusapi wine)
add_importlibs(clusapi msvcrt kernel32 ntdll)
add_cd_file(TARGET clusapi DESTINATION reactos/system32 FOR all)
