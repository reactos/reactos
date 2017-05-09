
set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

add_executable(shell32_apitest
    AddCommas.c
    CFSFolder.cpp
    CMyComputer.cpp
    CShellDesktop.cpp
    CShellLink.cpp
    menu.cpp
    ShellExecuteEx.cpp
    shelltest.cpp
    SHParseDisplayName.cpp
    testlist.c)
target_link_libraries(shell32_apitest wine uuid ${PSEH_LIB})
set_module_type(shell32_apitest win32cui)
add_importlibs(shell32_apitest user32 gdi32 shell32 ole32 oleaut32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET shell32_apitest)
