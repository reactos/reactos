
spec2def(comcat.dll comcat.spec)

list(APPEND SOURCE ${CMAKE_CURRENT_BINARY_DIR}/comcat.def)

add_library(comcat MODULE ${SOURCE} version.rc)
set_module_type(comcat win32dll)
target_link_libraries(comcat wine_dll_register wine_dll_canunload)
add_importlibs(comcat ole32 msvcrt kernel32)
add_cd_file(TARGET comcat DESTINATION reactos/system32 FOR all)
set_wine_module(comcat)
