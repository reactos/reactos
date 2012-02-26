
add_definitions(
    -D__WINESRC__
    -D_WINE
    -DHAVE_REGEX_H
    -DHAVE_ALLOCA_H
    -D_IMAGEHLP_SOURCE_)

if(ARCH MATCHES amd64)
    add_definitions(-DUNW_FLAG_NHANDLER=0 -DUNW_FLAG_EHANDLER=1 -DUNW_FLAG_UHANDLER=2 -DUNW_FLAG_CHAININFO=3)
endif()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()

spec2def(dbghelp.dll dbghelp.spec)

list(APPEND SOURCE
    coff.c
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
    regex.c
    rosstubs.c
    source.c
    stabs.c
    stack.c
    storage.c
    symbol.c
    type.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dbghelp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def)

add_library(dbghelp SHARED ${SOURCE})
allow_warnings(dbghelp)
set_module_type(dbghelp win32dll)
target_link_libraries(dbghelp wine ${PSEH_LIB} oldnames)
add_delay_importlibs(dbghelp version)
add_importlibs(dbghelp psapi msvcrt kernel32 ntdll)
add_importlib_target(dbghelp.spec dbghelp.dll)
add_cd_file(TARGET dbghelp DESTINATION reactos/system32 FOR all)
