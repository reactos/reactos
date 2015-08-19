PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/atl
    ${REACTOS_SOURCE_DIR}/dll/win32/shell32/shellmenu)

spec2def(rshell.dll rshell.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    misc.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/rshell.def)

add_library(rshell SHARED ${SOURCE})

set_module_type(rshell win32dll UNICODE)

target_link_libraries(rshell
    shellmenu
    atlnew
    uuid
    wine)

add_importlibs(rshell
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

add_custom_command(TARGET rshell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying to output directory")
