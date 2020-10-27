PROJECT(DEVMGR)

spec2def(devmgr.dll devmgr.spec ADD_IMPORTLIB)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/dll
    includes)

list(APPEND SOURCE
    precomp.h
    api.cpp
    properties/advprop.cpp
    properties/devprblm.cpp
    properties/hwpage.cpp
    properties/hwresource.cpp
    properties/misc.cpp
    devmgmt/ClassNode.cpp
    devmgmt/DeviceNode.cpp
    devmgmt/DeviceView.cpp
    devmgmt/MainWindow.cpp
    devmgmt/Node.cpp
    devmgmt/RootNode.cpp)

add_library(devmgr MODULE
    ${SOURCE}
    devmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

set_module_type(devmgr win32dll UNICODE)
target_link_libraries(devmgr uuid wine cpprt atl_classes)
set_target_cpp_properties(devmgr WITH_EXCEPTIONS WITH_RTTI)
add_importlibs(devmgr setupapi advapi32 shell32 newdev user32 gdi32 comctl32 version msvcrt kernel32 ole32 oleaut32 uxtheme ntdll)
add_pch(devmgr precomp.h SOURCE)
add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)
