spec2def(eventsys.dll eventsys.spec)

list(APPEND SOURCE
    eventsys.c)

add_library(eventsys MODULE
    ${SOURCE}
    eventsys.rc
    ${CMAKE_CURRENT_BINARY_DIR}/eventsys.def)

set_source_files_properties(eventsys.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/eventsys.rgs)
set_module_type(eventsys win32dll)
target_link_libraries(eventsys uuid wine)
add_importlibs(eventsys ole32 oleaut32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET eventsys DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(eventsys)
