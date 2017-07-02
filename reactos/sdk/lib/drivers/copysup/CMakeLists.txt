add_definitions(-DUNICODE -D_UNICODE)

list(APPEND SOURCE
    copysup.c)

add_library(copysup ${SOURCE})
add_dependencies(copysup bugcodes xdk)
