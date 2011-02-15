
add_definitions(-DBUILDING_DRMK)

spec2def(drmk.sys drmk.spec)

list(APPEND SOURCE
    stubs.cpp
    drmk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

add_library(drmk SHARED ${SOURCE})

set_entrypoint(drmk 0)
set_target_properties(drmk PROPERTIES SUFFIX ".sys")
add_importlibs(drmk ntoskrnl)
add_importlib_target(drmk.spec)

add_cab_target(drmk 2)
