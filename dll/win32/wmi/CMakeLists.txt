
spec2def(wmi.dll wmi.spec)

add_library(wmi SHARED wmi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmi.def)

set_entrypoint(wmi 0)

add_importlibs(wmi advapi32)
add_dependencies(wmi psdk buildno_header)
