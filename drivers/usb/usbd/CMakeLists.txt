
remove_definitions(-D_WIN32_WINNT=0x502)

spec2def(usbd.sys usbd.spec ADD_IMPORTLIB)

add_library(usbd MODULE
    usbd.c
    hubsup.c
    usbd.rc
    ${CMAKE_CURRENT_BINARY_DIR}/usbd.def)

set_module_type(usbd kernelmodedriver)
target_compile_definitions(usbd PRIVATE _WIN32_WINNT=0x600)
add_importlibs(usbd ntoskrnl hal)
add_cd_file(TARGET usbd DESTINATION reactos/system32/drivers NO_CAB FOR all)
