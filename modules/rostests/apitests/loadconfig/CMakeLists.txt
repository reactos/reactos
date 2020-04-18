
spec2def(loadconfig_apitest.exe loadconfig_apitest.spec)

list(APPEND SOURCE
    common.c
    stacktrace.c
    loadconfig.h)

add_executable(loadconfig_apitest ${SOURCE} testlist.c ${CMAKE_CURRENT_BINARY_DIR}/loadconfig_apitest.def)
target_link_libraries(loadconfig_apitest wine ${PSEH_LIB})
set_module_type(loadconfig_apitest win32cui)
add_importlibs(loadconfig_apitest msvcrt kernel32 ntdll)
#add_pch(loadconfig_apitest loadconfig.h SOURCE)
add_rostests_file(TARGET loadconfig_apitest)

# Tell GCC again that we are really interested in exporting symols (who would have figured that we want to use the .def we specified before?)
set_target_properties(loadconfig_apitest
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

fixup_load_config(loadconfig_apitest)
