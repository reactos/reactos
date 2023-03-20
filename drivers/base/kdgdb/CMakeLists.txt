
spec2def(kdgdb.dll kdgdb.spec)

list(APPEND SOURCE
    gdb_input.c
    gdb_receive.c
    gdb_send.c
    kdcom.c
    kdpacket.c
    utils.c
    kdgdb.h)

# TODO: ARM...
if(ARCH STREQUAL "i386")
    list(APPEND SOURCE i386_sup.c)
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE amd64_sup.c)
endif()

add_library(kdgdb MODULE
    ${SOURCE}
    kdgdb.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdgdb.def)
add_pch(kdgdb kdgdb.h SOURCE)

set_module_type(kdgdb kerneldll ENTRYPOINT 0)
set_subsystem(kdgdb native)
add_importlibs(kdgdb ntoskrnl hal)
target_link_libraries(kdgdb cportlib strtol)
if(GDB) ## See PR #5187
    set(KDGDB_NAME_ON_CD kdcom.dll)
else()
    set(KDGDB_NAME_ON_CD "$<TARGET_FILE_NAME:kdgdb>")
endif()
add_cd_file(TARGET kdgdb DESTINATION reactos/system32 NO_CAB NAME_ON_CD KDGDB_NAME_ON_CD FOR all)
