
spec2def(kdcom.dll kdgdb.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    gdb_input.c
    gdb_receive.c
    gdb_send.c
    kdcom.c
    kdpacket.c
    utils.c
    kdgdb.h)

# TODO: AMD64, ARM...
if(ARCH STREQUAL "i386")
    list(APPEND SOURCE i386_sup.c)
endif()

add_library(kdcom MODULE
    ${SOURCE}
    kdgdb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

set_module_type(kdcom module IMAGEBASE 0x00010000)
set_subsystem(kdcom native)
add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib)
add_pch(kdcom kdgdb.h SOURCE)
add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB FOR all)
