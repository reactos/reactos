
add_library(bdasup MODULE
            bdasup.c)
spec2def(bdasup bdasup.sys bdasup.spec ADD_IMPORTLIB)

set_module_type(bdasup kernelmodedriver ENTRYPOINT 0)
add_importlibs(bdasup ks ntoskrnl hal)
add_cd_file(TARGET bdasup DESTINATION reactos/system32/drivers FOR all)
