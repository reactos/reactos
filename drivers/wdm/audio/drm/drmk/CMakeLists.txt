
add_definitions(-DBUILDING_DRMK)

spec2def(drmk ${CMAKE_CURRENT_SOURCE_DIR}/drmk.spec ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

add_library(drmk SHARED stubs.cpp drmk.rc)

set_target_properties(drmk PROPERTIES LINK_FLAGS "-Wl,-entry,0 -Wl,--image-base,0x00010000 -Wl,--subsystem,native" SUFFIX ".sys")

target_link_libraries(drmk ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)
add_importlibs(drmk ntoskrnl)
add_dependencies(drmk drmk_def psdk bugcodes)
