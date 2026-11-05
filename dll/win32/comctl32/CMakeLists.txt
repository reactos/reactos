
add_definitions(
    -D_WINE
    -D_COMCTL32_
    -D__WINE_COMCTL32_VERSION=5
    -D__WINE_COMCTL32_VERSION_MINOR=82)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

spec2def(comctl32.dll comctl32.spec ADD_IMPORTLIB)

if(MSVC)

endif()

list(APPEND SOURCE
    animate.c
    comboex.c
    commctrl.c
    datetime.c
    dpa.c
    draglist.c
    dsa.c
    flatsb.c
    header.c
    hotkey.c
    imagelist.c
    ipaddress.c
    listview.c
    monthcal.c
    nativefont.c
    pager.c
    progress.c
    propsheet.c
    rebar.c
    smoothscroll.c
    status.c
    string.c
    tab.c
    toolbar.c
    tooltips.c
    trackbar.c
    treeview.c
    updown.c)

list(APPEND PCH_SKIP_SOURCE
    stubs.c)

add_library(comctl32 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    comctl32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comctl32.def)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    # Disable warning C4267: '=': conversion from 'size_t' to 'WORD', possible loss of data
    # Disable warning C4477 (printf format warnings)
    target_compile_options(comctl32 PRIVATE /wd4146 /wd4267 /wd4477)
endif()

set_module_type(comctl32 win32dll UNICODE)
target_link_libraries(comctl32 uuid wine wine_dll_version ${PSEH_LIB} oldnames)
add_delay_importlibs(comctl32 winmm)
add_importlibs(comctl32 user32 gdi32 advapi32 usp10 imm32 msvcrt kernelbase_ros kernel32_vista ntdll)
add_pch(comctl32 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET comctl32 DESTINATION reactos/system32 FOR all)

add_cd_file(TARGET comctl32 DESTINATION reactos/winsxs/${WINARCH}_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${WINARCH}_microsoft.windows.common-controls_6595b64144ccf1df_5.82.2600.2982_none_deadbeef.manifest DESTINATION reactos/winsxs/manifests FOR all)
set_wine_module_FIXME(comctl32) # CORE-5743: No ARRAY_SIZE macro
