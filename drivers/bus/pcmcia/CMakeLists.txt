
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_library(pcmcia SHARED
    fdo.c
    pcmcia.c
    pdo.c
    pcmcia.rc)

set_target_properties(pcmcia PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(pcmcia
    -lntoskrnl
    -lhal)

add_dependencies(pcmcia psdk bugcodes buildno_header)
