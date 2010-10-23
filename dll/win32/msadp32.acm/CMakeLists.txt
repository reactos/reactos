
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/msadp32.acm.spec --filename msadp32.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(msadp32.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def)

add_library(msadp32.acm SHARED msadp32.c)

set_entrypoint(msadp32.acm 0)
set_target_properties(msadp32.acm PROPERTIES SUFFIX "")

target_link_libraries(msadp32.acm
    ${CMAKE_CURRENT_BINARY_DIR}/msadp32.acm.def
    wine)

add_importlibs(msadp32.acm winmm user32 msvcrt kernel32 ntdll)
add_dependencies(msadp32.acm msadp32.acm_def psdk buildno_header)
add_livecd_target(msadp32.acm reactos/system32)
