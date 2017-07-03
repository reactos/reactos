
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
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION root NO_CAB FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION reactos FOR all)

## NOTE: The root file Setup.exe is a renamed Welcome.exe for BootCD purposes.
# Welcome.exe optional custom configuration (only for LiveCD)
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
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd-regtest.ini DESTINATION root NO_CAB NAME_ON_CD freeldr.ini FOR regtest)

# Unattend
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd/unattend.inf DESTINATION reactos NO_CAB FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/unattend.inf DESTINATION reactos NO_CAB FOR regtest)

# LiveImage shortcuts
macro(add_livecd_shortcut name app dest)
    add_link(NAME ${name} CMD_LINE_ARGS ${app} ICON ${app} PATH livecd_start.cmd GUID "{450D8FBA-AD25-11D0-98A8-0800361B1103}" MINIMIZE)
    list(APPEND LIVECD_SHORTCUTS "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk")
    add_cd_file(FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk"
                TARGET livecd_links
                DESTINATION ${dest}
                FOR livecd)
endmacro(add_livecd_shortcut)

## NOTE: What would be nice is to create this list using /media/inf/shortcuts.inf
## and taking their default english translation!
add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/livecd_start.cmd" DESTINATION "Profiles/Default User/My Documents" FOR livecd)
add_livecd_shortcut("Read Me" readme.txt "Profiles/All Users/Desktop")
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/All Users/Desktop")
add_livecd_shortcut("Device Manager" devmgmt.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Event Viewer" eventvwr.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Service Manager" servman.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("System Configuration" msconfig.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Accessibility Utility Manager" utilman.exe "Profiles/All Users/Start Menu/Programs/Accessories/Accessibility")
add_livecd_shortcut("Magnify" magnify.exe "Profiles/All Users/Start Menu/Programs/Accessories/Accessibility")
add_livecd_shortcut("On-Screen Keyboard" osk.exe "Profiles/All Users/Start Menu/Programs/Accessories/Accessibility")
add_livecd_shortcut("Remote Desktop Connection" mstsc.exe "Profiles/All Users/Start Menu/Programs/Accessories/Communications")
add_livecd_shortcut("Multimedia Player" mplay32.exe "Profiles/All Users/Start Menu/Programs/Accessories/Entertainment")
add_livecd_shortcut("Sound Recorder" sndrec32.exe "Profiles/All Users/Start Menu/Programs/Accessories/Entertainment")
add_livecd_shortcut("Volume Control" sndvol32.exe "Profiles/All Users/Start Menu/Programs/Accessories/Entertainment")
add_livecd_shortcut("Character Map" charmap.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("Clipboard Viewer" clipbrd.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("Keyboard Layout Switcher" kbswitch.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("ReactX Diagnostic" dxdiag.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("Registry Editor" regedit.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("Task Manager" taskmgr.exe "Profiles/All Users/Start Menu/Programs/Accessories/System Tools")
add_livecd_shortcut("Calculator" calc.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Paint" mspaint.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Notepad" notepad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("WordPad" wordpad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Solitaire" sol.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("Spider Solitaire" spider.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("WineMine" winmine.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("ReactOS Explorer" explorer.exe "Profiles/All Users/Start Menu/Programs")
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/Default User/Application Data/Microsoft/Internet Explorer/Quick Launch")
add_livecd_shortcut("ReactOS Explorer" explorer.exe "Profiles/Default User/Application Data/Microsoft/Internet Explorer/Quick Launch")

add_custom_target(livecd_links DEPENDS ${LIVECD_SHORTCUTS})
