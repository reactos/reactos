
spec2def(hidparse.sys hidparse.spec)
add_definitions(-DDEBUG_MODE)

include_directories(
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/lib/drivers/hidparser)


add_library(hidparse SHARED hidparse.c hidparse.rc ${CMAKE_CURRENT_BINARY_DIR}/hidparse.def)

set_module_type(hidparse kernelmodedriver)
add_importlibs(hidparse ntoskrnl)
add_importlib_target(hidparse.spec)

target_link_libraries(hidparse hidparser)
