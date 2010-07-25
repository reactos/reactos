include_directories(.)

SET(GLOBAL_FILES infcore.c infget.c infput.c)

if(CMAKE_CROSSCOMPILING)

file(GLOB_RECURSE SOURCE "infros*.c")
add_library(inflib ${GLOBAL_FILES} ${SOURCE})

else()

file(GLOB_RECURSE SOURCE "infhost*.c")

add_definitions(-D__NO_CTYPE_INLINES -DINFLIB_HOST)
add_definitions(-Wpointer-arith -Wconversion -Wstrict-prototypes -Wmissing-prototypes)
add_library(inflibhost ${GLOBAL_FILES} ${SOURCE})

endif()
