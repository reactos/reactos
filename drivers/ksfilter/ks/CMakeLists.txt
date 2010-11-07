
add_definitions(
    -DBUILDING_KS
    -D_COMDDK_)

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

set_target_properties(ks PROPERTIES SUFFIX ".sys")
set_entrypoint(ks 0)
set_subsystem(ks native)
set_image_base(ks 0x00010000)

target_link_libraries(ks
    ${CMAKE_CURRENT_SOURCE_DIR}/ks.def
    ${PSEH_LIB})

add_importlibs(ks ntoskrnl hal msvcrt)
add_dependencies(ks psdk bugcodes)

add_cab_target(ks 2)