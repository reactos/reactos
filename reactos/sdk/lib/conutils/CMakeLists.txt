
##
## 'conutils_base': the base utility library.
##
add_library(conutils_base utils.c)
add_dependencies(conutils_base xdk)
target_link_libraries(conutils_base ${PSEH_LIB})
add_importlibs(conutils_base msvcrt kernel32)

##
## 'conutils_stream': Console Stream API library.
##
add_library(conutils_stream stream.c)
target_link_libraries(conutils_stream PUBLIC conutils_base)
# add_dependencies(conutils_stream conutils_base xdk)

##
## 'conutils_screen': Console Screen API library.
##
add_library(conutils_screen screen.c)
target_link_libraries(conutils_screen PUBLIC conutils_stream conutils_base)
# add_dependencies(conutils_screen conutils_stream conutils_base xdk)

##
## 'conutils_pager': Console Pager API library.
##
add_library(conutils_pager pager.c)
target_link_libraries(conutils_pager PUBLIC conutils_screen conutils_stream conutils_base)
# add_dependencies(conutils_pager conutils_screen conutils_stream conutils_base xdk)

##
## 'conutils': Library target that groups everything together.
##
add_library(conutils INTERFACE)
target_link_libraries(conutils INTERFACE conutils_pager conutils_screen conutils_stream conutils_base)
# add_dependencies(conutils conutils_pager conutils_screen conutils_stream conutils_base xdk)
