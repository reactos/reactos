
set_unicode()

add_definitions(-DLANGPACK)
include_directories(include)

add_library(lpk SHARED dllmain.c stub.c lpk.rc lpk.def)

if(NOT MSVC)
    set_source_files_properties(lpk.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_module_type(lpk win32dll)

add_importlibs(lpk user32 usp10 kernel32 ntdll)

add_cab_target(lpk 1)
