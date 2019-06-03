
list(APPEND SOURCE
    wmi.rc)

add_library(wmi MODULE ${SOURCE})
spec2def(wmi wmi.dll wmi.spec)

set_module_type(wmi win32dll ENTRYPOINT 0)
add_importlibs(wmi advapi32)
add_dependencies(wmi psdk)
add_cd_file(TARGET wmi DESTINATION reactos/system32 FOR all)
