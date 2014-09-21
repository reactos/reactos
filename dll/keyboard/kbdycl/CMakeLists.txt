
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdycl.dll kbdycl.spec)

add_library(kbdycl SHARED kbdycl.c kbdycl.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdycl.def)

set_module_type(kbdycl kbdlayout)

add_dependencies(kbdycl psdk)

add_cd_file(TARGET kbdycl DESTINATION reactos/system32 FOR all)
