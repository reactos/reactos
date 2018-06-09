
spec2def(verifier.dll verifier.spec)

list(APPEND SOURCE
    verifier.c
    verifier_customstubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/verifier.def)

add_library(verifier SHARED ${SOURCE})
set_module_type(verifier win32dll UNICODE ENTRYPOINT DllMain 12)
add_importlibs(verifier ntdll)
add_cd_file(TARGET verifier DESTINATION reactos/system32 FOR all)
