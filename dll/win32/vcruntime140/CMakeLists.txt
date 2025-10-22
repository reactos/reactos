
spec2def(vcruntime140.dll vcruntime140.spec)

list(APPEND SOURCE
    misc.c
    ${CMAKE_CURRENT_BINARY_DIR}/vcruntime140_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/vcruntime140.def)

add_library(vcruntime140 MODULE ${SOURCE})
set_module_type(vcruntime140 win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(vcruntime140 wine)

add_importlibs(vcruntime140 kernel32_vista user32 kernel32 ucrtbase ntdll)
add_cd_file(TARGET vcruntime140 DESTINATION reactos/system32 FOR all)
set_wine_module(vcruntime140)
