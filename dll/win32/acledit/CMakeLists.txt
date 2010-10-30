
set_unicode()

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc)

spec2def(acledit ${CMAKE_CURRENT_SOURCE_DIR}/acledit.spec ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

add_library(acledit SHARED ${SOURCE})

set_module_type(acledit win32dll)

target_link_libraries(acledit ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)
add_importlibs(acledit ntdll)

add_dependencies(acledit acledit_def)
add_cab_target(acledit 1)
