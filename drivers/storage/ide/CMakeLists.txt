
add_subdirectory(atapi)
if(ARCH STREQUAL "i386" AND NOT ENABLE_UNIATA)
    add_subdirectory(legide)
endif()
add_subdirectory(pciide)
add_subdirectory(pciidex)
add_subdirectory(uniata)
