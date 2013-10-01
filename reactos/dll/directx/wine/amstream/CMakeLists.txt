
add_definitions(
    -D__WINESRC__
    -DWIDL_C_INLINE_WRAPPERS)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(amstream.dll amstream.spec)

list(APPEND SOURCE
    amstream.c
    audiodata.c
    main.c
    mediastream.c
    mediastreamfilter.c
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

add_library(amstream SHARED ${SOURCE} version.rc)
set_module_type(amstream win32dll)
target_link_libraries(amstream strmbase strmiids uuid wine)
add_importlibs(amstream ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)
