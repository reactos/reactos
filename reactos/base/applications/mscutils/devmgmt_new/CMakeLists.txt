project(DEVMGMT_NEW)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    Devices.cpp
    DeviceView.cpp
    devmgmt.cpp
    MainWindow.cpp
    precomp.h)

add_executable(devmgmt_new ${SOURCE} devmgmt.rc)
target_link_libraries(devmgmt_new uuid atlnew wine)
set_module_type(devmgmt_new win32gui UNICODE)
add_importlibs(devmgmt_new setupapi gdi32 user32 comctl32 advapi32 ole32 devmgr msvcrt uxtheme kernel32)
add_pch(devmgmt_new precomp.h SOURCE)
add_cd_file(TARGET devmgmt_new DESTINATION reactos/system32 FOR all)
