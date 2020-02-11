add_definitions(-D_WINE)

file(GLOB_RECURSE imageres_rc_deps res/*.*)
add_rc_deps(imageres.rc ${imageres_rc_deps})

add_library(imageres MODULE
    ${SOURCE}
    imageres.rc)

set_module_type(imageres win32dll ENTRYPOINT 0)
add_cd_file(TARGET imageres DESTINATION reactos/system32 FOR all)
