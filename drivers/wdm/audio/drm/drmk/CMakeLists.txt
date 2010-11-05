
add_definitions(-DBUILDING_DRMK)

spec2def(drmk.sys drmk.spec)

add_library(drmk SHARED
    stubs.cpp
    drmk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

set_module_type(drmk kernelmodedriver)
add_importlibs(drmk ntoskrnl)

add_cab_target(drmk 2)