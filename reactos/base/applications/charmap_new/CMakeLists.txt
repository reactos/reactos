PROJECT(CHARMAP)

set_cpp(WITH_RTTI WITH_RUNTIME WITH_EXCEPTIONS)

if(NOT MSVC)
    # HACK: this should be enabled globally!
    add_compile_flags_language("-std=c++11" "CXX")
endif()

include_directories(
    ${REACTOS_SOURCE_DIR}/lib/atl 
    includes)

list(APPEND SOURCE
    precomp.h
    MainWindow.cpp
	)

add_library(charmap SHARED
    ${SOURCE}
    charmap.rc)

set_module_type(charmap win32gui UNICODE)
target_link_libraries(charmap uuid atlnew wine)
add_importlibs(charmap advapi32 user32 gdi32 comctl32 version msvcrt kernel32 ole32 uxtheme ntdll)
add_pch(charmap precomp.h SOURCE)
add_cd_file(TARGET charmap DESTINATION reactos/system32 FOR all)
