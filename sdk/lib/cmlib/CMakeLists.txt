
add_definitions(
    -D_NTSYSTEM_
    -DNASSERT)

list(APPEND SOURCE
    cminit.c
    cmheal.c
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
    # CMLIB for NT bootloader
    add_library(blcmlib ${SOURCE})
    target_compile_definitions(blcmlib PRIVATE _BLDR_)
    add_dependencies(blcmlib bugcodes xdk)
    add_pch(blcmlib cmlib.h SOURCE)

    # CMLIB for NT kernel
    add_library(cmlib ${SOURCE})
    add_dependencies(cmlib bugcodes xdk)
    add_pch(cmlib cmlib.h SOURCE)
else()
    # CMLIB for host-tools
    add_definitions(
        -D__NO_CTYPE_INLINES
        -DCMLIB_HOST)
    add_library(cmlibhost ${SOURCE})
    target_include_directories(cmlibhost INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

    if(NOT MSVC)
        target_compile_options(cmlibhost PRIVATE -fshort-wchar -Wno-multichar)
    endif()

    target_link_libraries(cmlibhost PRIVATE host_includes)
endif()
