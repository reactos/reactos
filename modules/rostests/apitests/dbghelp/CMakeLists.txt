
add_definitions(-D__ROS_LONG64__ -DWINETEST_USE_DBGSTR_LONGLONG)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib
    ${REACTOS_SOURCE_DIR}/sdk/tools/rsym
)

list(APPEND SOURCE
    pdb.c
    rsym.c
    data.c
    testlist.c)

add_executable(dbghelp_apitest ${SOURCE} resource.rc)
set_module_type(dbghelp_apitest win32cui)
target_link_libraries(dbghelp_apitest zlib)
add_delay_importlibs(dbghelp_apitest dbghelp)
add_importlibs(dbghelp_apitest version msvcrt kernel32 ntdll)
add_rostests_file(TARGET dbghelp_apitest)
