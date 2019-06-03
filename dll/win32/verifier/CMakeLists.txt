
list(APPEND SOURCE
    verifier.c)

add_library(verifier MODULE ${SOURCE})
spec2def(verifier verifier.dll verifier.spec)

set_module_type(verifier win32dll UNICODE ENTRYPOINT DllMain 12)
add_importlibs(verifier ntdll)  # Only ntdll!
add_cd_file(TARGET verifier DESTINATION reactos/system32 FOR all)
