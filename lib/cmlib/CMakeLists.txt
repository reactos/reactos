

add_definitions(-D_NTOSKRNL_ -D_NTSYSTEM_ -DNASSERT)

file(GLOB_RECURSE SOURCE "*.c")

add_library(cmlib ${SOURCE})