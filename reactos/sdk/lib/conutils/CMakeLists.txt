
## Temporary HACK before we cleanly support UNICODE functions
add_definitions(-DUNICODE -D_UNICODE)

add_library(conutils conutils.c)
add_dependencies(conutils psdk)
target_link_libraries(conutils ${PSEH_LIB})
# add_importlibs(conutils msvcrt kernel32) ## ntdll
