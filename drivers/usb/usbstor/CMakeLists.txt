
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(usbstor SHARED descriptor.c fdo.c misc.c pdo.c scsi.c usbstor.c usbstor.rc)

set_module_type(usbstor kernelmodedriver)
add_importlibs(usbstor ntoskrnl hal usbd)
add_cab_target(usbstor 2)
