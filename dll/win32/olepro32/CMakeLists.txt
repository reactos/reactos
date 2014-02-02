
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(olepro32.dll olepro32.spec)

list(APPEND SOURCE
    olepro32stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/olepro32.def)

add_library(olepro32 SHARED ${SOURCE} version.rc)
set_module_type(olepro32 win32dll)
target_link_libraries(olepro32 wine)
add_importlibs(olepro32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET olepro32 DESTINATION reactos/system32 FOR all)
