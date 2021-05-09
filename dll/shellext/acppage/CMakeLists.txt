
project(appcompat)

include_directories(
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp
    ${REACTOS_SOURCE_DIR})
spec2def(acppage.dll acppage.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ACPPage.cpp
    CEditCompatModes.cpp
    CLayerUIPropPage.cpp
    precomp.h)

add_library(acppage MODULE
    ${SOURCE}
    acppage.spec
    ACPPage.rc
    CLayerUIPropPage.hpp
    CLayerStringList.hpp
    resource.h
    ${CMAKE_CURRENT_BINARY_DIR}/acppage.def)

set_module_type(acppage win32dll UNICODE)
target_link_libraries(acppage wine uuid cpprt atl_classes)
add_delay_importlibs(acppage apphelp ole32 oleaut32 shlwapi comctl32 sfc_os)
add_importlibs(acppage shell32 user32 advapi32 msvcrt kernel32)
add_pch(acppage precomp.h SOURCE)
add_cd_file(TARGET acppage DESTINATION reactos/system32 FOR all)
