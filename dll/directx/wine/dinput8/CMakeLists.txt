
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dinput8.dll dinput8.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dinput8_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def)

add_library(dinput8 SHARED ${SOURCE} version.rc)
set_module_type(dinput8 win32dll)
target_link_libraries(dinput8 dxguid uuid wine)
add_importlibs(dinput8 ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput8 DESTINATION reactos/system32 FOR all)
