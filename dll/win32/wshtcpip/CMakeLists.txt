
set_unicode()

include_directories(${REACTOS_SOURCE_DIR}/lib/tdilib)

add_library(wshtcpip SHARED wshtcpip.c wshtcpip.rc wshtcpip.def)

if(NOT MSVC)
    set_source_files_properties(wshtcpip.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

set_entrypoint(wshtcpip 0)

target_link_libraries(wshtcpip tdilib)

add_importlibs(wshtcpip ws2_32 kernel32 ntdll)
add_dependencies(wshtcpip psdk buildno_header)

add_livecd_target(wshtcpip reactos/system32)