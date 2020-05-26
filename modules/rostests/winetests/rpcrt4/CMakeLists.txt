
remove_definitions(-DWINVER=0x502 -D_WIN32_IE=0x600 -D_WIN32_WINNT=0x502)

add_definitions(
    -DUSE_WINE_TODOS
    -DWINETEST_USE_DBGSTR_LONGLONG
    -DPROXY_DELEGATION)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(OLD_IDL_FLAGS ${IDL_FLAGS})
set(IDL_FLAGS ${IDL_FLAGS} --prefix-server=s_ -Os --prefix-client=mixed_)
add_rpc_files(client server.idl)
add_rpc_files(server server.idl)
set(IDL_FLAGS ${OLD_IDL_FLAGS})
set(IDL_FLAGS ${IDL_FLAGS} --prefix-server=s_ -Oicf --prefix-client=interp_)
add_rpc_files(client server_interp.idl)
add_rpc_files(server server_interp.idl)
set(IDL_FLAGS ${OLD_IDL_FLAGS})
add_rpcproxy_files(cstub.idl)

list(APPEND SOURCE
    cstub.c
    generated.c
    ndr_marshall.c
    rpc.c
    rpc_async.c
    server.c
    testlist.c
    ${CMAKE_CURRENT_BINARY_DIR}/cstub_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_interp_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_interp_s.c)

if(MSVC)
    # The nmake generator misses this dependency, so set it explicitly until this bug is fixed.
    set_source_files_properties(server.c PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/server_s.h)
endif()

add_executable(rpcrt4_winetest ${SOURCE})

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(rpcrt4_winetest PRIVATE "-Wno-format")
    if(USE_CLANG_CL)
        target_compile_options(rpcrt4_winetest PRIVATE "-Wno-cast-calling-convention")
    endif()
endif()

target_link_libraries(rpcrt4_winetest uuid wine ${PSEH_LIB})
set_module_type(rpcrt4_winetest win32cui)
add_importlibs(rpcrt4_winetest oleaut32 ole32 rpcrt4 secur32 advapi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET rpcrt4_winetest)
