
add_library(watchdog MODULE
    watchdog.c)
spec2def(watchdog watchdog.sys watchdog.spec)

set_module_type(watchdog kernelmodedriver)
add_importlibs(watchdog ntoskrnl)
add_cd_file(TARGET watchdog DESTINATION reactos/system32/drivers FOR all)
