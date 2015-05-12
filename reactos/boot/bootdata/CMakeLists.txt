
add_subdirectory(packages)

#common hives

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/txtsetup.sif DESTINATION reactos NO_CAB FOR bootcd regtest)

add_registry_inf(
    hivecls.inf
    hivedef.inf
    hivesft.inf
    hivesys.inf)


#regtest
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcdregtest/regtest.cmd DESTINATION reactos/bin FOR all)

#autorun.inf
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/autorun.inf DESTINATION root NO_CAB NOT_IN_HYBRIDCD FOR all hybridcd)

#icon.ico
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico DESTINATION root NO_CAB NOT_IN_HYBRIDCD FOR all hybridcd)

#readme.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION root NO_CAB NOT_IN_HYBRIDCD FOR all hybridcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/readme.txt DESTINATION reactos FOR all)

#freeldr.ini
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/bootcd.ini DESTINATION root NO_CAB NOT_IN_HYBRIDCD NAME_ON_CD freeldr.ini FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/livecd.ini DESTINATION root NOT_IN_HYBRIDCD NAME_ON_CD freeldr.ini FOR livecd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hybridcd.ini DESTINATION root NAME_ON_CD freeldr.ini FOR hybridcd)

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
add_livecd_shortcut("On-Screen Keyboard" osk.exe "Profiles/All Users/Start Menu/Programs/Accessibility")
add_livecd_shortcut("Calculator" calc.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Command Prompt" cmd.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Paint" mspaint.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Notepad" notepad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Remote desktop" mstsc.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("WordPad" wordpad.exe "Profiles/All Users/Start Menu/Programs/Accessories")
add_livecd_shortcut("Sound Recorder" sndrec32.exe "Profiles/All Users/Start Menu/Programs/Entertainment")
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
