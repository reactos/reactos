
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(-DUSE_WINE_TODOS)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(IDL_FLAGS ${IDL_FLAGS} --prefix-server=s_)
add_rpc_files(client server.idl)
add_rpc_files(server server.idl)
unset(IDL_FLAGS)

list(APPEND SOURCE
    cstub.c
    generated_${ARCH}.c
    ndr_marshall.c
    rpc.c
    rpc_async.c
    rpc_protseq.c
    server.c
    testlist.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_s.c)

if(MSVC)
    # The nmake generator misses this dependency, so set it explicitly until this bug is fixed.
    set_source_files_properties(server.c PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/server_s.h)
endif()

add_executable(rpcrt4_winetest ${SOURCE})
target_link_libraries(rpcrt4_winetest uuid wine ${PSEH_LIB})
set_module_type(rpcrt4_winetest win32cui)
add_importlibs(rpcrt4_winetest ole32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET rpcrt4_winetest DESTINATION reactos/bin FOR all)

if(NOT MSVC)
    add_target_compile_flags(rpcrt4_winetest "-Wno-format")
endif()
