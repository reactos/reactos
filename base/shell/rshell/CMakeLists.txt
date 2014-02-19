PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

spec2def(rshell.dll rshell.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    CDesktopBrowser.cpp
    CMenuBand.cpp
    CMenuDeskBar.cpp
    CMenuSite.cpp
    CStartMenu.cpp
    misc.cpp
    wraplog.cpp
    logging/CMenuBandWrap.cpp
    logging/CMenuDeskBarWrap.cpp
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
    gdi32
    ole32
    user32
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET rshell DESTINATION reactos FOR all)

add_custom_command(TARGET rshell POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:rshell>"
     "$<TARGET_FILE_DIR:explorer_new>/$<TARGET_FILE_NAME:rshell>" 
  COMMENT "Copying to output directory")
