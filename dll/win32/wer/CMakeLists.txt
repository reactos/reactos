
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(wer.dll wer.spec)

list(APPEND SOURCE
    main.c)

add_library(wer MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/wer.def
    ${CMAKE_CURRENT_BINARY_DIR}/wer_stubs.c)

set_module_type(wer win32dll)
target_link_libraries(wer uuid wine)
add_importlibs(wer dbghelp advapi32 shell32 ole32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET wer DESTINATION reactos/system32 FOR all)
