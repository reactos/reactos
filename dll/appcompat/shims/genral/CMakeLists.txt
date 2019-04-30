
include_directories(${SHIMLIB_DIR})

spec2def(acgenral.dll genral.spec)

list(APPEND SOURCE
    ignoredbgout.c
    ignorefreelib.c
    main.c
    shimtest.c
    themes.c
    genral.spec)

add_library(acgenral MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/acgenral.def)

set_module_type(acgenral win32dll ENTRYPOINT DllMain 12)
target_link_libraries(acgenral shimlib)
add_importlibs(acgenral uxtheme kernel32 ntdll)
add_cd_file(TARGET acgenral DESTINATION reactos/AppPatch FOR all)
