set_cpp(WITH_RUNTIME)
spec2def(mydocs.dll mydocs.spec)

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

file(GLOB_RECURSE mydocs_rc_deps res/*.*)
add_rc_deps(mydocs.rc ${mydocs_rc_deps})

list(APPEND SOURCE
    CMyDocsDropHandler.cpp
    mydocs.cpp
    precomp.hpp)

add_library(mydocs MODULE
    ${SOURCE}
    mydocs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mydocs.def)

set_module_type(mydocs win32dll UNICODE)
target_link_libraries(mydocs uuid wine)
add_delay_importlibs(mydocs ole32 oleaut32)
add_importlibs(mydocs advapi32 shell32 user32 comctl32 shlwapi msvcrt kernel32 ntdll)
add_pch(mydocs precomp.hpp SOURCE)
add_cd_file(TARGET mydocs DESTINATION reactos/system32 FOR all)
