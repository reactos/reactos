PROJECT(DEVMGR)

spec2def(devmgr.dll devmgr.spec ADD_IMPORTLIB)

set_cpp(WITH_RTTI WITH_RUNTIME)

if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
    add_compile_flags("-fexceptions")
endif()

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/dll
    ${REACTOS_SOURCE_DIR}/lib/atl 
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

add_library(devmgr SHARED
    ${SOURCE}
    devmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

set_module_type(devmgr win32dll UNICODE)
target_link_libraries(devmgr uuid atlnew wine)
add_importlibs(devmgr setupapi advapi32 newdev user32 gdi32 comctl32 version msvcrt kernel32 ole32 uxtheme ntdll)
add_pch(devmgr precomp.h SOURCE)
add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)
