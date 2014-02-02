
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdal.dll kbdal.spec)

add_library(kbdal SHARED kbdal.c kbdal.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdal.def)

set_module_type(kbdal kbdlayout)

add_dependencies(kbdal psdk)

add_cd_file(TARGET kbdal DESTINATION reactos/system32 FOR all)
