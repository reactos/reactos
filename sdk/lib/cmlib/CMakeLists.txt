
add_definitions(
    -D_NTSYSTEM_
    -DNASSERT)

list(APPEND SOURCE
    cminit.c
    cmindex.c
    cmkeydel.c
    cmname.c
    cmse.c
    cmvalue.c
    hivebin.c
    hivecell.c
    hiveinit.c
    hivesum.c
    hivewrt.c
    cmlib.h)

if(CMAKE_CROSSCOMPILING)
    add_library(cmlib ${SOURCE})
    add_dependencies(cmlib bugcodes xdk)
    add_pch(cmlib cmlib.h SOURCE NO_UNITY_BUILD)
else()
    add_definitions(
        -D__NO_CTYPE_INLINES
        -DCMLIB_HOST)
    add_library(cmlibhost ${SOURCE})

    if(NOT MSVC)
        add_target_compile_flags(cmlibhost "-fshort-wchar -Wno-multichar")
    endif()

    target_link_libraries(cmlibhost unicode)
endif()
