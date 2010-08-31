
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_library(i8042prt SHARED
    createclose.c
    i8042prt.c
    keyboard.c
    misc.c
    mouse.c
    pnp.c
    ps2pp.c
    readwrite.c
    registry.c
    setup.c
    i8042prt.rc)

set_target_properties(i8042prt PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--exclude-all-symbols" SUFFIX ".sys")

target_link_libraries(i8042prt
    ${REACTOS_SOURCE_DIR}/ntoskrnl/libntoskrnl.a
    ${REACTOS_SOURCE_DIR}/hal/halx86/libhal.a)

add_dependencies(i8042prt psdk bugcodes)
