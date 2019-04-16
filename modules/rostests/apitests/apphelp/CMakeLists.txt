project(appcompat)

add_definitions(
    -D__ROS_LONG64__
    -DWINETEST_USE_DBGSTR_LONGLONG
    -DUNICODE
    -D_UNICODE
    -D_ATL_NO_EXCEPTIONS)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl
                    ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND SOURCE
    apphelp.c
    data.c
    db.cpp
    env.c
    layerapi.c
    register.cpp
    testlist.c
    testdata.rc
    testdb.xml)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb
                   COMMAND native-xml2sdb -i ${CMAKE_CURRENT_SOURCE_DIR}/testdb.xml -o ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/testdb.xml native-xml2sdb)
                   
add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb)
add_executable(apphelp_apitest ${SOURCE})
set_module_type(apphelp_apitest win32cui)
target_link_libraries(apphelp_apitest ${PSEH_LIB})
add_importlibs(apphelp_apitest advapi32 userenv version shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET apphelp_apitest)
