
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)

spec2def(kbdja ${CMAKE_CURRENT_SOURCE_DIR}/kbdja.spec ${CMAKE_CURRENT_BINARY_DIR}/kbdja.def)

add_library(kbdja SHARED kbdja.c kbdja.rc)

set_entrypoint(kbdja 0)

target_link_libraries(kbdja ${CMAKE_CURRENT_BINARY_DIR}/kbdja.def)

add_dependencies(kbdja kbdja_def psdk buildno_header)
