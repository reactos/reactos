
list(APPEND SOURCE
    VirtIOPCICommon.c
    VirtIOPCILegacy.c
    VirtIOPCIModern.c
    VirtIORing.c
    VirtIORing-Packed.c)

add_library(virtio STATIC ${SOURCE})
add_dependencies(virtio bugcodes xdk)
target_include_directories(virtio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT MSVC)
    target_compile_options(virtio PRIVATE
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-attributes)
endif()
