
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdgrist.dll kbdgrist.spec)

add_library(kbdgrist SHARED kbdgrist.c kbdgrist.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdgrist.def)

set_module_type(kbdgrist kbdlayout)

add_dependencies(kbdgrist psdk)

add_cd_file(TARGET kbdgrist DESTINATION reactos/system32 FOR all)
