
set_cpp()

include_directories(
    BEFORE ${CMAKE_CURRENT_SOURCE_DIR}
    inc)

#add_definitions(-DDEBUG)

list(APPEND SOURCE
    storahci.cpp
    ros_glue/ros_glue.cpp
    stdafx.h)

add_library(storahci SHARED ${SOURCE} storahci.rc)

if(NOT MSVC)
    add_target_compile_flags(storahci "-Wno-narrowing")
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(storahci "-Wno-unused-but-set-variable")
    endif()
endif()

add_pch(storahci stdafx.h SOURCE)
set_module_type(storahci kernelmodedriver)
add_importlibs(storahci storport ntoskrnl hal)
add_cd_file(TARGET storahci DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(storahci.inf)
