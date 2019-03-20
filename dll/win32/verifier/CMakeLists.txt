
spec2def(verifier.dll verifier.spec)

list(APPEND SOURCE
    verifier.c
    ${CMAKE_CURRENT_BINARY_DIR}/verifier.def
    ${CMAKE_CURRENT_BINARY_DIR}/verifier_stubs.c)

add_library(verifier MODULE ${SOURCE})
set_module_type(verifier win32dll UNICODE ENTRYPOINT DllMain 12)
add_importlibs(verifier ntdll)  # Only ntdll!
add_cd_file(TARGET verifier DESTINATION reactos/system32 FOR all)
