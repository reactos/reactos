
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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
