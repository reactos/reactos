
spec2def(wmilib.sys wmilib.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    wmilib.c
    wmilib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmilib.def)

add_library(wmilib SHARED ${SOURCE})

if(NOT MSVC)
    set_module_type(wmilib kernelmodedriver)
else()
    set_target_properties(wmilib PROPERTIES SUFFIX ".sys")
    set_subsystem(wmilib native)
    set_image_base(wmilib 0x00010000)
    add_linkerflag(wmilib "/DRIVER")
    add_dependencies(wmilib bugcodes)
    add_dependencies(wmilib psdk)
endif()

set_entrypoint(wmilib 0)
set_target_properties(wmilib PROPERTIES SUFFIX ".sys")
add_importlibs(wmilib ntoskrnl)
add_importlib_target(wmilib.spec)
add_cd_file(TARGET wmilib DESTINATION reactos/system32/drivers FOR all)
