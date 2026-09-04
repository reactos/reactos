
spec2def(mmcshex.dll mmcshex.spec)

list(APPEND SOURCE
    mmcshex.cpp)

add_library(mmcshex MODULE
    ${SOURCE}
    mmcshex.rc
    mmcshex.spec
    ${CMAKE_CURRENT_BINARY_DIR}/mmcshex.def)

set_module_type(mmcshex win32dll UNICODE)
target_link_libraries(mmcshex uuid cpprt atl_classes)
set_target_cpp_properties(mmcshex WITH_EXCEPTIONS)
add_importlibs(mmcshex oleaut32 ole32 shlwapi comctl32 shell32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(mmcshex precomp.h SOURCE)
add_cd_file(TARGET mmcshex DESTINATION reactos/system32 FOR all)
