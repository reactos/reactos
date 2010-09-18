


include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(snmpapi ${CMAKE_CURRENT_SOURCE_DIR}/snmpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/snmpapi.def)

add_library(snmpapi SHARED main.c snmpapi.rc)

set_target_properties(snmpapi PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(snmpapi
    ${CMAKE_CURRENT_BINARY_DIR}/snmpapi.def
    wine
    mingw_common)

add_dependencies(snmpapi snmpapi_def psdk)
