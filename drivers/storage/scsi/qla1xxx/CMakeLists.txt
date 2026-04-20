
list(APPEND SOURCE
     isp.h
     eeprom.c
     qla1xxx.c
     qla1xxx.h)

add_library(qla1xxx MODULE ${SOURCE} qla1xxx.rc)
target_link_libraries(qla1xxx libcntpr)
set_module_type(qla1xxx kernelmodedriver)
add_pch(qla1xxx qla1xxx.h SOURCE)
if(DBG)
    # Required for ASSERT()
    add_importlibs(qla1xxx storport ntoskrnl)
else()
    add_importlibs(qla1xxx storport)
endif()

add_cd_file(TARGET qla1xxx DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(qla1xxx qla1xxx.inf)
