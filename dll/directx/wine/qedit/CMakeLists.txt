
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(qedit.dll qedit.spec)

list(APPEND SOURCE
    main.c
    mediadet.c
    samplegrabber.c
    timeline.c
    qedit_private.h)

add_library(qedit SHARED
    ${SOURCE}
    qedit.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qedit.def)

set_module_type(qedit win32dll)
target_link_libraries(qedit strmbase strmiids uuid wine)
add_importlibs(qedit ole32 oleaut32 msvcrt kernel32 ntdll)
add_pch(qedit qedit_private.h SOURCE NO_UNITY_BUILD)
add_cd_file(TARGET qedit DESTINATION reactos/system32 FOR all)
