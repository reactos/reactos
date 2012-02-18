remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)


add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(usbccgp SHARED descriptor.c fdo.c function.c misc.c pdo.c usbccgp.c usbccgp.rc)

target_link_libraries(usbccgp ${PSEH_LIB})

set_module_type(usbccgp kernelmodedriver)
add_importlibs(usbccgp ntoskrnl hal usbd)

add_cd_file(TARGET usbccgp DESTINATION reactos/system32/drivers NO_CAB FOR all)
