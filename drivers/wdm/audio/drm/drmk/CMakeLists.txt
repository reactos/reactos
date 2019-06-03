
add_definitions(-DBUILDING_DRMK)

list(APPEND SOURCE
    stubs.cpp
    drmk.rc)

add_library(drmk MODULE ${SOURCE})
spec2def(drmk drmk.sys drmk.spec ADD_IMPORTLIB)

set_module_type(drmk kernelmodedriver ENTRYPOINT 0)
add_importlibs(drmk ntoskrnl)
add_cd_file(TARGET drmk DESTINATION reactos/system32/drivers FOR all)
