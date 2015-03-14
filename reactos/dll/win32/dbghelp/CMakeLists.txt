
if(NOT CMAKE_CROSSCOMPILING)
    add_definitions(-DDBGHELP_STATIC_LIB)

    if(ARCH STREQUAL "i386")
        add_definitions(-D_X86_)
    endif()

    include_directories(
        ${REACTOS_SOURCE_DIR}/tools
        ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

    list(APPEND SOURCE
        compat.c
        dbghelp.c
        dwarf.c
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
        BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine
        ${REACTOS_SOURCE_DIR}/include/reactos/libs/zlib)

    spec2def(dbghelp.dll dbghelp.spec ADD_IMPORTLIB)

    list(APPEND SOURCE
        coff.c
        cpu_arm.c
        cpu_arm64.c
        cpu_i386.c
        cpu_ppc.c
        cpu_x86_64.c
        crc32.c
        dbghelp.c
        dwarf.c
        elf_module.c
        image.c
        macho_module.c
        minidump.c
        module.c
        msc.c
        path.c
        pe_module.c
        rosstubs.c
        source.c
        stabs.c
        stack.c
        storage.c
        symbol.c
        type.c
        dbghelp_private.h
        ${CMAKE_CURRENT_BINARY_DIR}/dbghelp_stubs.c)

    add_library(dbghelp SHARED
        ${SOURCE}
        version.rc
        ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def)

    set_module_type(dbghelp win32dll)
    target_link_libraries(dbghelp wine ${PSEH_LIB} oldnames zlib)
    add_delay_importlibs(dbghelp version)
    add_importlibs(dbghelp psapi msvcrt kernel32 ntdll)
    add_pch(dbghelp dbghelp_private.h SOURCE)
    add_cd_file(TARGET dbghelp DESTINATION reactos/system32 FOR all)

endif()
