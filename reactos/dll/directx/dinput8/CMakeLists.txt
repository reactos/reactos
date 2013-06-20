
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dinput8.dll dinput8.spec ADD_IMPORTLIB)

add_library(dinput8 SHARED
    dinput8_main.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def)

set_module_type(dinput8 win32dll UNICODE)
target_link_libraries(dinput8 dxguid uuid wine)
add_importlibs(dinput8 dinput ole32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput8 DESTINATION reactos/system32 FOR all)
