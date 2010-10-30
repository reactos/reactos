
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(amstream ${CMAKE_CURRENT_SOURCE_DIR}/amstream.spec ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

add_library(amstream SHARED
    amstream.c
    main.c
    mediastream.c
    mediastreamfilter.c
    regsvr.c
    amstream_i.c
    version.rc)

set_module_type(amstream win32dll)

target_link_libraries(amstream
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def
    strmiids
    uuid
    wine)

add_importlibs(amstream
    ole32
    advapi32
    kernel32
    ntdll)

add_dependencies(amstream amstream_def dxsdk)
add_cab_target(amstream 1)
