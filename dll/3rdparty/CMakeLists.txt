
if(NSWPAT)
add_subdirectory(dxtn)
endif()
add_subdirectory(libjpeg)
add_subdirectory(libpng)
add_subdirectory(libtiff)

if(NOT MSVC)
    add_subdirectory(libxslt)
    add_subdirectory(mesa32)
endif()
