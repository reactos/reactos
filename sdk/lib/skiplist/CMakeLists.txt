
# The library
add_definitions(-DSKIPLIST_LEVELS=16)
add_library(skiplist16 skiplist.c)
add_dependencies(skiplist16 psdk)

# The Test program
add_executable(skiplist_test skiplist_test.c)
target_link_libraries(skiplist_test skiplist16)
set_module_type(skiplist_test win32cui)
add_importlibs(skiplist_test msvcrt kernel32 ntdll)
