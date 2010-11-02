
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(getuname SHARED getuname.c getuname.rc getuname.def)

if(NOT MSVC)
    set_source_files_properties(getuname.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(getuname win32dll)

add_importlibs(getuname kernel32 ntdll)

add_cab_target(getuname 1)
