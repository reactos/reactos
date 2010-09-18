


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
