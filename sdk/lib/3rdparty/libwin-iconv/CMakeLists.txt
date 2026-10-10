
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_library(iconv-static STATIC win_iconv.c)
set_target_properties(iconv-static PROPERTIES OUTPUT_NAME "iconv")
add_dependencies(iconv-static psdk)
target_include_directories(iconv-static INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
