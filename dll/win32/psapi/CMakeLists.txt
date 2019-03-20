
spec2def(psapi.dll psapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    psapi.c
    psapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/psapi.def)

add_library(psapi MODULE ${SOURCE})
set_module_type(psapi win32dll)
target_link_libraries(psapi ${PSEH_LIB})
add_importlibs(psapi msvcrt kernel32 ntdll)
add_cd_file(TARGET psapi DESTINATION reactos/system32 FOR all)
