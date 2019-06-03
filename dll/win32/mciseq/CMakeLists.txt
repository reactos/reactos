
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    mcimidi.c)

add_library(mciseq MODULE ${SOURCE})
spec2def(mciseq mciseq.dll mciseq.spec)

set_module_type(mciseq win32dll)
target_link_libraries(mciseq wine)
add_importlibs(mciseq winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mciseq DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_target_compile_flags(mciseq "-Wno-overflow")
endif()
