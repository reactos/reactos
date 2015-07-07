
spec2def(devmgr.dll devmgr.spec ADD_IMPORTLIB)

set_cpp(WITH_RTTI WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)

list(APPEND SOURCE
    precomp.h
    properties/advprop.c
    properties/devprblm.c
    properties/hwpage.c
    properties/hwresource.c
    properties/misc.c
    properties/stubs.c 
    devmgmt/ClassNode.cpp
    devmgmt/DeviceNode.cpp
    devmgmt/DeviceView.cpp
    devmgmt/MainWindow.cpp
    devmgmt/Node.cpp
    devmgmt/RootNode.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr_stubs.c)

add_library(devmgr SHARED
    ${SOURCE}
    devmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

set_module_type(devmgr win32dll UNICODE)
target_link_libraries(devmgr uuid atlnew wine)
add_importlibs(devmgr setupapi advapi32 newdev user32 gdi32 comctl32 version msvcrt kernel32 ole32 uxtheme ntdll)
add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)
