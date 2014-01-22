
spec2def(ksecdd.sys ksecdd.spec)

list(APPEND SOURCE
     ksecdd.c
     dispatch.c
     random.c
     stubs.c
     ksecdd.rc)

add_library(ksecdd SHARED ${SOURCE})
set_module_type(ksecdd kernelmodedriver)
add_importlibs(ksecdd ntoskrnl hal)
add_cd_file(TARGET ksecdd DESTINATION reactos/system32/drivers NO_CAB FOR all)
