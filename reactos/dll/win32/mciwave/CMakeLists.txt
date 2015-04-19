
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(mciwave.dll mciwave.spec)

list(APPEND SOURCE
    mciwave.c
    ${CMAKE_CURRENT_BINARY_DIR}/mciwave.def)

add_library(mciwave SHARED ${SOURCE})
set_module_type(mciwave win32dll)
target_link_libraries(mciwave wine)
add_importlibs(mciwave user32 winmm msvcrt kernel32 ntdll)
add_cd_file(TARGET mciwave DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_target_compile_flags(mciwave "-Wno-overflow")
endif()
