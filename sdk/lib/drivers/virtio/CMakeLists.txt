
list(APPEND SOURCE
    VirtIOPCICommon.c
    VirtIOPCILegacy.c
    VirtIOPCIModern.c
    VirtIORing.c
    VirtIORing-Packed.c)

add_library(virtio ${SOURCE})
add_dependencies(virtio bugcodes xdk)

if(NOT MSVC)
    target_compile_options(virtio PRIVATE
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-attributes)
endif()
