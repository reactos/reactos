
spec2def(msctfime.ime msctfime.spec)

list(APPEND SOURCE
    bridge.cpp
    inputcontext.cpp
    misc.cpp
    msctfime.cpp
    profile.cpp
    sinks.cpp
    tls.cpp
    ui.cpp)

file(GLOB msctfime_rc_deps res/*.*)
add_rc_deps(msctfime.rc ${msctfime_rc_deps})

add_library(msctfime MODULE
    ${SOURCE}
    msctfime.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctfime.def)
set_module_type(msctfime win32dll UNICODE)
set_target_properties(msctfime PROPERTIES SUFFIX ".ime")
target_link_libraries(msctfime cpprt wine uuid cicero)
add_importlibs(msctfime user32 gdi32 advapi32 msvcrt kernel32 ntdll)
add_delay_importlibs(msctfime uxtheme shlwapi comctl32 msctf oleaut32 imm32)
add_cd_file(TARGET msctfime DESTINATION reactos/system32 FOR all)
