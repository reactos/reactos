
set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

add_executable(shell32_apitest
    CMyComputer.cpp
    CShellDesktop.cpp
    menu.cpp
    shelltest.cpp
    testlist.c)
target_link_libraries(shell32_apitest wine uuid ${PSEH_LIB})
set_module_type(shell32_apitest win32cui)
add_importlibs(shell32_apitest user32 gdi32 shell32 ole32 oleaut32 shlwapi msvcrt kernel32 ntdll)
add_cd_file(TARGET shell32_apitest DESTINATION reactos/bin FOR all)
