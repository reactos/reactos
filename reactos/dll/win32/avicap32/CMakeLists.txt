
spec2def(avicap32.dll avicap32.spec ADD_IMPORTLIB)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(avicap32 SHARED
    avicap32.c
    avicap32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/avicap32.def)

set_module_type(avicap32 win32dll UNICODE)
target_link_libraries(avicap32 wine)
add_importlibs(avicap32 advapi32 user32 version msvcrt kernel32 ntdll)
add_cd_file(TARGET avicap32 DESTINATION reactos/system32 FOR all)
