
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(mmdevapi.dll mmdevapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    audiovolume.c
    devenum.c
    main.c
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/mmdevapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mmdevapi.def)

add_library(mmdevapi SHARED ${SOURCE} mmdevapi.rc)
set_module_type(mmdevapi win32dll)
target_link_libraries(mmdevapi uuid wine)
add_importlibs(mmdevapi ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mmdevapi DESTINATION reactos/system32 FOR all)
