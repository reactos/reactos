include_directories(./inc)

file(GLOB_RECURSE SOURCE "*.c")

add_library(dnslib ${SOURCE})