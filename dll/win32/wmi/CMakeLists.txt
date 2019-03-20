
spec2def(wmi.dll wmi.spec)

list(APPEND SOURCE
    wmi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wmi.def)

add_library(wmi MODULE ${SOURCE})
set_module_type(wmi win32dll ENTRYPOINT 0 )
add_importlibs(wmi advapi32)
add_dependencies(wmi psdk)
add_cd_file(TARGET wmi DESTINATION reactos/system32 FOR all)
