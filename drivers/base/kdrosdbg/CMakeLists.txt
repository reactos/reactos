
list(APPEND SOURCE
    kdrosdbg.c
    kdrosdbg.rc)

add_library(kdcom MODULE ${SOURCE})
spec2def(kdcom kdcom.dll kdrosdbg.spec ADD_IMPORTLIB)

set_module_type(kdcom kerneldll ENTRYPOINT 0)
add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib)
add_dependencies(kdcom psdk)
add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB FOR all)
