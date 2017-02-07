
add_definitions(-D__WINESRC__)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

set_rc_compiler()
spec2def(amstream.dll amstream.spec)

add_library(amstream SHARED
    amstream.c
    main.c
    mediastream.c
    mediastreamfilter.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

set_module_type(amstream win32dll)
target_link_libraries(amstream strmiids uuid wine)
add_importlibs(amstream ole32 advapi32 msvcrt kernel32 ntdll)
add_dependencies(amstream dxsdk)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)
