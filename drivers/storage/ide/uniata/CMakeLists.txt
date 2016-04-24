
set_cpp()

include_directories(
    BEFORE ${CMAKE_CURRENT_SOURCE_DIR}
    inc)

#add_definitions(-D_DEBUG)

list(APPEND SOURCE
    id_ata.cpp
    id_badblock.cpp
    id_dma.cpp
    id_init.cpp
    id_probe.cpp
    id_queue.cpp
    id_sata.cpp
    ros_glue/ros_glue.cpp
    stdafx.h)

add_library(uniata SHARED ${SOURCE} idedma.rc)

if(NOT MSVC)
    add_target_compile_flags(uniata "-Wno-narrowing")
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(uniata "-Wno-unused-but-set-variable")
    endif()
endif()

add_pch(uniata stdafx.h SOURCE)
set_module_type(uniata kernelmodedriver)
add_importlibs(uniata scsiport ntoskrnl hal)
add_cd_file(TARGET uniata DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(uniata_reg.inf)
