
spec2def(ks.sys ks.spec)

add_definitions(
    -DBUILDING_KS
    -D_COMDDK_)

list(APPEND SOURCE
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
    ks.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ks.def)

add_library(ks SHARED ${SOURCE})

set_target_properties(ks PROPERTIES SUFFIX ".sys")

set_entrypoint(ks 0)
set_subsystem(ks native)
set_image_base(ks 0x00010000)

target_link_libraries(ks ${PSEH_LIB})

add_importlibs(ks ntoskrnl hal msvcrt)
add_dependencies(ks psdk bugcodes)

add_cab_target(ks 2)
add_importlib_target(ks.spec ks.sys)