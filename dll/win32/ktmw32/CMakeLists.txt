
spec2def(ktmw32.dll ktmw32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ktmw32.c
    ${CMAKE_CURRENT_BINARY_DIR}/ktmw32.def)

add_library(ktmw32 MODULE ${SOURCE} ktmw32.rc)
set_module_type(ktmw32 win32dll)
add_importlibs(ktmw32 kernel32 ntdll)
add_cd_file(TARGET ktmw32 DESTINATION VerOS/system32 FOR all)
