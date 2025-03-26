
add_definitions(-D__WINESRC__ -D__ROS_LONG64__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(inetcomm.dll inetcomm.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    imaptransport.c
    inetcomm_main.c
    internettransport.c
    mimeintl.c
    mimeole.c
    pop3transport.c
    protocol.c
    smtptransport.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetcomm_stubs.c)

add_library(inetcomm MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    inetcomm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/inetcomm.def)

set_module_type(inetcomm win32dll)
target_link_libraries(inetcomm uuid wine)
add_importlibs(inetcomm ole32 oleaut32 ws2_32 user32 propsys urlmon msvcrt kernel32 ntdll)
add_pch(inetcomm precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET inetcomm DESTINATION reactos/system32 FOR all)
