
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(getuname ${CMAKE_CURRENT_SOURCE_DIR}/getuname.spec ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

add_library(getuname SHARED getuname.c getuname.rc)

set_target_properties(getuname PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(getuname
    ${CMAKE_CURRENT_BINARY_DIR}/getuname.def
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_dependencies(getuname getuname_def psdk)
