


add_definitions(-DUNICODE -D_UNICODE)

spec2def(batt ${CMAKE_CURRENT_SOURCE_DIR}/batt.spec ${CMAKE_CURRENT_BINARY_DIR}/batt.def)

add_library(batt SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/batt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/batt.rc)

set_target_properties(batt PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(batt
    ${CMAKE_CURRENT_BINARY_DIR}/batt.def
    -lkernel32)

add_dependencies(batt batt_def psdk)
