
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_library(isapnp SHARED
    isapnp.c
    pdo.c
    fdo.c
    hardware.c
    isapnp.rc)

set_target_properties(isapnp PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(isapnp
    ${REACTOS_SOURCE_DIR}/ntoskrnl/libntoskrnl.a
    ${REACTOS_SOURCE_DIR}/hal/halx86/libhal.a)

add_dependencies(isapnp psdk bugcodes)
