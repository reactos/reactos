
list(APPEND SOURCE
    partition.c
    partmgr.c
    utils.c)

list(APPEND PCH_SKIP_SOURCE
    guid.c)

add_library(partmgr MODULE
	${SOURCE}
	${PCH_SKIP_SOURCE}
	partmgr.rc)

add_pch(partmgr partmgr.h "${PCH_SKIP_SOURCE}")
set_module_type(partmgr kernelmodedriver)
add_importlibs(partmgr ntoskrnl hal)
add_registry_inf(partmgr_reg.inf)
add_cd_file(TARGET partmgr DESTINATION reactos/system32/drivers NO_CAB FOR all)
