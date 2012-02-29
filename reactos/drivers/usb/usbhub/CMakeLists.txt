
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(usbhub SHARED fdo.c misc.c pdo.c hub_fdo.c usbhub.c usbhub.rc)

target_link_libraries(usbhub ${PSEH_LIB})

set_module_type(usbhub kernelmodedriver)
add_importlibs(usbhub ntoskrnl hal usbd)

add_cd_file(TARGET usbhub DESTINATION reactos/system32/drivers NO_CAB FOR all)
