
add_definitions(
    -D_WINE
    -D__ROS_LONG64__
    -D_COMCTL32_
    -D__WINE_COMCTL32_VERSION_MINOR=0)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600 -D__WINE_COMCTL32_VERSION=6)

spec2def(comctl32_v6.dll comctl32_v6.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/dll/win32/comctl32)

if(MSVC)

endif()

list(APPEND SOURCE
    ../comctl32/animate.c
    button.c
    combo.c
    ../comctl32/comboex.c
    ../comctl32/commctrl.c
    ../comctl32/datetime.c
    ../comctl32/dpa.c
    ../comctl32/draglist.c
    ../comctl32/dsa.c
    edit.c
    ../comctl32/flatsb.c
    ../comctl32/header.c
    ../comctl32/hotkey.c
    ../comctl32/imagelist.c
    ../comctl32/ipaddress.c
    listbox.c
    ../comctl32/listview.c
    ../comctl32/monthcal.c
    ../comctl32/nativefont.c
    ../comctl32/pager.c
    ../comctl32/progress.c
    ../comctl32/propsheet.c
    ../comctl32/rebar.c
    ../comctl32/smoothscroll.c
    static.c
    ../comctl32/status.c
    ../comctl32/string.c
    syslink.c
    ../comctl32/tab.c
    taskdialog.c
    ../comctl32/toolbar.c
    ../comctl32/tooltips.c
    ../comctl32/trackbar.c
    ../comctl32/treeview.c
    ../comctl32/updown.c)

list(APPEND PCH_SKIP_SOURCE
    ../comctl32/stubs.c)

add_library(comctl32_v6 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ../comctl32/comctl32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/comctl32_v6.def)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    # Disable warning C4267: '=': conversion from 'size_t' to 'WORD', possible loss of data
    # Disable warning C4477 (printf format warnings)
    target_compile_options(comctl32_v6 PRIVATE /wd4146 /wd4267 /wd4477)
endif()

set_module_type(comctl32_v6 win32dll UNICODE)
target_link_libraries(comctl32_v6 uuid wine wine_dll_version ${PSEH_LIB} oldnames)
add_delay_importlibs(comctl32_v6 oleacc oleaut32 winmm uxtheme uxtheme_ros)
add_importlibs(comctl32_v6 user32 gdi32 advapi32 usp10 imm32 msvcrt kernelbase_ros kernel32_vista ntdll)
set_target_properties(comctl32_v6 PROPERTIES OUTPUT_NAME "comctl32")
add_pch(comctl32_v6 precomp.h "${PCH_SKIP_SOURCE}")

add_cd_file(TARGET comctl32_v6 DESTINATION reactos/winsxs/${WINARCH}_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/${WINARCH}_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef.manifest DESTINATION reactos/winsxs/manifests FOR all)
set_wine_module_FIXME(comctl32_v6) # CORE-5743: No ARRAY_SIZE macro
