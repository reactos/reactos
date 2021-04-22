add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    copysup.c)

add_library(copysup ${SOURCE})
target_link_libraries(copysup PRIVATE pseh)
add_dependencies(copysup bugcodes xdk)
