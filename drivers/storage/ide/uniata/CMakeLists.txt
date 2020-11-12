

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
    stdafx.h) # Not stdafx.cpp (which only includes the .h), to fix PCH.

add_library(uniata MODULE ${SOURCE} idedma.rc)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(uniata PRIVATE "-Wno-narrowing")
    if(USE_CLANG_CL)
        target_compile_options(uniata PRIVATE "-Wno-unused-const-variable")
    else()
        target_compile_options(uniata PRIVATE "-Wno-unused-but-set-variable")
    endif()
endif()

add_pch(uniata stdafx.h SOURCE)
set_module_type(uniata kernelmodedriver)
add_importlibs(uniata scsiport ntoskrnl hal)
add_cd_file(TARGET uniata DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(uniata_reg.inf)
add_driver_inf(uniata uniata_comm.inf)
