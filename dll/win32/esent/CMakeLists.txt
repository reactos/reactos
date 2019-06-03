
list(APPEND SOURCE)
add_library(esent MODULE ${SOURCE})
spec2def(esent esent.dll esent.spec)

set_module_type(esent win32dll)
add_importlibs(esent msvcrt kernel32 ntdll)
add_cd_file(TARGET esent DESTINATION reactos/system32 FOR all)
