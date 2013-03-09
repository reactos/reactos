
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys)

spec2def(basesrv.dll basesrv.spec)

list(APPEND SOURCE
    dosdev.c
    init.c
    proc.c
    sndsntry.c
    basesrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/basesrv.def)

add_library(basesrv SHARED ${SOURCE})

target_link_libraries(basesrv ${PSEH_LIB})

set_module_type(basesrv nativedll) # win32dll

add_importlibs(basesrv ntdll csrsrv)

add_dependencies(basesrv bugcodes)
add_cd_file(TARGET basesrv DESTINATION reactos/system32 FOR all)
