
list(APPEND SOURCE
     mup.c
     mup.h)

add_library(mup SHARED ${SOURCE} mup.rc)
set_module_type(mup kernelmodedriver)
target_link_libraries(mup ${PSEH_LIB})
add_importlibs(mup ntoskrnl hal)
add_pch(mup mup.h SOURCE)
add_cd_file(TARGET mup DESTINATION reactos/system32/drivers FOR all)
