
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbda2.dll kbda2.spec)

add_library(kbda2 SHARED kbda2.c kbda2.rc ${CMAKE_CURRENT_BINARY_DIR}/kbda2.def)

set_module_type(kbda2 kbdlayout)

add_dependencies(kbda2 psdk)

add_cd_file(TARGET kbda2 DESTINATION reactos/system32 FOR all)
