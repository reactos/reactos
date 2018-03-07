set_cpp(WITH_RUNTIME WITH_EXCEPTIONS)
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

spec2def(shell32_vista.dll shell32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.cpp
    pidl.c
    ShellItem.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_vista.def)

add_library(shell32_vista SHARED ${SOURCE})
set_module_type(shell32_vista win32dll UNICODE)
target_link_libraries(shell32_vista atlnew uuid wine)
add_importlibs(shell32_vista shlwapi shell32 comctl32 gdi32 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(shell32_vista psdk)
add_cd_file(TARGET shell32_vista DESTINATION reactos/system32 FOR all)
