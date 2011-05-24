
add_definitions(-DBUILDING_DRMK)

spec2def(drmk.sys drmk.spec)

list(APPEND SOURCE
    stubs.cpp
    drmk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/drmk.def)

add_library(drmk SHARED ${SOURCE})

if(NOT MSVC)
	set_module_type(drmk kernelmodedriver)
else()
	set_target_properties(drmk PROPERTIES SUFFIX ".sys")
	set_subsystem(drmk native)
    set_image_base(drmk 0x00010000)
    add_linkerflag(drmk "/DRIVER")
    add_dependencies(drmk bugcodes)
    target_link_libraries(drmk msvcsup)
	add_dependencies(drmk psdk)
endif()

set_entrypoint(drmk 0)
add_importlibs(drmk ntoskrnl)
add_importlib_target(drmk.spec)

add_cd_file(TARGET drmk DESTINATION reactos/system32/drivers FOR all)
