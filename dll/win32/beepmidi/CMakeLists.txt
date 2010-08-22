
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_definitions(-DUNICODE -D_UNICODE)

spec2def(beepmidi ${CMAKE_CURRENT_SOURCE_DIR}/beepmidi.spec ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED beepmidi.c)

set_target_properties(beepmidi PROPERTIES LINK_FLAGS "-Wl,-entry,0")

target_link_libraries(beepmidi
    ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def
    -lwinmm
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_dependencies(beepmidi beepmidi_def psdk)
