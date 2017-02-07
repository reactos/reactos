
if(NSWPAT)
    add_subdirectory(dxtn)
endif()

add_subdirectory(libjpeg)
add_subdirectory(libpng)
add_subdirectory(libtiff)
add_subdirectory(libxslt)

if(NOT MSVC)
    add_subdirectory(mesa32)
endif()
