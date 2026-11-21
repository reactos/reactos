
option(VIRTIO_SND_TEST_XRUN_INJECTION
    "Inject one synthetic XRUN per playback stream for recovery testing"
    OFF)

list(APPEND SOURCE
    adapter.c
    audio_controls.c
    driver.c
    topology.cpp
    transport.c
    virtio_debug.c
    wavecyclic.cpp)

add_library(virtiosnd MODULE ${SOURCE})

target_link_libraries(virtiosnd virtio stdunk libcntpr uuid)

if(VIRTIO_SND_TEST_XRUN_INJECTION)
    target_compile_definitions(virtiosnd PRIVATE VIRTIO_SND_TEST_XRUN_INJECTION=1)
    message(STATUS "Building virtiosnd with synthetic XRUN injection enabled")
endif()

set_module_type(virtiosnd wdmdriver)
add_importlibs(virtiosnd portcls hal ntoskrnl)
add_cd_file(TARGET virtiosnd DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(virtiosnd virtiosnd.inf)
