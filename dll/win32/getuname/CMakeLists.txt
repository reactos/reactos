


add_definitions(-DUNICODE -D_UNICODE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(getuname ${CMAKE_CURRENT_SOURCE_DIR}/getuname.spec ${CMAKE_CURRENT_BINARY_DIR}/getuname.def)

add_library(getuname SHARED getuname.c getuname.rc)

set_target_properties(getuname PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(getuname
    ${CMAKE_CURRENT_BINARY_DIR}/getuname.def
    -lkernel32
    -lntdll)

add_dependencies(getuname getuname_def psdk)
