
list(APPEND SOURCE
    cabinet.cxx
    dfp.cxx
    main.cxx
    mszip.cxx
    raw.cxx)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/zlib)
add_host_tool(cabman ${SOURCE})
target_link_libraries(cabman zlibhost)
