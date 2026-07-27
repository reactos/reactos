
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WIN32_WINNT=0x600
    -DWINVER=0x600
    -DUSE_NEW_WINE_REGISTER_RESOURCES)

spec2def(qcap.dll qcap.spec)

list(APPEND SOURCE
    audiorecord.c
    avico.c
    avimux.c
    capturegraph.c
    filewriter.c
    qcap_main.c
    smartteefilter.c
    v4l.c
    vfwcapture.c)

add_library(qcap MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qcap.def)
add_idl_reg_scripts(qcap registry qcap_classes.idl)

set_module_type(qcap win32dll)
target_link_libraries(qcap wine wine_dll_canunload strmbase strmiids uuid oldnames)
add_delay_importlibs(qcap msvfw32)
add_importlibs(qcap
    ole32 oleaut32 winmm user32 gdi32 advapi32
    msvcrt kernel32 ntdll)
add_dependencies(qcap wineheaders)
add_cd_file(TARGET qcap DESTINATION reactos/system32 FOR all)
set_wine_module(qcap)
