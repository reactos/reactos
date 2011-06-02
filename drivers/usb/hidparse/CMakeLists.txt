
spec2def(hidparse.sys hidparse.spec)
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(hidparse SHARED hidparse.c hidparse.rc ${CMAKE_CURRENT_BINARY_DIR}/hidparse.def)

set_module_type(hidparse kernelmodedriver)
add_importlibs(hidparse ntoskrnl)
add_cab_target(hidparse 2)

add_cab_target(hidparse 2)
add_importlib_target(hidparse.spec)
