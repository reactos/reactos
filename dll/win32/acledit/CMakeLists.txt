
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCE *.c)

spec2def(acledit ${CMAKE_CURRENT_SOURCE_DIR}/acledit.spec ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

add_library(acledit SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_SOURCE_DIR}/acledit.rc)

set_target_properties(acledit PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(acledit
    ${CMAKE_CURRENT_BINARY_DIR}/acledit.def
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_dependencies(acledit acledit_def psdk buildno_header)
