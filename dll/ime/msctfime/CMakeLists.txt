
include_directories(
    ${REACTOS_SOURCE_DIR}/win32ss/include)

spec2def(msctfime.ime msctfime.spec)

list(APPEND SOURCE
    msctfime.c)

file(GLOB msctfime_rc_deps res/*.*)
add_rc_deps(msctfime.rc ${msctfime_rc_deps})

add_library(msctfime MODULE
    ${SOURCE}
    msctfime.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msctfime.def)
set_module_type(msctfime win32dll UNICODE)
set_target_properties(msctfime PROPERTIES SUFFIX ".ime")
target_link_libraries(msctfime wine uuid)
add_importlibs(msctfime user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msctfime DESTINATION reactos/system32 FOR all)
