
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    crashdmp.c
    pagefile.c
    sminit.c
    smloop.c
    smsbapi.c
    smsessn.c
    smsubsys.c
    smutil.c
    smss.c
    smss.rc)

add_executable(smss WIN32 ${SOURCE})
target_link_libraries(smss nt ${PSEH_LIB} smlib)
add_pch(smss smss.h)
set_module_type(smss nativecui)
add_importlibs(smss ntdll)
add_cd_file(TARGET smss DESTINATION reactos/system32 FOR all)
