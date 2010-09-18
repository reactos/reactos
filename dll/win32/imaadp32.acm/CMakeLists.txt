


add_definitions(-D_DLL -D__USE_CRTIMP)

add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/imaadp32.acm.spec --filename imaadp32.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/imaadp32.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(imaadp32.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.acm.def)

add_library(imaadp32.acm SHARED imaadp32.c)

set_target_properties(imaadp32.acm PROPERTIES LINK_FLAGS "-Wl,-entry,0" SUFFIX "")

target_link_libraries(imaadp32.acm
    ${CMAKE_CURRENT_BINARY_DIR}/imaadp32.acm.def
    wine
    -lwinmm
    -luser32
    mingw_common
    -lmsvcrt)

add_dependencies(imaadp32.acm imaadp32.acm_def psdk)
