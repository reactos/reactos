
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
    ${REACTOS_SOURCE_DIR}
    ${REACTOS_BINARY_DIR})

spec2def(acppage.dll acppage.spec ADD_IMPORTLIB)

# The 'parent' RC file where all resources will be included
add_translatable_rc(ACPPage.rc)

add_translations(TEMPLATE template TRANSLATIONS 
    cs-CZ
    de-DE
    en-US
    es-ES
    eu-ES
    fr-FR
    hu-HU
    id-ID
    it-IT
    nl-NL
    no-NO
    pl-PL
    ro-RO
    ru-RU
    sk-SK
    sq-AL
    sv-SE
    tr-TR
    uk-UA
    zh-CN
    zh-TW)

# bg-BG and he-IL are included like before, since they have different dimensions / options.
# If we add support to automatically add RTL options to the dialog, we could include he-IL in the po files as well.
# If there are multiple languages that need a changed themplate, we can just repeat the 'add_translations' command with a different template.
# add_translations(TEMPLATE template_rtl TRANSLATIONS he-IL)


list(APPEND SOURCE
    ACPPage.cpp
    CLayerUIPropPage.cpp
    precomp.h)

add_library(acppage SHARED
    ${SOURCE}
    acppage.spec
    ACPPage.rc
    CLayerUIPropPage.hpp
    CLayerStringList.hpp
    resource.h
    ${CMAKE_CURRENT_BINARY_DIR}/acppage.def)

set_module_type(acppage win32dll UNICODE)
target_link_libraries(acppage atlnew wine uuid)
add_delay_importlibs(acppage apphelp ole32 oleaut32 shlwapi comctl32 sfc_os)
add_importlibs(acppage shell32 user32 advapi32 msvcrt kernel32)
add_pch(acppage precomp.h SOURCE)
add_cd_file(TARGET acppage DESTINATION reactos/system32 FOR all)
