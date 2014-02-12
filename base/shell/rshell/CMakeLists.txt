PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

spec2def(rshell.dll rshell.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    CDesktopBrowser.cpp
    CMenuBand.cpp
    CMenuSite.cpp
    CStartMenu.cpp
    misc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/rshell.def)

add_library(rshell SHARED ${SOURCE})

set_module_type(rshell win32dll UNICODE)

target_link_libraries(rshell
    atlnew
    uuid
    wine)

add_importlibs(rshell
    shlwapi
    shell32
    ole32
    user32
    msvcrt
    kernel32
    ntdll)

add_custom_command(TARGET rshell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "G:/reactos/vs12/reactos/base/shell/explorer-new/$<CONFIGURATION>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying to output directory")
