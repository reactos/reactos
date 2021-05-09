
if(NOT CMAKE_CROSSCOMPILING)
    add_definitions(-DDBGHELP_STATIC_LIB)

    include_directories(
        ${REACTOS_SOURCE_DIR}/tools)

    list(APPEND SOURCE
        compat.c
        dbghelp.c
        dwarf.c
        inflate.c
        module.c
        pe_module.c
        source.c
        stabs.c
        storage.c
        symbol.c
        type.c)

    if(ARCH STREQUAL "i386")
        list(APPEND SOURCE cpu_i386.c)
    endif()

    add_library(dbghelphost ${SOURCE})
    target_link_libraries(dbghelphost PRIVATE host_includes)
else()
    add_definitions(
        -D__WINESRC__
        -D_WINE
        -DHAVE_ALLOCA_H
        -D_IMAGEHLP_SOURCE_)

    if(ARCH STREQUAL "amd64")
        add_definitions(-DUNW_FLAG_NHANDLER=0 -DUNW_FLAG_EHANDLER=1 -DUNW_FLAG_UHANDLER=2 -DUNW_FLAG_CHAININFO=3)
    endif()

    include_directories(
        BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

    spec2def(dbghelp.dll dbghelp.spec ADD_IMPORTLIB)

    list(APPEND SOURCE
        coff.c
        cpu_arm.c
        cpu_arm64.c
        cpu_i386.c
        cpu_x86_64.c
        dbghelp.c
        dwarf.c
#       elf_module.c
        image.c
#       macho_module.c
        inflate.c
        minidump.c
        module.c
        msc.c
        path.c
        pe_module.c
        rosstubs.c
        rsym.c
        source.c
        stabs.c
        stack.c
        storage.c
        symbol.c
        type.c
        precomp.h)

    add_library(dbghelp MODULE
        ${SOURCE}
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/dbghelp_stubs.c
        ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def)

    set_module_type(dbghelp win32dll)
    target_link_libraries(dbghelp wine ${PSEH_LIB} oldnames)
    add_delay_importlibs(dbghelp version)
    add_importlibs(dbghelp psapi msvcrt kernel32 ntdll)
    add_pch(dbghelp precomp.h SOURCE)
    add_cd_file(TARGET dbghelp DESTINATION reactos/system32 FOR all)

    if(MSVC)
        # Disable warning C4477 (printf format warnings)
        remove_target_compile_option(dbghelp "/we4477")
    endif()
endif()
