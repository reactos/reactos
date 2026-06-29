
spec2def(bluetoothapis.dll bluetoothapis.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    sdp.c
    ${CMAKE_CURRENT_BINARY_DIR}/bluetoothapis_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bluetoothapis.def)

add_library(bluetoothapis MODULE ${SOURCE})
set_module_type(bluetoothapis win32dll UNICODE)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(bluetoothapis PRIVATE -Wno-old-style-declaration)
endif()

target_link_libraries(bluetoothapis wine wine_dll_canunload)
add_importlibs(bluetoothapis setupapi msvcrt kernel32 ntdll)
add_cd_file(TARGET bluetoothapis DESTINATION reactos/system32 FOR all)
set_wine_module(bluetoothapis)
