spec2def(imageres.dll imageres.spec)

add_definitions(-D_WINE)

file(GLOB_RECURSE imageres_rc_deps res/*.*)
add_rc_deps(imageres.rc ${imageres_rc_deps})

list(APPEND SOURCE imageres.c)

add_library(imageres MODULE
    ${SOURCE}
    imageres.rc
    ${CMAKE_CURRENT_BINARY_DIR}/imageres.def)

set_module_type(imageres win32dll UNICODE)
target_link_libraries(imageres wine)
add_importlibs(imageres kernel32 ntdll)
add_cd_file(TARGET imageres DESTINATION reactos/system32 FOR all)
