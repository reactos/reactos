
spec2def(mmcshext.dll mmcshext.spec)

list(APPEND SOURCE
    mmcshext.cpp)

add_library(mmcshext MODULE
    ${SOURCE}
    mmcshext.rc
    mmcshext.spec
    ${CMAKE_CURRENT_BINARY_DIR}/mmcshext.def)

set_module_type(mmcshext win32dll UNICODE)
target_link_libraries(mmcshext uuid cpprt atl_classes)
set_target_cpp_properties(mmcshext WITH_EXCEPTIONS)
add_importlibs(mmcshext oleaut32 ole32 shlwapi comctl32 shell32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(mmcshext precomp.h SOURCE)
add_cd_file(TARGET mmcshext DESTINATION reactos/system32 FOR all)
