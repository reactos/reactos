
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(inetcomm.dll inetcomm.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    imaptransport.c
    inetcomm_main.c
    internettransport.c
    mimeintl.c
    mimeole.c
    pop3transport.c
    smtptransport.c
    inetcomm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/inetcomm_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/inetcomm.def)

add_library(inetcomm SHARED ${SOURCE})

set_module_type(inetcomm win32dll)

target_link_libraries(inetcomm uuid wine)

add_importlibs(inetcomm
    ole32
    oleaut32
    ws2_32
    user32
    advapi32
    msvcrt
    kernel32
    ntdll)


add_cd_file(TARGET inetcomm DESTINATION reactos/system32 FOR all)
