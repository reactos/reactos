
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
    if(ARCH STREQUAL "amd64")
        # Disable warning "conversion from 'size_t' to 'int', possible loss of data"
        target_compile_options(iconv-static PRIVATE "/wd4267")
    endif()
endif()

add_library(iconv-static win_iconv.c)
set_target_properties(iconv-static PROPERTIES OUTPUT_NAME "iconv")
add_dependencies(iconv-static psdk)
