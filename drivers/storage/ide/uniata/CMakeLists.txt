
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

include_directories(inc)

add_library(uniata SHARED
    id_ata.cpp
    id_badblock.cpp
    id_dma.cpp
    id_init.cpp
    id_probe.cpp
    id_queue.cpp
    id_sata.cpp
    idedma.rc
    ros_glue/ros_glue.cpp)

set_target_properties(uniata PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(uniata
    -lscsiport
    -lntoskrnl
    -lhal
    -lgcc)

add_dependencies(uniata psdk bugcodes buildno_header)
