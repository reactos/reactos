spec2def(bdasup.sys bdasup.spec ADD_IMPORTLIB)

add_library(bdasup MODULE
            bdasup.c
            ${CMAKE_CURRENT_BINARY_DIR}/bdasup.def)

set_module_type(bdasup kernelmodedriver ENTRYPOINT 0)
add_importlibs(bdasup ks ntoskrnl hal)
add_cd_file(TARGET bdasup DESTINATION reactos/system32/drivers FOR all)

