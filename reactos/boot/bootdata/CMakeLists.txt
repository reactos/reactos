
add_subdirectory(packages)

add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/txtsetup.sif DESTINATION reactos NO_CAB FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivecls_${ARCH}.inf DESTINATION reactos NO_CAB NAME_ON_CD hivecls.inf FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivedef_${ARCH}.inf DESTINATION reactos NO_CAB NAME_ON_CD hivedef.inf FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivesft_${ARCH}.inf DESTINATION reactos NO_CAB NAME_ON_CD hivesft.inf FOR bootcd regtest)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/hivesys_${ARCH}.inf DESTINATION reactos NO_CAB NAME_ON_CD hivesys.inf FOR bootcd regtest)

# livecd hives
list(APPEND CD_HIVES
    ${CMAKE_CURRENT_SOURCE_DIR}/hivecls_${ARCH}.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivedef_${ARCH}.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivesft_${ARCH}.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hivesys_${ARCH}.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/livecd.inf
    ${CMAKE_CURRENT_SOURCE_DIR}/hiveinst_${ARCH}.inf)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sam
        ${CMAKE_CURRENT_BINARY_DIR}/default
        ${CMAKE_CURRENT_BINARY_DIR}/security
        ${CMAKE_CURRENT_BINARY_DIR}/software
        ${CMAKE_CURRENT_BINARY_DIR}/system
    COMMAND native-mkhive ${CMAKE_CURRENT_BINARY_DIR} ${CD_HIVES}
    DEPENDS native-mkhive ${CD_HIVES})

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
