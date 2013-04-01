
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(qedit.dll qedit.spec)

add_library(qedit SHARED
    main.c
    mediadet.c
    regsvr.c
    samplegrabber.c
    ${CMAKE_CURRENT_BINARY_DIR}/qedit.def)

set_module_type(qedit win32dll UNICODE)

target_link_libraries(qedit
    strmiids
    uuid
    wine)

add_importlibs(qedit
    msvcrt
    advapi32
    ole32
    oleaut32
    kernel32
    ntdll)

add_pch(qedit qedit_private.h)
add_dependencies(qedit dxsdk)
add_cd_file(TARGET qedit DESTINATION reactos/system32 FOR all)
