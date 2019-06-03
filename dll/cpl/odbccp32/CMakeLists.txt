
add_library(odbccp32i MODULE
    odbccp32.c
    odbccp32.rc)
spec2def(odbccp32i odbccp32i.cpl odbccp32.spec)

set_module_type(odbccp32i cpl UNICODE)
add_importlibs(odbccp32i user32 comctl32 msvcrt kernel32)
add_cd_file(TARGET odbccp32i DESTINATION reactos/system32 FOR all)
