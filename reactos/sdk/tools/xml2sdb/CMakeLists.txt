
add_definitions(-DSDBWRITE_HOSTTOOL=1)

list(APPEND SOURCE
    main.cpp
    tinyxml2.cpp
    xml2sdb.cpp
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbwrite.c
    ${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp/sdbstringtable.c)

include_directories(${REACTOS_SOURCE_DIR}/dll/appcompat/apphelp)
add_host_tool(xml2sdb ${SOURCE})

if(MSVC)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS " /EHsc")
else()
    add_target_compile_flags(xml2sdb "-fshort-wchar")
endif()
