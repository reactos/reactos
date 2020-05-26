
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mciseq.dll mciseq.spec)

list(APPEND SOURCE
    mcimidi.c
    ${CMAKE_CURRENT_BINARY_DIR}/mciseq.def)

add_library(mciseq MODULE ${SOURCE})
set_module_type(mciseq win32dll)
target_link_libraries(mciseq wine)
add_importlibs(mciseq winmm user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mciseq DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    target_compile_options(mciseq PRIVATE "-Wno-overflow")
endif()
