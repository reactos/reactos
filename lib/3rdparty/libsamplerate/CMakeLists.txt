add_definitions(-DHAVE_LRINT -DHAVE_LRINTF)
file(GLOB_RECURSE SOURCE "*.c")

add_library(libsamplerate ${SOURCE})