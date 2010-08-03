include_directories(inc)

add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCE "*.c")

add_library(dnslib ${SOURCE})