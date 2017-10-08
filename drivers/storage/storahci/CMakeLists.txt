add_definitions(-DDEBUG)

list(APPEND SOURCE
    storahci.c)

add_library(storahci SHARED ${SOURCE} storahci.rc)

set_module_type(storahci kernelmodedriver)
add_importlibs(storahci storport ntoskrnl hal)
add_cd_file(TARGET storahci DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(storahci.inf)
