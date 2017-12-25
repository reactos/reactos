
set_cpp(WITH_RUNTIME)

spec2def(shell32_apitest.exe shell32_apitest.spec)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    AddCommas.cpp
    Control_RunDLLW.cpp
    CFSFolder.cpp
    CMyComputer.cpp
    CShellDesktop.cpp
    CShellLink.cpp
    menu.cpp
    PathResolve.cpp
    SHCreateFileExtractIconW.cpp
    ShellExecuteEx.cpp
    shelltest.cpp
    SHParseDisplayName.cpp
    shelltest.h)

add_executable(shell32_apitest
    ${SOURCE}
    testlist.c
    resource.rc
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_apitest.def)

if(NOT MSVC)
    # FIXME: Inspect DoEntry() in PathResolve.cpp
    add_target_compile_flags(shell32_apitest "-Wno-error=write-strings")
endif()

target_link_libraries(shell32_apitest wine uuid ${PSEH_LIB})
set_module_type(shell32_apitest win32cui)
add_importlibs(shell32_apitest user32 gdi32 shell32 ole32 oleaut32 advapi32 shlwapi msvcrt kernel32 ntdll)
add_pch(shell32_apitest shelltest.h SOURCE)
add_rostests_file(TARGET shell32_apitest)
