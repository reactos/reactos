
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdmac.dll kbdmac.spec)

add_library(kbdmac SHARED kbdmac.c kbdmac.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdmac.def)

set_module_type(kbdmac kbdlayout)

add_dependencies(kbdmac psdk)

add_cd_file(TARGET kbdmac DESTINATION reactos/system32 FOR all)
