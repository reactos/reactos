project(appcompat)

set_cpp(WITH_RUNTIME)
if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

add_definitions(
    -D_ATL_NO_EXCEPTIONS)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp
    ${REACTOS_SOURCE_DIR})
spec2def(acppage.dll acppage.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ACPPage.cpp
    CLayerUIPropPage.cpp
    CLayerUIPropPage.hpp
    CLayerStringList.hpp
    acppage.spec
    precomp.h
    resource.h)

add_library(acppage SHARED
    ${SOURCE}
    ACPPage.rc
    ${CMAKE_CURRENT_BINARY_DIR}/acppage.def)

set_module_type(acppage win32dll UNICODE)
target_link_libraries(acppage atlnew wine uuid)
add_delay_importlibs(acppage apphelp ole32 oleaut32 shlwapi comctl32 sfc_os)
add_importlibs(acppage shell32 user32 advapi32 msvcrt kernel32)
add_pch(acppage precomp.h SOURCE)
add_cd_file(TARGET acppage DESTINATION reactos/system32 FOR all)

