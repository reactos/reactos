PROJECT(SHELL)

add_definitions(
    -D_ATL_NO_EXCEPTIONS)

set_cpp(WITH_RUNTIME)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl)

list(APPEND SOURCE
    CQuickLaunchBand.cpp
    misc.cpp)

add_library(rshell MODULE ${SOURCE})
spec2def(rshell rshell.dll rshell.spec ADD_IMPORTLIB)

set_module_type(rshell win32dll UNICODE)

target_link_libraries(rshell
    shellbars
    shellmenu
    shelldesktop
    uuid
    wine)

add_importlibs(rshell
    browseui
    uxtheme
    shlwapi
    advapi32
    shell32
    comctl32
    gdi32
    ole32
    user32
    msvcrt
    kernel32
    ntdll)

add_custom_command(TARGET rshell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "$<TARGET_FILE_DIR:explorer>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying to output directory")

add_custom_command(TARGET rshell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying to output directory")
