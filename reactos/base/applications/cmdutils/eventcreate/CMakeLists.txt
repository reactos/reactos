
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/conutils)

## The message string templates are in ANSI to reduce binary size
add_message_headers(ANSI evtmsgstr.mc)

add_executable(eventcreate eventcreate.c eventcreate.rc)
set_module_type(eventcreate win32cui UNICODE)
add_dependencies(eventcreate evtmsgstr)
target_link_libraries(eventcreate conutils ${PSEH_LIB})
add_importlibs(eventcreate advapi32 msvcrt kernel32)
add_cd_file(TARGET eventcreate DESTINATION reactos/system32 FOR all)
