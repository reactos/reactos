


add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCE *.c)

spec2def(acledit ${CMAKE_CURRENT_SOURCE_DIR}/acledit.spec ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

add_library(acledit SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/acledit.rc)

set_target_properties(acledit PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(acledit
    ${CMAKE_CURRENT_BINARY_DIR}/acledit.def
    -lntdll)

add_dependencies(acledit acledit_def psdk buildno_header)
