project(appcompat)
add_definitions(-D__ROS_LONG64__ -DWINETEST_USE_DBGSTR_LONGLONG)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

list(APPEND SOURCE
    apphelp.c
    data.c
    db.cpp
    env.c
    layerapi.c
    testlist.c
    testdata.rc
    testdb.xml)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb
                   COMMAND native-xml2sdb -i ${CMAKE_CURRENT_SOURCE_DIR}/testdb.xml -o ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/testdb.xml native-xml2sdb)
                   
add_custom_target(testdb DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/testdb.sdb)
add_rc_deps(testdata.rc testdb)
add_executable(apphelp_apitest ${SOURCE})
set_module_type(apphelp_apitest win32cui)
target_link_libraries(apphelp_apitest ${PSEH_LIB})
add_importlibs(apphelp_apitest advapi32 userenv version shlwapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET apphelp_apitest)
