spec2def(wdfldr.sys wdfldr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
   wdfldr.c
   library.c
   class.c
   common.c
   wdfldr.h
   library.h
   class.h
   common.h
   ntddk_ex.h)

add_library(wdfldr MODULE
    ${SOURCE}
    wdfldr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wdfldr.def)

set_module_type(wdfldr kernelmodedriver ENTRYPOINT 0)
add_importlibs(wdfldr ntoskrnl hal)
add_cd_file(TARGET wdfldr DESTINATION reactos/system32/drivers NO_CAB FOR all)
