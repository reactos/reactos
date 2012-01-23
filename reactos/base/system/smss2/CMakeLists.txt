
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    smss.c
    smss.rc)

add_executable(smss2 WIN32 ${SOURCE})

target_link_libraries(smss2 nt)

add_pch(smss2 smss.h)

set_module_type(smss2 nativecui)
add_importlibs(smss2 ntdll)
add_cd_file(TARGET smss2 DESTINATION reactos/system32 NO_CAB FOR all)
