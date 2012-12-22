
include_directories(
    BEFORE ${CMAKE_CURRENT_SOURCE_DIR}
    inc)

#add_definitions(-D_DEBUG)

set_cpp()

list(APPEND SOURCE
    id_ata.cpp
    id_badblock.cpp
    id_dma.cpp
    id_init.cpp
    id_probe.cpp
    id_queue.cpp
    id_sata.cpp
    ros_glue/ros_glue.cpp)

add_library(uniata SHARED ${SOURCE} idedma.rc)

# FIXME: http://www.cmake.org/Bug/view.php?id=12998
if(MSVC)
    #add_target_compile_flags(uniata "/GR-")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "/GR-")
else()
    #allow_warnings(uniata)
    #add_target_compile_flags(uniata "-fno-exceptions -fno-rtti")
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-fno-exceptions -fno-rtti -Wno-error")
endif()

add_pch(uniata stdafx.h)
set_module_type(uniata kernelmodedriver)
add_importlibs(uniata scsiport ntoskrnl hal)
add_cd_file(TARGET uniata DESTINATION reactos/system32/drivers NO_CAB FOR all)
