
spec2def(kdcom.dll kdrosdbg.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdrosdbg.c
    kdrosdbg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)

if(KDBG)
    add_definitions(-DKDBG)
endif()

add_library(kdcom MODULE ${SOURCE})
set_module_type(kdcom kerneldll ENTRYPOINT 0)
add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib libcntpr)
add_dependencies(kdcom psdk)
add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB FOR all)
