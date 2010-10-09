
spec2def(wmi ${CMAKE_CURRENT_SOURCE_DIR}/wmi.spec ${CMAKE_CURRENT_BINARY_DIR}/wmi.def)

add_library(wmi SHARED wmi.rc)

set_entrypoint(wmi 0)

target_link_libraries(wmi ${CMAKE_CURRENT_BINARY_DIR}/wmi.def)

add_importlibs(wmi advapi32)
add_dependencies(wmi wmi_def psdk buildno_header)
