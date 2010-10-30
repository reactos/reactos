
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(slbcsp.dll slbcsp.spec)

add_library(slbcsp SHARED main.c ${CMAKE_CURRENT_BINARY_DIR}/slbcsp.def)

set_module_type(slbcsp win32dll)

target_link_libraries(slbcsp wine)

add_importlibs(slbcsp kernel32 ntdll)

add_cab_target(slbcsp 1)
