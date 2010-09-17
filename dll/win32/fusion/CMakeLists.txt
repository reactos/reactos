
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(fusion ${CMAKE_CURRENT_SOURCE_DIR}/fusion.spec ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

add_library(fusion SHARED
    asmcache.c
    asmenum.c
    asmname.c
    assembly.c
    fusion.c
    fusion_main.c)

set_target_properties(fusion PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(fusion
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def
    wine
    mingw_common
    -lshlwapi
    -ladvapi32
    -ldbghelp
    -luser32)

add_dependencies(fusion fusion_def psdk)
