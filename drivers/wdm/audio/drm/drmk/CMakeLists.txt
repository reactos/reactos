
add_definitions(-DBUILDING_DRMK)

spec2def(drmk.sys drmk.spec)

list(APPEND SOURCE
    stubs.cpp
    drmk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

add_library(drmk SHARED ${SOURCE})

set_module_type(drmk kernelmodedriver ENTRYPOINT 0)

add_importlibs(drmk ntoskrnl)
add_importlib_target(drmk.spec drmk.sys)
add_cd_file(TARGET drmk DESTINATION reactos/system32/drivers FOR all)
