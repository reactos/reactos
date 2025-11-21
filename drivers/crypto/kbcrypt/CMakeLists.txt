# ReactOS Drivers - kbcrypt (kernel bcrypt driver)
spec2def(bcryptdd.sys bcryptdd.spec)

include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/drivers/kbcrypt
    ${REACTOS_SOURCE_DIR}/sdk/lib/cryptlib
)

# List the bcrypt source file
list(APPEND SOURCE
    bcrypt.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptdd.def)

add_library(kbcrypt MODULE ${SOURCE})
target_link_libraries(kbcrypt cryptlib pseh)
set_module_type(kbcrypt kernelmodedriver)
add_importlibs(kbcrypt ntoskrnl hal)
add_cd_file(TARGET kbcrypt DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(bcryptdd_reg.inf)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-error")