
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdlt1.dll kbdlt1.spec)

add_library(kbdlt1 SHARED kbdlt1.c kbdlt1.rc ${CMAKE_CURRENT_BINARY_DIR}/kbdlt1.def)

set_entrypoint(kbdlt1 0)

add_dependencies(kbdlt1 psdk)

add_cd_file(TARGET kbdlt1 DESTINATION reactos/system32 FOR all)
