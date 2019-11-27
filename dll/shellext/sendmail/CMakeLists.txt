set_cpp(WITH_RUNTIME)
spec2def(sendmail.dll sendmail.spec)

add_definitions(
    -D_WINE
    -D_ATL_NO_EXCEPTIONS)

if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/atl
    ${REACTOS_SOURCE_DIR})

file(GLOB_RECURSE sendmail_rc_deps res/*.*)
add_rc_deps(sendmail.rc ${sendmail_rc_deps})

list(APPEND SOURCE
    CDeskLinkDropHandler.cpp
    sendmail.cpp
    precomp.hpp)

add_library(sendmail MODULE
    ${SOURCE}
    sendmail.rc
    ${CMAKE_CURRENT_BINARY_DIR}/sendmail.def)

set_module_type(sendmail win32dll UNICODE)
target_link_libraries(sendmail uuid wine)
add_delay_importlibs(sendmail ole32 oleaut32)
add_importlibs(sendmail advapi32 shell32 user32 comctl32 shlwapi msvcrt kernel32 ntdll)
add_pch(sendmail precomp.hpp SOURCE)
add_cd_file(TARGET sendmail DESTINATION reactos/system32 FOR all)
