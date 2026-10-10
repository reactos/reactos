include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/lib/drivers/virtio)

spec2def(viostor.sys viostor.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    viostor.c
    viostor-debug.c)

add_library(viostor MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/viostor.def)
set_module_type(viostor kernelmodedriver)
target_link_libraries(viostor virtio)
add_importlibs(viostor ntoskrnl hal storport)
add_cd_file(TARGET viostor DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(viostor_reg.inf)
add_driver_inf(viostor viostor.inf)
