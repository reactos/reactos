
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(inetcomm.dll inetcomm.spec)

add_library(inetcomm SHARED
    imaptransport.c
    inetcomm_main.c
    internettransport.c
    mimeintl.c
    mimeole.c
    pop3transport.c
    regsvr.c
    smtptransport.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetcomm.def)

set_module_type(inetcomm win32dll)

target_link_libraries(inetcomm
    uuid
    wine)

add_importlibs(inetcomm
    ole32
    oleaut32
    ws2_32
    user32
    advapi32
    kernel32
    ntdll)


add_cab_target(inetcomm 1)
