
spec2def(msgina.dll msgina.spec)

list(APPEND SOURCE
    gui.c
    lsa.c
    msgina.c
    shutdown.c
    stubs.c
    tui.c
    utils.c)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND PCH_SKIP_SOURCE dimmedwindow.cpp)
endif()

list(APPEND PCH_SKIP_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/msgina_stubs.c)

add_library(msgina MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    msgina.rc
    ${CMAKE_CURRENT_BINARY_DIR}/msgina.def)

set_module_type(msgina win32dll UNICODE)
target_link_libraries(msgina wine uuid ${PSEH_LIB})
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(msgina cpprt atl_classes)
endif()
add_delay_importlibs(msgina secur32)
add_importlibs(msgina advapi32 user32 gdi32 userenv msvcrt kernel32 ntdll)
add_pch(msgina msgina.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET msgina DESTINATION reactos/system32 FOR all)
