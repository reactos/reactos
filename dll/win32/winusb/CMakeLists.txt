
spec2def(winusb.dll winusb.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/winusb_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winusb.def)

add_library(winusb MODULE ${SOURCE})
set_module_type(winusb win32dll)
target_link_libraries(winusb wine)
add_importlibs(winusb msvcrt kernel32 ntdll)
add_cd_file(TARGET winusb DESTINATION reactos/system32 FOR all)
set_wine_module(winusb)
