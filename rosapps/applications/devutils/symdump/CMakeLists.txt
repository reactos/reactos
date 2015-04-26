
include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_executable(symdump symdump.c)
set_module_type(symdump win32cui)
target_link_libraries(symdump wine)
add_importlibs(symdump dbghelp shlwapi msvcrt kernel32)
add_compile_flags("-Wno-unused-but-set-variable")
add_cd_file(TARGET symdump DESTINATION reactos/system32 FOR all)
