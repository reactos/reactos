
add_cabinet(reactos ${CMAKE_CURRENT_SOURCE_DIR}/reactos.dff.in)

add_iso(bootcd EXCLUDE_FROM_ALL)
add_iso(livecd EXCLUDE_FROM_ALL)
add_iso(hybridcd EXCLUDE_FROM_ALL)
add_iso(regtest EXCLUDE_FROM_ALL)

add_custom_target(bootcd DEPENDS bootcd_iso)
add_custom_target(livecd DEPENDS livecd_iso)
add_custom_target(hybridcd DEPENDS hybridcd_iso)
add_custom_target(bootcdregtest DEPENDS regtest_iso)

add_cd_file(
    TARGET reactos_cab
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.cab
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)

add_cd_file(
    TARGET reactos_cab
    FILE ${CMAKE_CURRENT_BINARY_DIR}/reactos.inf
    DESTINATION reactos
    NO_CAB FOR bootcd regtest)

add_cd_file(
    FILE ${CMAKE_CURRENT_BINARY_DIR}/livecd.iso
    DESTINATION livecd
    FOR hybridcd)

function(_add_empty_folders _target _prefix)
    # Create the empty Desktop, Favorites, and Start Menu folders. And many more.
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Application Data")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Documents/My Music")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Documents/My Pictures")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Documents/My Videos")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Favorites")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/My Documents")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Start Menu/Programs/StartUp")
    add_iso_folder(${_target} "${_prefix}Profiles/All Users/Templates")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Application Data")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Application Data/Microsoft/Internet Explorer/Quick Launch")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Cookies")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Desktop")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Favorites")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Local Settings/Application Data")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Local Settings/History")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Local Settings/Temporary Internet Files")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/My Music")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/My Pictures")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/My Videos")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/NetHood")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/PrintHood")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Recent")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/SendTo")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Start Menu/Programs")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Start Menu/Programs/Administrative Tools")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Start Menu/Programs/StartUp")
    add_iso_folder(${_target} "${_prefix}Profiles/Default User/Templates")
endfunction()

_add_empty_folders(livecd "")
_add_empty_folders(hybridcd "livecd/")
