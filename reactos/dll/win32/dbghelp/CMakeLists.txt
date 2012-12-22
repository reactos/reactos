
add_definitions(
    -D__WINESRC__
    -D_WINE
    -DHAVE_REGEX_H
    -DHAVE_ALLOCA_H
    -D_IMAGEHLP_SOURCE_)

if(ARCH STREQUAL "amd64")
    add_definitions(-DUNW_FLAG_NHANDLER=0 -DUNW_FLAG_EHANDLER=1 -DUNW_FLAG_UHANDLER=2 -DUNW_FLAG_CHAININFO=3)
endif()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(dbghelp.dll dbghelp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    coff.c
    cpu_arm.c
    cpu_i386.c
    cpu_ppc.c
    cpu_sparc.c
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
    regex.c
    ${CMAKE_CURRENT_BINARY_DIR}/dbghelp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def)

add_library(dbghelp SHARED ${SOURCE} version.rc)

if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(dbghelp)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()

set_module_type(dbghelp win32dll)
target_link_libraries(dbghelp wine ${PSEH_LIB} oldnames)
add_delay_importlibs(dbghelp version)
add_importlibs(dbghelp psapi msvcrt kernel32 ntdll)
add_pch(dbghelp dbghelp_private.h)
add_cd_file(TARGET dbghelp DESTINATION reactos/system32 FOR all)
