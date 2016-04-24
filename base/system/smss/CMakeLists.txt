
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

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
    smss.h)

add_executable(smss WIN32 ${SOURCE} smss.rc)
target_link_libraries(smss nt ${PSEH_LIB} smlib)
set_module_type(smss nativecui)
add_importlibs(smss ntdll)
add_pch(smss smss.h SOURCE)
add_cd_file(TARGET smss DESTINATION reactos/system32 FOR all)
