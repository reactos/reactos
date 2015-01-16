
add_subdirectory(packages)

#common hives

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/txtsetup.sif DESTINATION reactos NO_CAB FOR bootcd regtest)

list(APPEND _common_hives
    ${CMAKE_CURRENT_SOURCE_DIR}/hivecls.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivedef.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivesft.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivesys.inf)

foreach(_hive ${_common_hives})
    get_filename_component(_hive_name ${_hive} NAME)
    set(_converted_hive ${CMAKE_CURRENT_BINARY_DIR}/${_hive_name})
    add_custom_command(OUTPUT ${_converted_hive}
                       COMMAND native-utf16le ${_hive} ${_converted_hive}
                       DEPENDS native-utf16le ${_hive})
    list(APPEND _converted_common_hives ${_converted_hive})
endforeach()

add_custom_target(converted_hives DEPENDS ${_converted_common_hives})
add_cd_file(TARGET converted_hives
            FILE ${_converted_common_hives}
            DESTINATION reactos
            NO_CAB
            FOR bootcd regtest)

# livecd hives
list(APPEND _livecd_hives
    ${_converted_common_hives}
    ${CMAKE_CURRENT_SOURCE_DIR}/livecd.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hiveinst.inf)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sam
        ${CMAKE_CURRENT_BINARY_DIR}/default
        ${CMAKE_CURRENT_BINARY_DIR}/security
        ${CMAKE_CURRENT_BINARY_DIR}/software
        ${CMAKE_CURRENT_BINARY_DIR}/system
    COMMAND native-mkhive ${CMAKE_CURRENT_BINARY_DIR} ${_livecd_hives}
    DEPENDS native-mkhive ${_livecd_hives} ${_converted_common_hives})

add_custom_target(livecd_hives
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/sam
        ${CMAKE_CURRENT_BINARY_DIR}/default
        ${CMAKE_CURRENT_BINARY_DIR}/security
        ${CMAKE_CURRENT_BINARY_DIR}/software
        ${CMAKE_CURRENT_BINARY_DIR}/system)

add_cd_file(
    FILE ${CMAKE_CURRENT_BINARY_DIR}/sam
        ${CMAKE_CURRENT_BINARY_DIR}/default
        ${CMAKE_CURRENT_BINARY_DIR}/security
        ${CMAKE_CURRENT_BINARY_DIR}/software
        ${CMAKE_CURRENT_BINARY_DIR}/system
    TARGET livecd_hives
    DESTINATION reactos/system32/config
    FOR livecd)

#regtest
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/regtest.cmd DESTINATION reactos/bin FOR all)

#autorun.inf
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/autorun.inf DESTINATION root NO_CAB FOR all)

#icon.ico
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico DESTINATION root NO_CAB FOR all)

#readme.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION root NO_CAB FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION reactos FOR all)

#system.ini
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/system.ini DESTINATION reactos FOR all)

#freeldr.ini
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd.ini DESTINATION root NO_CAB NAME_ON_CD freeldr.ini FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/livecd.ini DESTINATION root NAME_ON_CD freeldr.ini FOR livecd)

#unattend
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/unattend.inf DESTINATION reactos NO_CAB FOR regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd/unattend.inf DESTINATION reactos NO_CAB FOR bootcd)

#LiveCD shortcuts
macro(add_livecd_shortcut name app dest)
    add_link(NAME ${name} CMD_LINE_ARGS ${app} ICON ${app} PATH livecd_start.cmd GUID "{450D8FBA-AD25-11D0-98A8-0800361B1103}" MINIMIZE)
    list(APPEND LIVECD_SHORTCUTS "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk")
    add_cd_file(FILE "${CMAKE_CURRENT_BINARY_DIR}/${name}.lnk"
                TARGET livecd_links
                DESTINATION ${dest}
                FOR livecd)
endmacro(add_livecd_shortcut)

add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/livecd_start.cmd" DESTINATION "Profiles/Default User/My Documents" FOR livecd)
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/All Users/Desktop")
add_livecd_shortcut("Device Manager" devmgmt.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Event Viewer" eventvwr.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Service Manager" servman.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("System Configuration" msconfig.exe "Profiles/All Users/Start Menu/Programs/Administrative Tools")
add_livecd_shortcut("Magnify" magnify.exe "Profiles/All Users/Start Menu/Programs/Accessibility")
add_livecd_shortcut("Calculator" calc.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Paint" mspaint.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Notepad" notepad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Remote desktop" mstsc.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("WordPad" wordpad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Audiorecorder" sndrec32.exe "Profiles/All Users/Start Menu/Programs/Entertainment")
add_livecd_shortcut("Multimedia Player" mplay32.exe "Profiles/All Users/Start Menu/Programs/Entertainment")
add_livecd_shortcut("Volume Control" sndvol32.exe "Profiles/All Users/Start Menu/Programs/Entertainment")
add_livecd_shortcut("Solitaire" sol.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("Spider Solitaire" spider.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("WineMine" winmine.exe "Profiles/All Users/Start Menu/Programs/Games")
add_livecd_shortcut("Character Map" charmap.exe "Profiles/All Users/Start Menu/Programs/System Tools")
add_livecd_shortcut("Keyboard Layout Switcher" kbswitch.exe "Profiles/All Users/Start Menu/Programs/System Tools")
add_livecd_shortcut("ReactX Diagnostic" dxdiag.exe "Profiles/All Users/Start Menu/Programs/System Tools")
add_livecd_shortcut("Regedit" regedit.exe "Profiles/All Users/Start Menu/Programs/System Tools")
add_livecd_shortcut("ReactOS Explorer" explorer.exe "Profiles/All Users/Start Menu/Programs")

add_custom_target(livecd_links DEPENDS ${LIVECD_SHORTCUTS})
