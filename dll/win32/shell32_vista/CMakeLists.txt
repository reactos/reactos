set_cpp(WITH_RUNTIME)
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)

spec2def(shell32_vista.dll shell32_vista.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    pidl.c
    ShellItem.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/shell32_vista.def)

add_library(shell32_vista SHARED ${SOURCE})
set_module_type(shell32_vista win32dll)
add_importlibs(shell32_vista shell32 kernel32 ntdll)
target_link_libraries(shell32_vista atlnew)
add_dependencies(shell32_vista psdk)
add_cd_file(TARGET shell32_vista DESTINATION reactos/system32 FOR all)
