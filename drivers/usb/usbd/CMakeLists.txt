
spec2def(usbd ${CMAKE_CURRENT_SOURCE_DIR}/usbd.spec ${CMAKE_CURRENT_BINARY_DIR}/usbd.def)

add_library(usbd SHARED usbd.c usbd.rc)

set_target_properties(usbd PROPERTIES LINK_FLAGS "-Wl,-entry,_DriverEntry@8 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(usbd
    ${CMAKE_CURRENT_BINARY_DIR}/usbd.def)

add_importlibs(usbd ntoskrnl hal)
add_dependencies(usbd usbd_def psdk bugcodes)
