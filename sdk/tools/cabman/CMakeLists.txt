
list(APPEND SOURCE
    cabinet.cxx
    dfp.cxx
    main.cxx
    mszip.cxx
    raw.cxx
    CCFDATAStorage.cxx)

add_host_tool(cabman ${SOURCE})
target_link_libraries(cabman PRIVATE host_includes zlibhost)
