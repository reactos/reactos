
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(usbstor SHARED usbstor.c usbstor.rc)

set_module_type(usbstor kernelmodedriver)
add_importlibs(usbstor ntoskrnl hal)
add_cab_target(usbstor 2)
