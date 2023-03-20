
spec2def(kdcom.dll kdcom.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdcom.c
    kddll.c
    kdserial.c
    kddll.h)

add_library(kdcom MODULE
    ${SOURCE}
    kdcom.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdcom.def)
add_pch(kdcom kddll.h SOURCE)

set_module_type(kdcom kerneldll ENTRYPOINT 0)
set_subsystem(kdcom native)
add_importlibs(kdcom ntoskrnl hal)
target_link_libraries(kdcom cportlib strtol)
add_dependencies(kdcom psdk bugcodes)
if(GDB) ## See PR #5187
    set(KDCOM_NAME_ON_CD kdser.dll)
else()
    set(KDCOM_NAME_ON_CD "$<TARGET_FILE_NAME:kdcom>")
endif()
add_cd_file(TARGET kdcom DESTINATION reactos/system32 NO_CAB NAME_ON_CD KDCOM_NAME_ON_CD FOR all)
