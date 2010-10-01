
add_definitions(-DBUILDING_KS)
add_definitions(-D_COMDDK_)

add_library(ks SHARED
    api.c
    allocators.c
    bag.c
    device.c
    deviceinterface.c
    driver.c
    clocks.c
    connectivity.c
    event.c
    filter.c
    filterfactory.c
    image.c
    irp.c
    methods.c
    misc.c
    pin.c
    property.c
    topology.c
    unknown.c
    worker.c
    kcom.c
    ks.rc)

set_target_properties(ks PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

set_module_type(ks kernelmodedriver)

target_link_libraries(ks
    ${CMAKE_CURRENT_SOURCE_DIR}/ks.def
    pseh)

add_importlibs(ks ntoskrnl hal msvcrt)
add_dependencies(ks ks_def psdk bugcodes)
