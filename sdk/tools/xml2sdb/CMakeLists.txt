
add_definitions(-DSDBWRITE_HOSTTOOL=1)

if(MSVC)
    set_property(SOURCE main.cpp tinyxml2.cpp xml2sdb.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " /EHsc")
else()
    add_compile_flags("-fshort-wchar -Wno-multichar")
endif()


list(APPEND SOURCE
    main.cpp
    tinyxml2.cpp
    xml2sdb.cpp
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbwrite.c
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbstringtable.c)

include_directories(${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp)
add_host_tool(xml2sdb ${SOURCE})
