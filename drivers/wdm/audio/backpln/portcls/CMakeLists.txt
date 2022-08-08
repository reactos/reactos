

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(portcls.sys portcls.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    adapter.cpp
    api.cpp
    connection.cpp
    dispatcher.cpp
    dll.cpp
    dma_slave.cpp
    drm.cpp
    drm_port.cpp
    filter_topology.cpp
    filter_dmus.cpp
    filter_wavecyclic.cpp
    filter_wavepci.cpp
    filter_wavert.cpp
    interrupt.cpp
    irp.cpp
    irpstream.cpp
    miniport.cpp
    miniport_dmus.cpp
    miniport_fmsynth.cpp
    pin_dmus.cpp
    pin_wavecyclic.cpp
    pin_wavepci.cpp
    pin_wavert.cpp
    pool.cpp
    port.cpp
    port_dmus.cpp
    port_topology.cpp
    port_wavecyclic.cpp
    port_wavepci.cpp
    port_wavert.cpp
    port_wavertstream.cpp
    power.cpp
    propertyhandler.cpp
    purecall.cpp
    registry.cpp
    resource.cpp
    service_group.cpp
    undoc.cpp
    unregister.cpp
    version.cpp)

list(APPEND PCH_SKIP_SOURCE
    guid.cpp)

add_library(portcls MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    portcls.rc
    ${CMAKE_CURRENT_BINARY_DIR}/portcls.def)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(portcls PRIVATE "-Wno-missing-braces")
endif()

target_link_libraries(portcls libcntpr ${PSEH_LIB})
set_module_type(portcls kernelmodedriver ENTRYPOINT 0 )
add_pch(portcls private.hpp "${PCH_SKIP_SOURCE}")
add_importlibs(portcls ntoskrnl ks drmk hal)
add_cd_file(TARGET portcls DESTINATION reactos/system32/drivers FOR all)
