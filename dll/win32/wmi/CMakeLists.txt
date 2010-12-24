
spec2def(wmi.dll wmi.spec)

list(APPEND SOURCE
    wmi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmi.def)

add_library(wmi SHARED ${SOURCE})

set_entrypoint(wmi 0)

add_importlibs(wmi advapi32)
add_dependencies(wmi psdk buildno_header)
add_cab_target(wmi 1)
