
set_unicode()

pdef2def(acledit.pdef)

list(APPEND SOURCE
    acledit.c
    stubs.c
    acledit.rc
    ${CMAKE_CURRENT_BINARY_DIR}/acledit.def)

if(NOT MSVC)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/acledit.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(acledit SHARED ${SOURCE})

set_module_type(acledit win32dll)

add_importlibs(acledit ntdll)

add_cab_target(acledit 1)
