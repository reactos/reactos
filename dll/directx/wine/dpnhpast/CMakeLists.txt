
spec2def(dpnhpast.dll dpnhpast.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpnhpast_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpnhpast.def)

add_library(dpnhpast MODULE ${SOURCE} version.rc)
set_module_type(dpnhpast win32dll)
target_link_libraries(dpnhpast dxguid uuid wine)
add_importlibs(dpnhpast msvcrt kernel32 ntdll)
add_cd_file(TARGET dpnhpast DESTINATION reactos/system32 FOR all)
set_wine_module(dpnhpast)
