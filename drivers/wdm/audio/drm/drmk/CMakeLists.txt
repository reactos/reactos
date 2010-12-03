
add_definitions(-DBUILDING_DRMK)

spec2def(drmk.sys drmk.spec)

list(APPEND SOURCE
    stubs.cpp
    drmk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

add_library(drmk SHARED ${SOURCE})

set_module_type(drmk kernelmodedriver)
add_importlibs(drmk ntoskrnl)
add_importlib_target(drmk.spec drmk.sys)

add_cab_target(drmk 2)
