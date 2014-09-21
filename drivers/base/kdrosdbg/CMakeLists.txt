
spec2def(kdcom.dll kdrosdbg.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdrosdbg.c
    kdrosdbg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

add_library(kdcom SHARED ${SOURCE})

set_entrypoint(kdcom 0)
set_subsystem(kdcom native)
set_image_base(kdcom 0x00010000)

add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib)
add_dependencies(kdcom psdk bugcodes)

add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB FOR all)
