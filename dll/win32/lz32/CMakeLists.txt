
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(lz32.dll lz32.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lz32.def)

add_library(lz32 SHARED ${SOURCE})

set_entrypoint(lz32 0)
target_link_libraries(lz32 wine)

add_importlibs(lz32 kernel32 ntdll)
add_dependencies(lz32 psdk)
add_cd_file(TARGET lz32 DESTINATION reactos/system32 FOR all)
add_importlib_target(lz32.spec)
