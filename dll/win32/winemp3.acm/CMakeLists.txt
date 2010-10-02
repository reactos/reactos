
add_definitions(-D_DLL -D__USE_CRTIMP)
add_definitions(-D__WINESRC__ -DWIN32)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/libmpg123)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/winemp3.acm.def
    COMMAND native-winebuild -o ${CMAKE_CURRENT_BINARY_DIR}/winemp3.acm.def --def -E ${CMAKE_CURRENT_SOURCE_DIR}/winemp3.acm.spec --filename winemp3.acm
    DEPENDS native-winebuild)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/winemp3.acm.def PROPERTIES GENERATED TRUE)
add_custom_target(winemp3.acm_def ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/winemp3.acm.def)

add_library(winemp3.acm SHARED mpegl3.c)

set_target_properties(winemp3.acm PROPERTIES LINK_FLAGS "-Wl,-entry,0" SUFFIX "")

target_link_libraries(winemp3.acm
    ${CMAKE_CURRENT_BINARY_DIR}/winemp3.acm.def
    mingw_common
    wine
    mpg123)

add_importlibs(winemp3.acm msvcrt winmm user32)
add_dependencies(winemp3.acm winemp3.acm_def psdk)
