include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdus.dll kbdus.spec)

add_library(kbdus SHARED kbdus.c kbdus.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdus.def)

set_module_type(kbdus kbdlayout)

add_dependencies(kbdus psdk)

add_cd_file(TARGET kbdus DESTINATION reactos/system32 FOR all)
