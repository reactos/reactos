
spec2def(beepmidi.dll beepmidi.spec)

list(APPEND SOURCE
    beepmidi.c
    ${CMAKE_CURRENT_BINARY_DIR}/beepmidi.def)

add_library(beepmidi SHARED ${SOURCE})

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_link_libraries(beepmidi mingwex)
endif()

set_module_type(beepmidi win32dll UNICODE)

add_importlibs(beepmidi winmm msvcrt kernel32 ntdll)


add_dependencies(beepmidi psdk)
add_cd_file(TARGET beepmidi DESTINATION reactos/system32 FOR all)
