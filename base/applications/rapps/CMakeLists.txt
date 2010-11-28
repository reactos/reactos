
set_unicode()

list(APPEND SOURCE
    aboutdlg.c
    available.c
    installdlg.c
    installed.c
    listview.c
    loaddlg.c
    misc.c
    parser.c
    richedit.c
    settingsdlg.c
    splitter.c
    statusbar.c
    toolbar.c
    treeview.c
    winmain.c
    rapps.rc)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${REACTOS_BINARY_DIR}/include/reactos)
set_rc_compiler()

add_executable(rapps ${SOURCE})
set_module_type(rapps win32gui)
target_link_libraries(rapps uuid)

add_importlibs(rapps advapi32 comctl32 gdi32 urlmon user32 shell32 shlwapi kernel32 msvcrt ntdll)
add_dependencies(rapps rappsmsg)

#FIXME : move this to cmakemacros + gcc/msvc.cmake

if(MSVC)
MACRO(MACRO_MC FILE)
    set (COMMAND_MC mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc)
ENDMACRO()
else()
MACRO(MACRO_MC FILE)
    set(COMMAND_MC ${MINGW_PREFIX}windmc -A -b ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc -r ${REACTOS_BINARY_DIR}/include/reactos -h ${REACTOS_BINARY_DIR}/include/reactos)
ENDMACRO()
endif()

MACRO (MACRO_ADD_MESSAGE_HEADERS)

  FOREACH(_in_FILE ${ARGN})

    get_filename_component(FILE ${_in_FILE} NAME_WE)
    MACRO_MC(${FILE})
    add_custom_command(
      OUTPUT ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h
      COMMAND ${COMMAND_MC}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.mc
    )
    set_source_files_properties(
      ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc
      PROPERTIES GENERATED TRUE
    )
    add_custom_target(${FILE} ALL DEPENDS ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.h ${REACTOS_BINARY_DIR}/include/reactos/${FILE}.rc)

  ENDFOREACH(_in_FILE ${ARGN})

ENDMACRO (MACRO_ADD_MESSAGE_HEADERS)

MACRO_ADD_MESSAGE_HEADERS(rappsmsg.mc)

add_cab_target(rapps 1)
