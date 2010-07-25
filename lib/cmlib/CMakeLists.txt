
file(GLOB_RECURSE SOURCE "*.c")

add_definitions(-D_NTOSKRNL_ -D_NTSYSTEM_ -DNASSERT)

if(CMAKE_CROSSCOMPILING)

add_library(cmlib ${SOURCE})

else()

add_definitions(-DWINE_UNICODE_API= -D__NO_CTYPE_INLINES -DCMLIB_HOST)
add_library(cmlibhost ${SOURCE})
target_link_libraries(cmlibhost unicode)

endif()

