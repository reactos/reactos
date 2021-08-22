project(iernonce)

include_directories(include)

spec2def(iernonce.dll iernonce.spec)

list(APPEND SOURCE
    dialog.cpp
    iernonce.cpp
    registry.cpp
    iernonce.h)

add_library(iernonce MODULE
    ${SOURCE}
    iernonce.rc
    ${CMAKE_CURRENT_BINARY_DIR}/iernonce.def)

set_module_type(iernonce win32dll UNICODE)
target_link_libraries(iernonce cppstl atl_classes)
set_target_cpp_properties(iernonce WITH_EXCEPTIONS)
add_importlibs(iernonce advapi32 msvcrt gdi32 ole32 shell32 shlwapi kernel32 user32 ntdll)
add_pch(iernonce iernonce.h SOURCE)
add_cd_file(TARGET iernonce DESTINATION reactos/system32 FOR all)
