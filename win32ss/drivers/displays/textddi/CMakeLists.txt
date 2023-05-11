list(APPEND SOURCE
  enable.c
  textddi.h)

add_library(textddi MODULE
  ${SOURCE}
  textddi.rc)

set_module_type(textddi kerneldll ENTRYPOINT DrvEnableDriver 12)
add_pch(textddi textddi.h SOURCE)
target_link_libraries(textddi libcntpr)
add_importlibs(textddi win32k)
add_dependencies(textddi psdk)
add_cd_file(TARGET textddi DESTINATION reactos/system32 FOR all)
