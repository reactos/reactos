PROJECT(CHARMAP)

include_directories(
    includes)

list(APPEND SOURCE
    precomp.h
    MainWindow.cpp
	)

add_library(charmap MODULE
    ${SOURCE}
    charmap.rc)

set_module_type(charmap win32gui UNICODE)
target_link_libraries(charmap uuid wine cpprt atl_classes)
set_target_cpp_properties(charmap WITH_EXCEPTIONS WITH_RTTI)
add_importlibs(charmap advapi32 user32 gdi32 comctl32 version msvcrt kernel32 ole32 uxtheme ntdll)
add_pch(charmap precomp.h SOURCE)
add_cd_file(TARGET charmap DESTINATION reactos/system32 FOR all)
