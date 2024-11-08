
spec2def(cabview.dll cabview.spec)

list(APPEND SOURCE
    cabview.cpp
    extract.cpp
    folder.cpp
    resource.h)

add_library(cabview MODULE
    ${SOURCE}
    cabview.spec
    cabview.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cabview.def)

set_module_type(cabview win32dll UNICODE)
target_link_libraries(cabview uuid cpprt atl_classes)
set_target_cpp_properties(cabview WITH_EXCEPTIONS)
add_importlibs(cabview cabinet oleaut32 ole32 shlwapi comctl32 shell32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(cabview precomp.h SOURCE)
add_cd_file(TARGET cabview DESTINATION reactos/system32 FOR all)
