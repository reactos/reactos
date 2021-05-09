spec2def(sendmail.dll sendmail.spec)

add_definitions(
    -D_WINE)

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
target_link_libraries(sendmail uuid wine cpprt atl_classes)
add_delay_importlibs(sendmail ole32 oleaut32)
add_importlibs(sendmail advapi32 shell32 user32 comctl32 shlwapi msvcrt kernel32 ntdll)
add_pch(sendmail precomp.hpp SOURCE)
add_cd_file(TARGET sendmail DESTINATION reactos/system32 FOR all)
