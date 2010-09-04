
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_library(buslogic SHARED BusLogic958.c BusLogic958.rc)

set_target_properties(buslogic PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(buslogic
    ${REACTOS_SOURCE_DIR}/drivers/storage/scsiport/libscsiport.a 
    libcntpr)

add_dependencies(buslogic psdk bugcodes)
