
add_definitions(
    -D_NTOSKRNL_
    -D_NTSYSTEM_
    -DNASSERT)

list(APPEND SOURCE
    cminit.c
    cmtools.c
    hivebin.c
    hivecell.c
    hiveinit.c
    hivesum.c
    hivewrt.c
    cmlib.h)

if(CMAKE_CROSSCOMPILING)
    add_library(cmlib ${SOURCE})
    add_dependencies(cmlib bugcodes xdk)
    add_pch(cmlib cmlib.h SOURCE)
else()
    add_definitions(
        -D__NO_CTYPE_INLINES
        -DCMLIB_HOST)
    add_library(cmlibhost ${SOURCE})
    target_link_libraries(cmlibhost unicode)
endif()
