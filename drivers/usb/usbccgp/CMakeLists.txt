
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(usbccgp SHARED descriptor.c fdo.c misc.c pdo.c usbccgp.c usbccgp.rc)

target_link_libraries(usbccgp ${PSEH_LIB})

set_module_type(usbccgp kernelmodedriver)
add_importlibs(usbccgp ntoskrnl hal usbd)

add_cd_file(TARGET usbccgp DESTINATION reactos/system32/drivers NO_CAB FOR all)
