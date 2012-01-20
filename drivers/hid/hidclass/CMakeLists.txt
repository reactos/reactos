
spec2def(hidclass.sys hidclass.spec)

list(APPEND SOURCE
    fdo.c
    hidclass.c
    hidclass.rc
    pdo.c
    ${CMAKE_CURRENT_BINARY_DIR}/hidclass.def)

add_library(hidclass SHARED ${SOURCE})

set_entrypoint(hidclass 0)
set_image_base(hidclass 0x00010000)
set_subsystem(hidclass native)
set_target_properties(hidclass PROPERTIES SUFFIX ".sys")


add_importlibs(hidclass ntoskrnl hidparse hal)
add_importlib_target(hidclass.spec)