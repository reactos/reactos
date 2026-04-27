
add_subdirectory(packages)

# Setup settings file
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/txtsetup.sif DESTINATION reactos NO_CAB FOR bootcd regtest)

add_custom_target(converted_caroots_inf DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/caroots.inf)
utf16le_convert(${CMAKE_CURRENT_SOURCE_DIR}/caroots.inf ${CMAKE_CURRENT_BINARY_DIR}/caroots.inf)
add_cd_file(TARGET converted_caroots_inf FILE ${CMAKE_CURRENT_BINARY_DIR}/caroots.inf DESTINATION reactos NO_CAB FOR bootcd regtest)

# Common hives
add_registry_inf(
    hivecls.inf
    hivedef.inf
    hivesft.inf
    hivesys.inf
    hivebcd.inf)

# Regtest
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/regtest.cmd DESTINATION reactos/bin FOR all)

# autorun.inf
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/autorun.inf DESTINATION root NO_CAB FOR bootcd)

# icon.ico
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico DESTINATION root NO_CAB FOR bootcd)

# readme.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION root NO_CAB FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION reactos FOR all)

## NOTE: The root setup.exe file is a renamed welcome.exe for BootCD purposes.
# welcome.exe optional custom configuration (only for LiveImage)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/welcome_config/)
    # Copy the main configuration file
    add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/welcome_config/welcome.ini DESTINATION reactos NO_CAB FOR livecd)

    # Convert the translation files (name format: xx-YY.ini) into UTF-16
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/welcome_config)
    file(GLOB I18N_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/welcome_config/ ${CMAKE_CURRENT_SOURCE_DIR}/welcome_config/*-*.ini)
    foreach(_file ${I18N_FILES})
        set(_converted_file ${CMAKE_CURRENT_BINARY_DIR}/welcome_config/${_file})
        set(_source_file ${CMAKE_CURRENT_SOURCE_DIR}/welcome_config/${_file})
        utf16le_convert(${_source_file} ${_converted_file})
        add_cd_file(TARGET converted_welcome_i18n_files FILE ${_converted_file} DESTINATION reactos/welcome NO_CAB NAME_ON_CD ${_file} FOR livecd)
        list(APPEND _converted_welcome_i18n_files ${_converted_file})
    endforeach(_file)
    add_custom_target(converted_welcome_i18n_files DEPENDS ${_converted_welcome_i18n_files})
endif()

# freeldr.ini
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd.ini DESTINATION root NO_CAB NAME_ON_CD freeldr.ini FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd_regtest.ini DESTINATION root NO_CAB NAME_ON_CD freeldr.ini FOR regtest)

# Unattend
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd/unattend.inf DESTINATION reactos NO_CAB FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/unattend.inf DESTINATION reactos NO_CAB FOR regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/livecd/unattend.inf DESTINATION reactos NO_CAB FOR livecd)

# LiveImage shortcuts
add_custom_target(livecd_links)
function(add_livecd_shortcut name app)
    cmake_parse_arguments(_shortcut "" "" "DESTINATION" ${ARGN})
    if(NOT _shortcut_DESTINATION)
        message(FATAL_ERROR "You must provide at least one destination")
    endif()

    add_link(${name} livecd_start.cmd GUID "{450D8FBA-AD25-11D0-98A8-0800361B1103}" CMD_LINE_ARGS ${app} ICON ${app} MINIMIZE)
    # Only on CMake 3.20+: target_sources(livecd_links PRIVATE ...)
    set_property(TARGET livecd_links APPEND PROPERTY SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk")

    foreach(dest ${_shortcut_DESTINATION})
        add_cd_file(FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk"
                    TARGET livecd_links
                    DESTINATION ${dest}
                    FOR livecd)
    endforeach()
endfunction()

## NOTE: What would be nice is to create this list using /media/inf/shortcuts.inf
## and taking their default english translation!

add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/livecd_start.cmd" DESTINATION "Profiles/Default User/My Documents" FOR livecd)

set(dir_allusers_desktop        "Profiles/All Users/Desktop")
set(dir_startmenu_programs      "Profiles/All Users/Start Menu/Programs")
set(dir_startmenu_admintools    "${dir_startmenu_programs}/Administrative Tools")
set(dir_startmenu_accessories   "${dir_startmenu_programs}/Accessories")
set(dir_startmenu_games         "${dir_startmenu_programs}/Games")
set(dir_startmenu_accessibility "${dir_startmenu_accessories}/Accessibility")
set(dir_startmenu_comms         "${dir_startmenu_accessories}/Communications")
set(dir_startmenu_entertainment "${dir_startmenu_accessories}/Entertainment")
set(dir_startmenu_systemtools   "${dir_startmenu_accessories}/System Tools")
set(dir_quicklaunch             "Profiles/Default User/Application Data/Microsoft/Internet Explorer/Quick Launch")

add_livecd_shortcut("Read Me" readme.txt DESTINATION "${dir_allusers_desktop}")
add_livecd_shortcut("ReactOS Explorer" explorer.exe
    DESTINATION "${dir_startmenu_programs}" "${dir_quicklaunch}")
add_livecd_shortcut("Command Prompt" cmd.exe
    DESTINATION "${dir_allusers_desktop}" "${dir_startmenu_accessories}" "${dir_quicklaunch}")
add_livecd_shortcut("Device Manager" devmgmt.exe DESTINATION "${dir_startmenu_admintools}")
add_livecd_shortcut("Event Viewer" eventvwr.exe DESTINATION "${dir_startmenu_admintools}")
add_livecd_shortcut("Service Manager" servman.exe DESTINATION "${dir_startmenu_admintools}")
add_livecd_shortcut("System Configuration" msconfig.exe DESTINATION "${dir_startmenu_admintools}")
add_livecd_shortcut("Accessibility Utility Manager" utilman.exe DESTINATION "${dir_startmenu_accessibility}")
add_livecd_shortcut("Magnify" magnify.exe DESTINATION "${dir_startmenu_accessibility}")
add_livecd_shortcut("On-Screen Keyboard" osk.exe DESTINATION "${dir_startmenu_accessibility}")
add_livecd_shortcut("Remote Desktop Connection" mstsc.exe DESTINATION "${dir_startmenu_comms}")
add_livecd_shortcut("Multimedia Player" mplay32.exe DESTINATION "${dir_startmenu_entertainment}")
add_livecd_shortcut("Sound Recorder" sndrec32.exe DESTINATION "${dir_startmenu_entertainment}")
add_livecd_shortcut("Volume Control" sndvol32.exe DESTINATION "${dir_startmenu_entertainment}")
add_livecd_shortcut("Character Map" charmap.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("Clipboard Viewer" clipbrd.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("Keyboard Layout Switcher" kbswitch.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("ReactX Diagnostic" dxdiag.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("Registry Editor" regedit.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("Task Manager" taskmgr.exe DESTINATION "${dir_startmenu_systemtools}")
add_livecd_shortcut("Calculator" calc.exe DESTINATION "${dir_startmenu_accessories}")
add_livecd_shortcut("Paint" mspaint.exe DESTINATION "${dir_startmenu_accessories}")
add_livecd_shortcut("Notepad" notepad.exe DESTINATION "${dir_startmenu_accessories}")
add_livecd_shortcut("WordPad" wordpad.exe DESTINATION "${dir_startmenu_accessories}")
add_livecd_shortcut("Solitaire" sol.exe DESTINATION "${dir_startmenu_games}")
add_livecd_shortcut("Spider Solitaire" spider.exe DESTINATION "${dir_startmenu_games}")
add_livecd_shortcut("WineMine" winmine.exe DESTINATION "${dir_startmenu_games}")
