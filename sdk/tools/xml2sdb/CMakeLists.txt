
add_definitions(-DSDBWRITE_HOSTTOOL=1)

list(APPEND SOURCE
    main.cpp
    tinyxml2.cpp
    xml2sdb.cpp
    xml2sdb.h
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbwrite.c
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbstringtable.c)

include_directories(
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/appcompat)
add_host_tool(xml2sdb ${SOURCE})

if(MSVC)
    # Disable warning "'=': conversion from 'a' to 'b', possible loss of data"
    add_target_compile_flags(xml2sdb "/wd4244")
else()
    add_target_compile_flags(xml2sdb "-fshort-wchar")
endif()
