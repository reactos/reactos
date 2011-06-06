
set_unicode()

list(APPEND SOURCE
    epmp.c
    irotp.c
    rpcss_main.c
    service_main.c
    rpcss.rc)

include_directories(${REACTOS_BINARY_DIR}/include/reactos/wine)

add_executable(rpcss ${SOURCE})

target_link_libraries(rpcss epmrpc irotrpc ${PSEH_LIB} wine)

set_module_type(rpcss win32cui)

add_importlibs(rpcss advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET rpcss DESTINATION reactos/system32 FOR all)
add_dependencies(rpcss winesdk)
