
add_definitions(-DDEBUG_MODE)

include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

add_library(kbdhid SHARED kbdhid.c kbdhid.rc)

set_module_type(kbdhid kernelmodedriver)
add_importlibs(kbdhid ntoskrnl hal hidparse)
add_cab_target(kbdhid 2)

add_cab_target(kbdhid 2)

