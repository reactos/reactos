

include_directories(
    BEFORE ${CMAKE_CURRENT_SOURCE_DIR}
    inc)

#add_definitions(-D_DEBUG)

list(APPEND SOURCE
    atacmd_map.cpp
    bm_devs.cpp
    id_ata.cpp
    id_badblock.cpp
    id_dma.cpp
    id_init.cpp
    id_probe.cpp
    id_queue.cpp
    id_sata.cpp
    ros_glue/ros_glue.cpp
    stdafx.h)

add_library(uniata MODULE ${SOURCE} idedma.rc)

if(MSVC)
    # Disable warning C4267: '=': conversion from 'size_t' to 'USHORT', possible loss of data
    # Disable warning C4838: conversion from 'int' to 'ULONG' requires a narrowing conversion
    target_compile_options(uniata PRIVATE /wd4267 /wd4838)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(uniata PRIVATE -Wno-narrowing -Wno-unused-but-set-variable)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(uniata PRIVATE -Wno-unused-const-variable)
endif()

add_pch(uniata stdafx.h SOURCE)
set_module_type(uniata kernelmodedriver)
add_importlibs(uniata scsiport ntoskrnl hal)
add_cd_file(TARGET uniata DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(uniata_reg.inf)
add_driver_inf(uniata uniata_comm.inf)
