
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D_WINE)

add_definitions(-DHAVE_REGEX_H)
add_definitions(-D_IMAGEHLP_SOURCE_)

if(ARCH MATCHES amd64)
add_definitions(-DUNW_FLAG_NHANDLER=0 -DUNW_FLAG_EHANDLER=1 -DUNW_FLAG_UHANDLER=2 -DUNW_FLAG_CHAININFO=3)
endif(ARCH MATCHES amd64)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dbghelp ${CMAKE_CURRENT_SOURCE_DIR}/dbghelp.spec ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def)

add_library(dbghelp SHARED
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
    version.rc)

set_target_properties(dbghelp PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(dbghelp
    ${CMAKE_CURRENT_BINARY_DIR}/dbghelp.def
    wine
    mingw_common
    -lpsapi
    -lversion
    pseh
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a)

add_dependencies(dbghelp dbghelp_def psdk)
